// Copyright GINHE, Inc. All Rights Reserved.


#include "Pythonrun.h"

//Pyrun只能指针
using Ptr = TSharedPtr<Pythonrun, ESPMode::ThreadSafe>;

// 实例ID生成器
uint32 Pythonrun::InstanceCounter = 0;
// 实例存储容器
TMap<uint32, Pythonrun::Ptr> Pythonrun::PythonRunList;
// 实例操作的线程锁
FCriticalSection Pythonrun::InstanceMutex;

//-----------------------------------------------------------------------------
// 功能：启动Python进程和通信管道
// 注意：仅在Stopped状态有效，Error状态需要手动重置
//-----------------------------------------------------------------------------
void Pythonrun::Start()
{
	// 状态校验：仅允许从停止状态启动
	if (RunState != ERunState::Stopped) return;

	//管道名称格式：\\.\pipe\PythonRunPipe_<RunId>
	FString PipeName = FString::Printf(TEXT("\\\\.\\pipe\\UnrealPythonRunPipe_%d"), RunId);

	// 创建异步命名管道（Windows API）
	PipeHandle = CreateNamedPipe(
		*PipeName, // 管道名称
		PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,  // 双工通信+异步模式
		PIPE_TYPE_MESSAGE | // 消息类型管道
		PIPE_READMODE_MESSAGE | // 消息读取模式
		PIPE_WAIT, // 阻塞等待模式
		1,  // 单实例管道
		4096,  // 输出缓冲区
		4096,  // 输入缓冲区
		0, // 默认超时时间（50ms）
		NULL  // 安全属性（默认）
	);

	// 错误处理：记录错误并更新状态
	if (PipeHandle == INVALID_HANDLE_VALUE) {
		LastError = FString::Printf(TEXT("CreateNamedPipe failed (%d)"), GetLastError());
		RunState = ERunState::Error;
		return;
	}

	//  /* 第二阶段：启动Python子进程 */
	STARTUPINFO si = { sizeof(si) }; // 进程启动参数结构体
	PROCESS_INFORMATION pi;  // 进程信息结构体

	// 构造命令行：确保路径带引号处理空格
	FString CommandLine = FString::Printf(TEXT("\"%s\" \"%s\""), *PythonPath, *RunPythonExePath);

	// 创建新进程（不显示控制台窗口）
	if (!CreateProcess(
		NULL, // 创建新进程（显示控制台窗口）
		const_cast<LPWSTR>(*CommandLine),  // 命令行参数
		NULL, // 进程安全属性
		NULL,  // 线程安全属性
		FALSE, // 不继承句柄
		0, // 不创建新控制台窗口
		NULL,
		NULL,
		&si,
		&pi))
	{
		// 错误处理：关闭管道并记录错误
		LastError = FString::Printf(TEXT("CreateProcess failed (%d)"), GetLastError());
		CloseHandle(PipeHandle);
		RunState = ERunState::Error;
		return;
	}
	// 保存进程句柄并清理线程句柄
	PythonProcessHandle = pi.hProcess;
	CloseHandle(pi.hThread); // 不需要线程控制

	// 更新状态为运行中
	RunState = ERunState::Running;
}

//-----------------------------------------------------------------------------
// 功能：强制终止Python进程并清理资源
// 注意：立即终止可能导致数据丢失，优先使用SafeStop
//-----------------------------------------------------------------------------
void Pythonrun::ForceStop()
{
	// 终止Python进程
	if (PythonProcessHandle != INVALID_HANDLE_VALUE) {
		TerminateProcess(PythonProcessHandle, 0); // 强制退出代码0
		CloseHandle(PythonProcessHandle);
		PythonProcessHandle = INVALID_HANDLE_VALUE;
	}

	// 关闭管道连接
	if (PipeHandle != INVALID_HANDLE_VALUE) {
		DisconnectNamedPipe(PipeHandle); // 断开客户端连接
		CloseHandle(PipeHandle);
		PipeHandle = INVALID_HANDLE_VALUE;
	}
	// 更新状态为已停止
	RunState = ERunState::Stopped;
}

//-----------------------------------------------------------------------------
// 功能：安全停止Python进程（建议使用）
// 流程：1.发送退出指令 2.等待正常退出 3.超时强制终止
//-----------------------------------------------------------------------------
void Pythonrun::SafeStop()
{
	if (RunState != ERunState::Running) return;

	// 步骤1：通过管道发送退出指令
	SendMessage(TEXT("EXIT"));

	// 步骤2：等待进程退出（5秒超时）
	if (WaitForSingleObject(PythonProcessHandle, 5000) == WAIT_TIMEOUT) {
		ForceStop(); // 超时后强制终止
	}
	else {
		// 正常关闭流程
		CloseHandle(PythonProcessHandle);
		PythonProcessHandle = INVALID_HANDLE_VALUE;
		RunState = ERunState::Stopped;
	}
}

