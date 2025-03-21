#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "Containers/Queue.h"
#include "LibDumpling.h"

class MAKEDUMPLING_API Pythonrun
{

public:
	// ����״̬ö��
	enum class ERunState : uint8
	{
		Stopped,
		Starting,
		Running,
		Stopping,
		Error
	};

	// �������ȼ�ö��
	enum class EPriorityLevel : uint8
	{
		Low,
		Normal,
		High
	};



public:
	// ����ָ�����Ͷ���
	using Ptr = TSharedPtr<Pythonrun, ESPMode::ThreadSafe>;
	~Pythonrun() = default; 
	Pythonrun() = default;
	// ������Ϣ
	FString Message;
	FString Attribute;
	uint32 RunId;
	FString RunPythonExePath;
	ERunState RunState = ERunState::Stopped;

	// ϵͳ��Դ���
	float CostRamByMByte = 0.0f;
	float CpuUsagePercent = 0.0f;
	EPriorityLevel Priority = EPriorityLevel::Normal;

	// ���̿��ƽӿ�
	void Start();
	void ForceStop();
	void SafeStop();
	void Restart();
	void UpdateCpuUsage();

	// ��־ϵͳ
	TArray<FString> MessageAll;
	TArray<FString> AsyncMessageQueue;
	void PrintLog();
	void FlushAsyncLogs();
	void AsyncPrintLog();

	// ���̼�ͨ��
	TQueue<FString> MessageQueue;
	void SendMessage(const FString& InMessage);
	TArray<FString> ReceiveMessages();

	// ������
	FString LastError;

	// ʵ������ϵͳ
	static uint32 InstanceCounter;
	static TMap<uint32, Ptr> PythonRunList;

	static Ptr GetInstanceById(uint32 id);
	static Ptr CreateById(EPriorityLevel InPriority = EPriorityLevel::Normal,
		FString AttributeName = "None",uint32 id = 0, FString InRunPythonExePath = FString());
	static Ptr AddRunner(EPriorityLevel InPriority = EPriorityLevel::Normal,
		FString AttributeName = "None", FString InRunPythonExePath = FString());
	static void ClearNoRunPython();

	// ��ֹ���ƺ��ƶ�
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