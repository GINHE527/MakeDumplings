#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "Containers/Queue.h"
#include "LibDumpling.h"

class MAKEDUMPLING_API Pythonrun
{

public:
	// 进程状态枚举
	enum class ERunState : uint8
	{
		Stopped,
		Starting,
		Running,
		Stopping,
		Error
	};

	// 进程优先级枚举
	enum class EPriorityLevel : uint8
	{
		Low,
		Normal,
		High
	};



public:
	// 智能指针类型定义
	using Ptr = TSharedPtr<Pythonrun, ESPMode::ThreadSafe>;
	~Pythonrun() = default; 
	Pythonrun() = default;
	// 基础信息
	FString Message;
	FString Attribute;
	uint32 RunId;
	FString RunPythonExePath;
	ERunState RunState = ERunState::Stopped;

	// 系统资源监控
	float CostRamByMByte = 0.0f;
	float CpuUsagePercent = 0.0f;
	EPriorityLevel Priority = EPriorityLevel::Normal;

	// 进程控制接口
	void Start();
	void ForceStop();
	void SafeStop();
	void Restart();
	void UpdateCpuUsage();

	// 日志系统
	TArray<FString> MessageAll;
	TArray<FString> AsyncMessageQueue;
	void PrintLog();
	void FlushAsyncLogs();
	void AsyncPrintLog();

	// 进程间通信
	TQueue<FString> MessageQueue;
	void SendMessage(const FString& InMessage);
	TArray<FString> ReceiveMessages();

	// 错误处理
	FString LastError;

	// 实例管理系统
	static uint32 InstanceCounter;
	static TMap<uint32, Ptr> PythonRunList;

	static Ptr GetInstanceById(uint32 id);
	static Ptr CreateById(EPriorityLevel InPriority = EPriorityLevel::Normal,
		FString AttributeName = "None",uint32 id = 0, FString InRunPythonExePath = FString());
	static Ptr AddRunner(EPriorityLevel InPriority = EPriorityLevel::Normal,
		FString AttributeName = "None", FString InRunPythonExePath = FString());
	static void ClearNoRunPython();

	// 禁止复制和移动
	Pythonrun(const Pythonrun&) = delete;
	Pythonrun& operator=(const Pythonrun&) = delete;
	Pythonrun(Pythonrun&&) = delete;
	Pythonrun& operator=(Pythonrun&&) = delete;
	
	HANDLE PipeHandle = INVALID_HANDLE_VALUE;
	HANDLE PythonProcessHandle = INVALID_HANDLE_VALUE;
	static FCriticalSection InstanceMutex;
	FCriticalSection LogMutex;


    inline static const FString PythonPath = FString(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + "python/Python/python.exe"));
};