//-----------------------------------------------------------------------------
// 功能：重启Python进程
// 注意：需先成功停止当前实例
//-----------------------------------------------------------------------------
void Pythonrun::Restart()
{
	SafeStop();  // 尝试安全停止

	// 确认进入停止状态后重启
	if (RunState == ERunState::Stopped)
	{
		// 重置管道和进程句柄
		PipeHandle = INVALID_HANDLE_VALUE;
		PythonProcessHandle = INVALID_HANDLE_VALUE;
		Start();  // 重新启动
	}
}

//-----------------------------------------------------------------------------
// 功能：更新进程CPU使用率
// 原理：通过进程时间计算CPU占用百分比
//-----------------------------------------------------------------------------
void Pythonrun::UpdateCpuUsage()
{
	if (PythonProcessHandle == INVALID_HANDLE_VALUE) return;

	FILETIME createTime, exitTime, kernelTime, userTime;
	if (GetProcessTimes(PythonProcessHandle, &createTime, &exitTime, &kernelTime, &userTime)) {
		// 将FILETIME转换为64位整型
		ULARGE_INTEGER kernel = { { kernelTime.dwLowDateTime, kernelTime.dwHighDateTime } };
		ULARGE_INTEGER user = { { userTime.dwLowDateTime, userTime.dwHighDateTime } };

		// 计算总CPU时间并转换为百分比（假设1秒间隔）
		CpuUsagePercent = (kernel.QuadPart + user.QuadPart) / 10000.0f;
	}
}

//-----------------------------------------------------------------------------
// 日志系统实现
// 设计：异步消息队列保证线程安全，主日志存储完整记录
//-----------------------------------------------------------------------------

// 功能：将异步日志输出到引擎日志系统
void Pythonrun::PrintLog()
{
	// 加锁保证线程安全
	FScopeLock Lock(&LogMutex);
	// 遍历异步队列，输出并转移至主日志
	for (const auto& Msg : AsyncMessageQueue)
	{
		UE_LOG(LogTemp, Display, TEXT("[PythonRun %d] %s"), RunId, *Msg);
		MessageAll.Add(Msg); // 归档到主日志
	}
	AsyncMessageQueue.Empty(); // 清空临时队列
}

// 功能：合并异步日志到主日志（不输出）
void Pythonrun::FlushAsyncLogs()
{
	FScopeLock Lock(&LogMutex);
	MessageAll.Append(AsyncMessageQueue); // 直接追加
	AsyncMessageQueue.Empty();
}

// 功能：接收新消息到异步队列
void Pythonrun::AsyncPrintLog()
{
	TArray<FString> NewMessages = ReceiveMessages(); // 从管道读取
	FScopeLock Lock(&LogMutex);
	AsyncMessageQueue.Append(NewMessages); // 追加到临时队列
}

//-----------------------------------------------------------------------------
// 进程间通信方法
// 注意：使用异步IO操作避免阻塞主线程
//-----------------------------------------------------------------------------
void Pythonrun::SendMessage(const FString& InMessage)
{
	if (RunState != ERunState::Running) return;

	DWORD bytesWritten;
	OVERLAPPED overlapped = { 0 };
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// 异步写入
	if (!WriteFile(
		PipeHandle, // 管道句柄
		*InMessage, // 消息内容
		InMessage.Len() * sizeof(TCHAR),  // 字节长度
		&bytesWritten, // 实际写入字节数
		&overlapped))  // 异步操作结构
	{
		// 错误处理（忽略IO挂起状态）
		if (GetLastError() != ERROR_IO_PENDING) {
			LastError = FString::Printf(TEXT("WriteFile failed (%d)"), GetLastError());
			RunState = ERunState::Error;
		}
	}

	// 等待写入完成
	WaitForSingleObject(overlapped.hEvent, INFINITE);
	CloseHandle(overlapped.hEvent);
}

// 功能：接收来自Python进程的消息
TArray<FString> Pythonrun::ReceiveMessages()
{
	TArray<FString> Result;
	if (RunState != ERunState::Running) return Result;

	CHAR buffer[4096];  // 接收缓冲区
	DWORD bytesRead;
	OVERLAPPED overlapped = { 0 };
	overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// 循环读取直到无数据
	while (true) {
		if (!ReadFile(
			PipeHandle,
			buffer,
			sizeof(buffer),
			&bytesRead,
			&overlapped))
		{
			// 处理特定错误状态
			if (GetLastError() == ERROR_MORE_DATA) continue;
			if (GetLastError() == ERROR_IO_PENDING) {
				// 等待异步操作完成
				WaitForSingleObject(overlapped.hEvent, INFINITE);
				GetOverlappedResult(PipeHandle, &overlapped, &bytesRead, FALSE);
			}
			else break; // 其他错误终止循环
		}

		// 转换并存储有效消息
		if (bytesRead > 0) {
			FString Messagess = FString(bytesRead, UTF8_TO_TCHAR(buffer));
			Result.Add(Messagess); 
		}
		else break; // 无数据可读
	}

	CloseHandle(overlapped.hEvent);
	return Result;
}

