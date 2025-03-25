#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "Containers/Queue.h"
#include "LibDumpling.h"


/**
 * @class Pythonrun
 * @brief Python进程管理器，提供进程控制、资源监控和通信功能
 *
 * 该类封装了Python进程的创建、监控和通信功能，支持优先级控制、状态跟踪、
 * 系统资源监控（内存/CPU）、异步日志系统和进程间通信。使用单例模式管理多个进程实例。
 */
class MAKEDUMPLING_API Pythonrun
{

public:

	///进程状态枚举
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

private:
	// 专用于保护进程句柄的锁
	mutable FCriticalSection HandleMutex;

public:
	// 智能指针类型定义
	using Ptr = TSharedPtr<Pythonrun, ESPMode::ThreadSafe>;
	~Pythonrun() = default; 
	Pythonrun() = default;

	// 基础信息
	FString Message;   ///< 当前状态信息（最新消息）
	FString Attribute;  ///< 进程属性标识名称
	uint32 RunId;       ///< 进程唯一标识ID
	FString RunPythonExePath;  ///< Python解释器路径
	ERunState RunState = ERunState::Stopped;  ///< 当前进程状态

	// 系统资源监控
	float CostRamByMByte = 0.0f;  ///< 内存占用量（MB）
	float CpuUsagePercent = 0.0f; ///< CPU使用率（百分比）
	EPriorityLevel Priority = EPriorityLevel::Normal; ///< 进程优先级级别

	// 进程控制接口
	void Start(); /** 启动Python进程（非阻塞） */
	void ForceStop(); /** 强制终止进程（立即停止） */
	void SafeStop(); /** 安全停止进程（发送终止信号） */
	void Restart(); /** 重启进程 */
	void UpdateCpuUsage(); /** 更新CPU使用率数据 */

	// 日志系统
	TArray<FString> MessageAll; ///< 完整日志记录
	TArray<FString> AsyncMessageQueue; ///< 异步日志缓冲区
	void PrintLog();  /** 同步打印日志到输出设备 */
	void FlushAsyncLogs();  /** 刷新异步日志到主日志 */
	void AsyncPrintLog();  /** 异步线程安全的日志记录 */

	// 进程间通信
	TQueue<FString> MessageQueue; /// 线程安全的消息队列
	void SendMessage(const FString& InMessage); /** 发送消息到进程 */
	TArray<FString> ReceiveMessages(); /** 接收所有待处理消息 */
	bool GetState() const; /** 获取当前运行状态（线程安全） */
	// 错误处理
	FString LastError; /// 最后记录的错误信息

	// 实例管理系统
	static uint32 InstanceCounter;  ///实例计数器
	static TMap<uint32, Ptr> PythonRunList;    ///活动实例映射表

	static Ptr GetInstanceById(uint32 id); /** 通过ID获取实例指针 */
	static Ptr CreateById(EPriorityLevel InPriority = EPriorityLevel::Normal, 
		FString AttributeName = "None",uint32 id = 0, FString InRunPythonExePath = FString());  /** 创建指定ID的实例（需唯一ID） */
	static Ptr AddRunner(EPriorityLevel InPriority = EPriorityLevel::Normal, 
		FString AttributeName = "None", FString InRunPythonExePath = FString()); /** 添加新运行实例（自动分配ID） */
	static void ClearNoRunPython();/** 清理非运行状态实例 */
	static void DeleteInstanceById(uint32 id); /** 删除指定ID的实例 */

	// 禁止复制和移动
	Pythonrun(const Pythonrun&) = delete;
	Pythonrun& operator=(const Pythonrun&) = delete;
	Pythonrun(Pythonrun&&) = delete;
	Pythonrun& operator=(Pythonrun&&) = delete;
	
	///系统资源句柄
	HANDLE PipeHandle = INVALID_HANDLE_VALUE;  ///进程通信管道句柄
	HANDLE PythonProcessHandle = INVALID_HANDLE_VALUE;  ///进程内核对象句柄
	
	///同步对象
	static FCriticalSection InstanceMutex;  /// 实例映射表访问锁
	FCriticalSection LogMutex; /// 日志系统访问锁

	///内置Python解释器路径（Content/python目录）
    inline static const FString PythonPath = FString(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + "python/Python/python.exe"));
};