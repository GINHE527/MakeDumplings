// Fill out your copyright notice in the Description page of Project Settings.


#include "Pythonrun.h"

using Ptr = TSharedPtr<Pythonrun, ESPMode::ThreadSafe>;

uint32 Pythonrun::InstanceCounter = 0;
TMap<uint32, Pythonrun::Ptr> Pythonrun::PythonRunList;
FCriticalSection Pythonrun::InstanceMutex;

void Pythonrun::Start()
{
	if (RunState != ERunState::Stopped) return;

	// 创建命名管道（示例名称：PythonRunPipe_123）
	FString PipeName = FString::Printf(TEXT("\\\\.\\pipe\\PythonRunPipe_%d"), RunId);

	PipeHandle = CreateNamedPipe(
		*PipeName,
		PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		1,  // 单实例管道
		4096,  // 输出缓冲区
		4096,  // 输入缓冲区
		0,
		NULL
	);

	if (PipeHandle == INVALID_HANDLE_VALUE) {
		LastError = FString::Printf(TEXT("CreateNamedPipe failed (%d)"), GetLastError());
		RunState = ERunState::Error;
		return;
	}

	// 启动Python进程
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	FString CommandLine = FString::Printf(TEXT("\"%s\" \"%s\""), *PythonPath, *RunPythonExePath);

	if (!CreateProcess(
		NULL,
		const_cast<LPWSTR>(*CommandLine),
		NULL,
		NULL,
		FALSE,
		CREATE_NEW_CONSOLE,
		NULL,
		NULL,
		&si,
		&pi))
	{
		LastError = FString::Printf(TEXT("CreateProcess failed (%d)"), GetLastError());
		CloseHandle(PipeHandle);
		RunState = ERunState::Error;
		return;
	}

	PythonProcessHandle = pi.hProcess;
	CloseHandle(pi.hThread);
	RunState = ERunState::Running;
}

void Pythonrun::ForceStop()
{
	if (PythonProcessHandle != INVALID_HANDLE_VALUE) {
		TerminateProcess(PythonProcessHandle, 0);
		CloseHandle(PythonProcessHandle);
		PythonProcessHandle = INVALID_HANDLE_VALUE;
	}

	if (PipeHandle != INVALID_HANDLE_VALUE) {
		DisconnectNamedPipe(PipeHandle);
		CloseHandle(PipeHandle);
		PipeHandle = INVALID_HANDLE_VALUE;
	}

	RunState = ERunState::Stopped;
}

void Pythonrun::SafeStop()
{
	if (RunState != ERunState::Running) return;

	// 发送终止指令
	SendMessage(TEXT("EXIT"));

	// 等待正常退出
	if (WaitForSingleObject(PythonProcessHandle, 5000) == WAIT_TIMEOUT) {
		ForceStop();
	}
	else {
		CloseHandle(PythonProcessHandle);
		PythonProcessHandle = INVALID_HANDLE_VALUE;
		RunState = ERunState::Stopped;
	}
}

// Restart实现
void Pythonrun::Restart()
{
	SafeStop();
	if (RunState == ERunState::Stopped)
	{
		// 重置管道和进程句柄
		PipeHandle = INVALID_HANDLE_VALUE;
		PythonProcessHandle = INVALID_HANDLE_VALUE;
		Start();
	}
}

void Pythonrun::UpdateCpuUsage()
{
	if (PythonProcessHandle == INVALID_HANDLE_VALUE) return;

	FILETIME createTime, exitTime, kernelTime, userTime;
	if (GetProcessTimes(PythonProcessHandle, &createTime, &exitTime, &kernelTime, &userTime)) {
		ULARGE_INTEGER kernel = { { kernelTime.dwLowDateTime, kernelTime.dwHighDateTime } };
		ULARGE_INTEGER user = { { userTime.dwLowDateTime, userTime.dwHighDateTime } };
		CpuUsagePercent = (kernel.QuadPart + user.QuadPart) / 10000.0f;
	}
}

// 日志系统实现
void Pythonrun::PrintLog()
{
	FScopeLock Lock(&LogMutex);
	for (const auto& Msg : AsyncMessageQueue)
	{
		UE_LOG(LogTemp, Display, TEXT("[PythonRun %d] %s"), RunId, *Msg);
		MessageAll.Add(Msg);
	}
	AsyncMessageQueue.Empty();
}