//-----------------------------------------------------------------------------
// 实例管理系统
// 设计：全局唯一ID分配，线程安全访问
//-----------------------------------------------------------------------------
Ptr Pythonrun::GetInstanceById(uint32 id) // 功能：通过ID获取实例（线程安全）
{
	// 自动加锁（InstanceMutex在调用链中已锁定）
	if (!PythonRunList.Contains(id)) {
		return Ptr();
	}
	else 
		return PythonRunList[id];
}

// 功能：创建指定ID的实例（需确保ID唯一）
Pythonrun::Ptr Pythonrun::CreateById(EPriorityLevel InPriority, FString AttributeName, uint32 id, FString InRunPythonExePath)
{
	FScopeLock Lock(&InstanceMutex); // 实例操作加锁

	if (PythonRunList.Contains(id))
	{
		UE_LOG(LogTemp, Warning, TEXT("Instance ID %d already exists!"), id);
		return nullptr; // ID冲突返回空指针
	}

	// 创建新实例并初始化属性
	Ptr NewInstance = MakeShared<Pythonrun, ESPMode::ThreadSafe>();
	NewInstance->RunId = id;
	NewInstance->Attribute = AttributeName;
	NewInstance->Priority = InPriority;
	NewInstance->RunPythonExePath = InRunPythonExePath;

	// 注册到全局列表并更新ID计数器
	PythonRunList.Add(id, NewInstance);
	InstanceCounter = FMath::Max(InstanceCounter, id + 1);

	return NewInstance;
}

// 功能：自动生成ID创建新实例
Pythonrun::Ptr Pythonrun::AddRunner(EPriorityLevel InPriority, FString AttributeName, FString InRunPythonExePath)
{
	FScopeLock Lock(&InstanceMutex);

	// 实例数量上限检查
	if (InstanceCounter >= 1000)
	{
		UE_LOG(LogTemp, Error, TEXT("Maximum instance limit reached!"));
		return nullptr;
	}

	// 分配新ID并创建实例
	const uint32 NewId = InstanceCounter++;
	return CreateById(InPriority, AttributeName, NewId, InRunPythonExePath);
}

// 功能：清理停止/错误的实例
void Pythonrun::ClearNoRunPython()
{
	FScopeLock Lock(&InstanceMutex);

	TArray<uint32> ToRemove;
	// 第一阶段：收集需要删除的键
	for (const auto& Pair : PythonRunList)
	{
		if (Pair.Value->RunState == ERunState::Stopped || // 确保资源释放
			Pair.Value->RunState == ERunState::Error) // 记录待删除键
		{
			Pair.Value->ForceStop();  // 确保资源释放
			ToRemove.Add(Pair.Key);
		}
	}

	// 第二阶段：批量删除（避免迭代器失效）
	for (const auto& Key : ToRemove)
	{
		PythonRunList.Remove(Key);
	}
}

//-----------------------------------------------------------------------------
// 功能：根据ID强制删除指定实例
// 参数：id - 要删除的实例ID
// 注意：无论实例当前状态如何都会强制终止
//-----------------------------------------------------------------------------
void Pythonrun::DeleteInstanceById(uint32 id)
{
	FScopeLock Lock(&InstanceMutex);  // 保证线程安全

	if (!PythonRunList.Contains(id))
	{
		UE_LOG(LogTemp, Warning, TEXT("DeleteInstanceById: Instance %d not found!"), id);
		return;
	}

	// 获取实例引用
	Ptr Instance = PythonRunList[id];

	// 强制终止进程（如果仍在运行）
	if (Instance->RunState == ERunState::Running ||
		Instance->RunState == ERunState::Error)
	{
		Instance->ForceStop();  // 确保资源释放
	}

	// 从全局列表移除
	PythonRunList.Remove(id);
	UE_LOG(LogTemp, Log, TEXT("Instance %d deleted successfully"), id);
}

//-----------------------------------------------------------------------------
// 功能：实时检测进程是否正在运行
// 返回：true-进程正常运行，false-进程已停止/句柄无效/检测失败
// 特点：线程安全，实际查询进程状态而非依赖RunState缓存
//-----------------------------------------------------------------------------
bool Pythonrun::GetState() const
{
	FScopeLock Lock(&HandleMutex);  // 使用独立句柄锁保证线程安全

	// 有效性检查
	if (PythonProcessHandle == INVALID_HANDLE_VALUE) {
		return false;
	}

	// 查询实际进程状态
	DWORD ExitCode = 0;
	if (!GetExitCodeProcess(PythonProcessHandle, &ExitCode)) {
		DWORD ErrCode = GetLastError();
		// 特殊处理：进程已终止但尚未关闭句柄的情况
		if (ErrCode == ERROR_INVALID_HANDLE) {
			const_cast<Pythonrun*>(this)->RunState = ERunState::Stopped;
		}
		return false;
	}

	// 判断活动状态（同时更新RunState）
	const bool bIsActive = (ExitCode == STILL_ACTIVE);
	if (!bIsActive) {
		const_cast<Pythonrun*>(this)->RunState = ERunState::Stopped;
	}
	return bIsActive;
}