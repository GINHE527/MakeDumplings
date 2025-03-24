// Copyright GINHE, Inc. All Rights Reserved.


#include "Pythonrun.h"

//Pyrunֻ��ָ��
using Ptr = TSharedPtr<Pythonrun, ESPMode::ThreadSafe>;

// ʵ��ID������
uint32 Pythonrun::InstanceCounter = 0;
// ʵ���洢����
TMap<uint32, Pythonrun::Ptr> Pythonrun::PythonRunList;
// ʵ���������߳���
FCriticalSection Pythonrun::InstanceMutex;

//-----------------------------------------------------------------------------
// ���ܣ�����Python���̺�ͨ�Źܵ�
// ע�⣺����Stopped״̬��Ч��Error״̬��Ҫ�ֶ�����
//-----------------------------------------------------------------------------
void Pythonrun::Start()
{
	// ״̬У�飺�������ֹͣ״̬����
	if (RunState != ERunState::Stopped) return;

	//�ܵ����Ƹ�ʽ��\\.\pipe\PythonRunPipe_<RunId>
	FString PipeName = FString::Printf(TEXT("\\\\.\\pipe\\UnrealPythonRunPipe_%d"), RunId);

	// �����첽�����ܵ���Windows API��
	PipeHandle = CreateNamedPipe(
		*PipeName, // �ܵ�����
		PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,  // ˫��ͨ��+�첽ģʽ
		PIPE_TYPE_MESSAGE | // ��Ϣ���͹ܵ�
		PIPE_READMODE_MESSAGE | // ��Ϣ��ȡģʽ
		PIPE_WAIT, // �����ȴ�ģʽ
		1,  // ��ʵ���ܵ�
		4096,  // ���������
		4096,  // ���뻺����
		0, // Ĭ�ϳ�ʱʱ�䣨50ms��
		NULL  // ��ȫ���ԣ�Ĭ�ϣ�
	);

	// ��������¼���󲢸���״̬
	if (PipeHandle == INVALID_HANDLE_VALUE) {
		LastError = FString::Printf(TEXT("CreateNamedPipe failed (%d)"), GetLastError());
		RunState = ERunState::Error;
		return;
	}

	//  /* �ڶ��׶Σ�����Python�ӽ��� */
	STARTUPINFO si = { sizeof(si) }; // �������������ṹ��
	PROCESS_INFORMATION pi;  // ������Ϣ�ṹ��

	// ���������У�ȷ��·�������Ŵ���ո�
	FString CommandLine = FString::Printf(TEXT("\"%s\" \"%s\""), *PythonPath, *RunPythonExePath);

	// �����½��̣�����ʾ����̨���ڣ�
	if (!CreateProcess(
		NULL, // �����½��̣���ʾ����̨���ڣ�
		const_cast<LPWSTR>(*CommandLine),  // �����в���
		NULL, // ���̰�ȫ����
		NULL,  // �̰߳�ȫ����
		FALSE, // ���̳о��
		0, // �������¿���̨����
		NULL,
		NULL,
		&si,
		&pi))
	{
		// �������رչܵ�����¼����
		LastError = FString::Printf(TEXT("CreateProcess failed (%d)"), GetLastError());
		CloseHandle(PipeHandle);
		RunState = ERunState::Error;
		return;
	}
	// ������̾���������߳̾��
	PythonProcessHandle = pi.hProcess;
	CloseHandle(pi.hThread); // ����Ҫ�߳̿���

	// ����״̬Ϊ������
	RunState = ERunState::Running;
}

//-----------------------------------------------------------------------------
// ���ܣ�ǿ����ֹPython���̲�������Դ
// ע�⣺������ֹ���ܵ������ݶ�ʧ������ʹ��SafeStop
//-----------------------------------------------------------------------------
void Pythonrun::ForceStop()
{
	// ��ֹPython����
	if (PythonProcessHandle != INVALID_HANDLE_VALUE) {
		TerminateProcess(PythonProcessHandle, 0); // ǿ���˳�����0
		CloseHandle(PythonProcessHandle);
		PythonProcessHandle = INVALID_HANDLE_VALUE;
	}

	// �رչܵ�����
	if (PipeHandle != INVALID_HANDLE_VALUE) {
		DisconnectNamedPipe(PipeHandle); // �Ͽ��ͻ�������
		CloseHandle(PipeHandle);
		PipeHandle = INVALID_HANDLE_VALUE;
	}
	// ����״̬Ϊ��ֹͣ
	RunState = ERunState::Stopped;
}