void Pythonrun::FlushAsyncLogs()
{
	FScopeLock Lock(&LogMutex);
	MessageAll.Append(AsyncMessageQueue);
	AsyncMessageQueue.Empty();
}

void Pythonrun::AsyncPrintLog()
{
	TArray<FString> NewMessages = ReceiveMessages();
	FScopeLock Lock(&LogMutex);
	AsyncMessageQueue.Append(NewMessages);
}

void Pythonrun::SendMessage(const FString& InMessage)
{
	if (RunState != ERunState::Running) return;

	DWORD bytesWritten;
	OVERLAPPED overlapped = { 0 };
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// 异步写入
	if (!WriteFile(
		PipeHandle,
		*InMessage,
		InMessage.Len() * sizeof(TCHAR),
		&bytesWritten,
		&overlapped))
	{
		if (GetLastError() != ERROR_IO_PENDING) {
			LastError = FString::Printf(TEXT("WriteFile failed (%d)"), GetLastError());
			RunState = ERunState::Error;
		}
	}

	// 等待写入完成
	WaitForSingleObject(overlapped.hEvent, INFINITE);
	CloseHandle(overlapped.hEvent);
}

TArray<FString> Pythonrun::ReceiveMessages()
{
	TArray<FString> Result;
	if (RunState != ERunState::Running) return Result;

	CHAR buffer[4096];
	DWORD bytesRead;
	OVERLAPPED overlapped = { 0 };
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	while (true) {
		if (!ReadFile(
			PipeHandle,
			buffer,
			sizeof(buffer),
			&bytesRead,
			&overlapped))
		{
			if (GetLastError() == ERROR_MORE_DATA) continue;
			if (GetLastError() == ERROR_IO_PENDING) {
				WaitForSingleObject(overlapped.hEvent, INFINITE);
				GetOverlappedResult(PipeHandle, &overlapped, &bytesRead, FALSE);
			}
			else break;
		}

		if (bytesRead > 0) {
			FString Messagess = FString(bytesRead, UTF8_TO_TCHAR(buffer));
			Result.Add(Messagess);
		}
		else break;
	}

	CloseHandle(overlapped.hEvent);
	return Result;
}

Ptr Pythonrun::GetInstanceById(uint32 id)
{
	if (!PythonRunList.Contains(id)) {
		return Ptr();
	}
	else 
		return PythonRunList[id];
}

// 实例管理实现
Pythonrun::Ptr Pythonrun::CreateById(EPriorityLevel InPriority, FString AttributeName, uint32 id, FString InRunPythonExePath)
{
	FScopeLock Lock(&InstanceMutex);

	if (PythonRunList.Contains(id))
	{
		UE_LOG(LogTemp, Warning, TEXT("Instance ID %d already exists!"), id);
		return nullptr;
	}

	Ptr NewInstance = MakeShared<Pythonrun, ESPMode::ThreadSafe>();
	NewInstance->RunId = id;
	NewInstance->Attribute = AttributeName;
	NewInstance->Priority = InPriority;
	NewInstance->RunPythonExePath = InRunPythonExePath;

	PythonRunList.Add(id, NewInstance);
	InstanceCounter = FMath::Max(InstanceCounter, id + 1);

	return NewInstance;
}

Pythonrun::Ptr Pythonrun::AddRunner(EPriorityLevel InPriority, FString AttributeName, FString InRunPythonExePath)
{
	FScopeLock Lock(&InstanceMutex);

	if (InstanceCounter >= 1000)
	{
		UE_LOG(LogTemp, Error, TEXT("Maximum instance limit reached!"));
		return nullptr;
	}

	const uint32 NewId = InstanceCounter++;
	return CreateById(InPriority, AttributeName, NewId, InRunPythonExePath);
}

void Pythonrun::ClearNoRunPython()
{
	FScopeLock Lock(&InstanceMutex);

	TArray<uint32> ToRemove;
	for (const auto& Pair : PythonRunList)
	{
		if (Pair.Value->RunState == ERunState::Stopped ||
			Pair.Value->RunState == ERunState::Error)
		{
			Pair.Value->ForceStop();  // 确保资源释放
			ToRemove.Add(Pair.Key);
		}
	}

	for (const auto& Key : ToRemove)
	{
		PythonRunList.Remove(Key);
	}
}