#pragma once

#include "CoreMinimal.h"
#include "Templates/SharedPointer.h"
#include "Containers/Queue.h"
#include "LibDumpling.h"


/**
 * @class Pythonrun
 * @brief Python���̹��������ṩ���̿��ơ���Դ��غ�ͨ�Ź���
 *
 * �����װ��Python���̵Ĵ�������غ�ͨ�Ź��ܣ�֧�����ȼ����ơ�״̬���١�
 * ϵͳ��Դ��أ��ڴ�/CPU�����첽��־ϵͳ�ͽ��̼�ͨ�š�ʹ�õ���ģʽ����������ʵ����
 */
class MAKEDUMPLING_API Pythonrun
{

public:

	///����״̬ö��
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

private:
	// ר���ڱ������̾������
	mutable FCriticalSection HandleMutex;

public:
	// ����ָ�����Ͷ���
	using Ptr = TSharedPtr<Pythonrun, ESPMode::ThreadSafe>;
	~Pythonrun() = default; 
	Pythonrun() = default;

	// ������Ϣ
	FString Message;   ///< ��ǰ״̬��Ϣ��������Ϣ��
	FString Attribute;  ///< �������Ա�ʶ����
	uint32 RunId;       ///< ����Ψһ��ʶID
	FString RunPythonExePath;  ///< Python������·��
	ERunState RunState = ERunState::Stopped;  ///< ��ǰ����״̬

	// ϵͳ��Դ���
	float CostRamByMByte = 0.0f;  ///< �ڴ�ռ������MB��
	float CpuUsagePercent = 0.0f; ///< CPUʹ���ʣ��ٷֱȣ�
	EPriorityLevel Priority = EPriorityLevel::Normal; ///< �������ȼ�����

	// ���̿��ƽӿ�
	void Start(); /** ����Python���̣��������� */
	void ForceStop(); /** ǿ����ֹ���̣�����ֹͣ�� */
	void SafeStop(); /** ��ȫֹͣ���̣�������ֹ�źţ� */
	void Restart(); /** �������� */
	void UpdateCpuUsage(); /** ����CPUʹ�������� */

	// ��־ϵͳ
	TArray<FString> MessageAll; ///< ������־��¼
	TArray<FString> AsyncMessageQueue; ///< �첽��־������
	void PrintLog();  /** ͬ����ӡ��־������豸 */
	void FlushAsyncLogs();  /** ˢ���첽��־������־ */
	void AsyncPrintLog();  /** �첽�̰߳�ȫ����־��¼ */

	// ���̼�ͨ��
	TQueue<FString> MessageQueue; /// �̰߳�ȫ����Ϣ����
	void SendMessage(const FString& InMessage); /** ������Ϣ������ */
	TArray<FString> ReceiveMessages(); /** �������д�������Ϣ */
	bool GetState() const; /** ��ȡ��ǰ����״̬���̰߳�ȫ�� */
	// ������
	FString LastError; /// ����¼�Ĵ�����Ϣ

	// ʵ������ϵͳ
	static uint32 InstanceCounter;  ///ʵ��������
	static TMap<uint32, Ptr> PythonRunList;    ///�ʵ��ӳ���

	static Ptr GetInstanceById(uint32 id); /** ͨ��ID��ȡʵ��ָ�� */
	static Ptr CreateById(EPriorityLevel InPriority = EPriorityLevel::Normal, 
		FString AttributeName = "None",uint32 id = 0, FString InRunPythonExePath = FString());  /** ����ָ��ID��ʵ������ΨһID�� */
	static Ptr AddRunner(EPriorityLevel InPriority = EPriorityLevel::Normal, 
		FString AttributeName = "None", FString InRunPythonExePath = FString()); /** ���������ʵ�����Զ�����ID�� */
	static void ClearNoRunPython();/** ���������״̬ʵ�� */
	static void DeleteInstanceById(uint32 id); /** ɾ��ָ��ID��ʵ�� */

	// ��ֹ���ƺ��ƶ�
	Pythonrun(const Pythonrun&) = delete;
	Pythonrun& operator=(const Pythonrun&) = delete;
	Pythonrun(Pythonrun&&) = delete;
	Pythonrun& operator=(Pythonrun&&) = delete;
	
	///ϵͳ��Դ���
	HANDLE PipeHandle = INVALID_HANDLE_VALUE;  ///����ͨ�Źܵ����
	HANDLE PythonProcessHandle = INVALID_HANDLE_VALUE;  ///�����ں˶�����
	
	///ͬ������
	static FCriticalSection InstanceMutex;  /// ʵ��ӳ��������
	FCriticalSection LogMutex; /// ��־ϵͳ������

	///����Python������·����Content/pythonĿ¼��
    inline static const FString PythonPath = FString(FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + "python/Python/python.exe"));
};