//-----------------------------------------------------------------------------
// ���ܣ���ȫֹͣPython���̣�����ʹ�ã�
// ���̣�1.�����˳�ָ�� 2.�ȴ������˳� 3.��ʱǿ����ֹ
//-----------------------------------------------------------------------------
void Pythonrun::SafeStop()
{
	if (RunState != ERunState::Running) return;

	// ����1��ͨ���ܵ������˳�ָ��
	SendMessage(TEXT("EXIT"));

	// ����2���ȴ������˳���5�볬ʱ��
	if (WaitForSingleObject(PythonProcessHandle, 5000) == WAIT_TIMEOUT) {
		ForceStop(); // ��ʱ��ǿ����ֹ
	}
	else {
		// �����ر�����
		CloseHandle(PythonProcessHandle);
		PythonProcessHandle = INVALID_HANDLE_VALUE;
		RunState = ERunState::Stopped;
	}
}

//-----------------------------------------------------------------------------
// ���ܣ�����Python����
// ע�⣺���ȳɹ�ֹͣ��ǰʵ��
//-----------------------------------------------------------------------------
void Pythonrun::Restart()
{
	SafeStop();  // ���԰�ȫֹͣ

	// ȷ�Ͻ���ֹͣ״̬������
	if (RunState == ERunState::Stopped)
	{
		// ���ùܵ��ͽ��̾��
		PipeHandle = INVALID_HANDLE_VALUE;
		PythonProcessHandle = INVALID_HANDLE_VALUE;
		Start();  // ��������
	}
}

//-----------------------------------------------------------------------------
// ���ܣ����½���CPUʹ����
// ԭ��ͨ������ʱ�����CPUռ�ðٷֱ�
//-----------------------------------------------------------------------------
void Pythonrun::UpdateCpuUsage()
{
	if (PythonProcessHandle == INVALID_HANDLE_VALUE) return;

	FILETIME createTime, exitTime, kernelTime, userTime;
	if (GetProcessTimes(PythonProcessHandle, &createTime, &exitTime, &kernelTime, &userTime)) {
		// ��FILETIMEת��Ϊ64λ����
		ULARGE_INTEGER kernel = { { kernelTime.dwLowDateTime, kernelTime.dwHighDateTime } };
		ULARGE_INTEGER user = { { userTime.dwLowDateTime, userTime.dwHighDateTime } };

		// ������CPUʱ�䲢ת��Ϊ�ٷֱȣ�����1������
		CpuUsagePercent = (kernel.QuadPart + user.QuadPart) / 10000.0f;
	}
}

//-----------------------------------------------------------------------------
// ��־ϵͳʵ��
// ��ƣ��첽��Ϣ���б�֤�̰߳�ȫ������־�洢������¼
//-----------------------------------------------------------------------------

// ���ܣ����첽��־�����������־ϵͳ
void Pythonrun::PrintLog()
{
	// ������֤�̰߳�ȫ
	FScopeLock Lock(&LogMutex);
	// �����첽���У������ת��������־
	for (const auto& Msg : AsyncMessageQueue)
	{
		UE_LOG(LogTemp, Display, TEXT("[PythonRun %d] %s"), RunId, *Msg);
		MessageAll.Add(Msg); // �鵵������־
	}
	AsyncMessageQueue.Empty(); // �����ʱ����
}

// ���ܣ��ϲ��첽��־������־���������
void Pythonrun::FlushAsyncLogs()
{
	FScopeLock Lock(&LogMutex);
	MessageAll.Append(AsyncMessageQueue); // ֱ��׷��
	AsyncMessageQueue.Empty();
}

// ���ܣ���������Ϣ���첽����
void Pythonrun::AsyncPrintLog()
{
	TArray<FString> NewMessages = ReceiveMessages(); // �ӹܵ���ȡ
	FScopeLock Lock(&LogMutex);
	AsyncMessageQueue.Append(NewMessages); // ׷�ӵ���ʱ����
}

//-----------------------------------------------------------------------------
// ���̼�ͨ�ŷ���
// ע�⣺ʹ���첽IO���������������߳�
//-----------------------------------------------------------------------------
void Pythonrun::SendMessage(const FString& InMessage)
{
	if (RunState != ERunState::Running) return;

	DWORD bytesWritten;
	OVERLAPPED overlapped = { 0 };
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// �첽д��
	if (!WriteFile(
		PipeHandle, // �ܵ����
		*InMessage, // ��Ϣ����
		InMessage.Len() * sizeof(TCHAR),  // �ֽڳ���
		&bytesWritten, // ʵ��д���ֽ���
		&overlapped))  // �첽�����ṹ
	{
		// ����������IO����״̬��
		if (GetLastError() != ERROR_IO_PENDING) {
			LastError = FString::Printf(TEXT("WriteFile failed (%d)"), GetLastError());
			RunState = ERunState::Error;
		}
	}

	// �ȴ�д�����
	WaitForSingleObject(overlapped.hEvent, INFINITE);
	CloseHandle(overlapped.hEvent);
}

// ���ܣ���������Python���̵���Ϣ
TArray<FString> Pythonrun::ReceiveMessages()
{
	TArray<FString> Result;
	if (RunState != ERunState::Running) return Result;

	CHAR buffer[4096];  // ���ջ�����
	DWORD bytesRead;
	OVERLAPPED overlapped = { 0 };
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// ѭ����ȡֱ��������
	while (true) {
		if (!ReadFile(
			PipeHandle,
			buffer,
			sizeof(buffer),
			&bytesRead,
			&overlapped))
		{
			// �����ض�����״̬
			if (GetLastError() == ERROR_MORE_DATA) continue;
			if (GetLastError() == ERROR_IO_PENDING) {
				// �ȴ��첽�������
				WaitForSingleObject(overlapped.hEvent, INFINITE);
				GetOverlappedResult(PipeHandle, &overlapped, &bytesRead, FALSE);
			}
			else break; // ����������ֹѭ��
		}

		// ת�����洢��Ч��Ϣ
		if (bytesRead > 0) {
			FString Messagess = FString(bytesRead, UTF8_TO_TCHAR(buffer));
			Result.Add(Messagess); 
		}
		else break; // �����ݿɶ�
	}

	CloseHandle(overlapped.hEvent);
	return Result;
}

//-----------------------------------------------------------------------------
// ʵ������ϵͳ
// ��ƣ�ȫ��ΨһID���䣬�̰߳�ȫ����
//-----------------------------------------------------------------------------
Ptr Pythonrun::GetInstanceById(uint32 id) // ���ܣ�ͨ��ID��ȡʵ�����̰߳�ȫ��
{
	// �Զ�������InstanceMutex�ڵ���������������
	if (!PythonRunList.Contains(id)) {
		return Ptr();
	}
	else 
		return PythonRunList[id];
}

// ���ܣ�����ָ��ID��ʵ������ȷ��IDΨһ��
Pythonrun::Ptr Pythonrun::CreateById(EPriorityLevel InPriority, FString AttributeName, uint32 id, FString InRunPythonExePath)
{
	FScopeLock Lock(&InstanceMutex); // ʵ����������

	if (PythonRunList.Contains(id))
	{
		UE_LOG(LogTemp, Warning, TEXT("Instance ID %d already exists!"), id);
		return nullptr; // ID��ͻ���ؿ�ָ��
	}

	// ������ʵ������ʼ������
	Ptr NewInstance = MakeShared<Pythonrun, ESPMode::ThreadSafe>();
	NewInstance->RunId = id;
	NewInstance->Attribute = AttributeName;
	NewInstance->Priority = InPriority;
	NewInstance->RunPythonExePath = InRunPythonExePath;

	// ע�ᵽȫ���б�����ID������
	PythonRunList.Add(id, NewInstance);
	InstanceCounter = FMath::Max(InstanceCounter, id + 1);

	return NewInstance;
}

// ���ܣ��Զ�����ID������ʵ��
Pythonrun::Ptr Pythonrun::AddRunner(EPriorityLevel InPriority, FString AttributeName, FString InRunPythonExePath)
{
	FScopeLock Lock(&InstanceMutex);

	// ʵ���������޼��
	if (InstanceCounter >= 1000)
	{
		UE_LOG(LogTemp, Error, TEXT("Maximum instance limit reached!"));
		return nullptr;
	}

	// ������ID������ʵ��
	const uint32 NewId = InstanceCounter++;
	return CreateById(InPriority, AttributeName, NewId, InRunPythonExePath);
}

// ���ܣ�����ֹͣ/�����ʵ��
void Pythonrun::ClearNoRunPython()
{
	FScopeLock Lock(&InstanceMutex);

	TArray<uint32> ToRemove;
	// ��һ�׶Σ��ռ���Ҫɾ���ļ�
	for (const auto& Pair : PythonRunList)
	{
		if (Pair.Value->RunState == ERunState::Stopped || // ȷ����Դ�ͷ�
			Pair.Value->RunState == ERunState::Error) // ��¼��ɾ����
		{
			Pair.Value->ForceStop();  // ȷ����Դ�ͷ�
			ToRemove.Add(Pair.Key);
		}
	}

	// �ڶ��׶Σ�����ɾ�������������ʧЧ��
	for (const auto& Key : ToRemove)
	{
		PythonRunList.Remove(Key);
	}
}

//-----------------------------------------------------------------------------
// ���ܣ�����IDǿ��ɾ��ָ��ʵ��
// ������id - Ҫɾ����ʵ��ID
// ע�⣺����ʵ����ǰ״̬��ζ���ǿ����ֹ
//-----------------------------------------------------------------------------
void Pythonrun::DeleteInstanceById(uint32 id)
{
	FScopeLock Lock(&InstanceMutex);  // ��֤�̰߳�ȫ

	if (!PythonRunList.Contains(id))
	{
		UE_LOG(LogTemp, Warning, TEXT("DeleteInstanceById: Instance %d not found!"), id);
		return;
	}

	// ��ȡʵ������
	Ptr Instance = PythonRunList[id];

	// ǿ����ֹ���̣�����������У�
	if (Instance->RunState == ERunState::Running ||
		Instance->RunState == ERunState::Error)
	{
		Instance->ForceStop();  // ȷ����Դ�ͷ�
	}

	// ��ȫ���б��Ƴ�
	PythonRunList.Remove(id);
	UE_LOG(LogTemp, Log, TEXT("Instance %d deleted successfully"), id);
}

//-----------------------------------------------------------------------------
// ���ܣ�ʵʱ�������Ƿ���������
// ���أ�true-�����������У�false-������ֹͣ/�����Ч/���ʧ��
// �ص㣺�̰߳�ȫ��ʵ�ʲ�ѯ����״̬��������RunState����
//-----------------------------------------------------------------------------
bool Pythonrun::GetState() const
{
	FScopeLock Lock(&HandleMutex);  // ʹ�ö����������֤�̰߳�ȫ

	// ��Ч�Լ��
	if (PythonProcessHandle == INVALID_HANDLE_VALUE) {
		return false;
	}

	// ��ѯʵ�ʽ���״̬
	DWORD ExitCode = 0;
	if (!GetExitCodeProcess(PythonProcessHandle, &ExitCode)) {
		DWORD ErrCode = GetLastError();
		// ���⴦����������ֹ����δ�رվ�������
		if (ErrCode == ERROR_INVALID_HANDLE) {
			const_cast<Pythonrun*>(this)->RunState = ERunState::Stopped;
		}
		return false;
	}

	// �жϻ״̬��ͬʱ����RunState��
	const bool bIsActive = (ExitCode == STILL_ACTIVE);
	if (!bIsActive) {
		const_cast<Pythonrun*>(this)->RunState = ERunState::Stopped;
	}
	return bIsActive;
}