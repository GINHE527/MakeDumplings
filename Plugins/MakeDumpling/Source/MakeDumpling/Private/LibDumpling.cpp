// Fill out your copyright notice in the Description page of Project Settings.


#include "LibDumpling.h"
#include "Math/UnrealMathUtility.h" 
#include "Editor.h"
#include "TimerManager.h"
// #include "Kismet"
#include "Misc/ConfigCacheIni.h"
#include "UObject/StrongObjectPtr.h"
#include "Pythonrun.h"




ULibDumpling* ULibDumpling::BeginPack(const FString& NeedPakedPath, const FString& AimPakedPath, const FString& PakedName, const FString& PakedImgPath) {
	
	// 创建实例
	ULibDumpling* Instance = NewObject<ULibDumpling>();
	
	Instance->NeedPakedPath = NeedPakedPath ;
	Instance->AimPakedPath = AimPakedPath;
	Instance->PakedName = PakedName;
	Instance->PakedImgPath = PakedImgPath;
	
	return Instance;
	
}


void ULibDumpling::FixPyRunEnv(TArray<FString>& Mesg , FFixedState FixState)
{
	TArray<FString>ReciveMesg;
	ReciveMesg.Add(TEXT("Fixed process Init..."));
	Mesg = ReciveMesg;
	FixState.ExecuteIfBound(true);
	return;
}


void ULibDumpling::TestFunc(UObject* WorldContextObject, FString& Mesg, FFixedState FixState, const FProcessing Processing)
{
	Mesg = "";
}


// --------------------------------@@@[Hebness]---------------------------------------
void ULibDumpling::UseExetoPak()
{
	//如果需要用EXE去打开 位于 {project_cotnet} python\Pypak\OnePackedTool.exe  -> EXE打包程序
	//需要将位于 {project_cotnet} python\Pypak\ 新增一个pak文件夹 (若未有)
	//将项目文件压缩 命名为 HTML5LaunchHelper.zip 移动至 pak文件夹 中
	//随后UE运行 EXE打包程序 监控其状态(打包完会自动关闭)
	//成功后会出现一个{project_cotnet} python\Pypak\packed 文件夹 里面的 HTML5LaunchHelper.exe 就是打包后内容
	//重命名移动即可实现打包成功了

	//exe测试链路 如果要验证虚幻以外是否正常运行，直接 {project_cotnet} python\Pypak\ 新增一个pak文件夹 后
	//将项目文件压缩 命名为 HTML5LaunchHelper.zip 移动至 pak文件夹 中 再双击  位于 {project_cotnet} python\Pypak\OnePackedTool.exe
	//它会自动修复并打包 成功后会出现一个{project_cotnet} python\Pypak\packed 文件夹 里面的 HTML5LaunchHelper.exe 就是打包后内容
	//win11 5.6 UE 已经测试成功
}
// --------------------------------@@@[Hebness]  end-----------------------------------


void ULibDumpling::Activate() {
	//@param is the new addd
	const FString zip_path = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + "python/PypakHtml/zip");
	const FString final_app_path = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + "python/PypakHtml/FinalApp.exe");
	const FString fix_py_path = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + "python/pythonFixed.py");
	FString NeedRunPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() + "python/PypakHtml/Dumpling.py");
	
	
	const FString m_NeedPakedPath = this->NeedPakedPath;
	const FString m_AimPakedPath = this->AimPakedPath;
	const FString m_PakedName = this->PakedName;
	const FString m_PakedImgPath = this->PakedImgPath;

	bool PythonValue = false;
	//First Cheak python
	if (!GConfig->GetBool(TEXT("/Plugins/MakeDumpling.Settings"), TEXT("PythonLibInstall"), PythonValue, GGameIni)) {
		GConfig->SetBool(TEXT("/Plugins/MakeDumpling.Settings"), TEXT("PythonLibInstall"), false, GGameIni);
		GConfig->Flush(true);
	}
		GConfig->GetBool(TEXT("/Plugins/MakeDumpling.Settings"), TEXT("PythonLibInstall"), PythonValue, GGameIni);

	//move m_NeedPakedPath(has exe) to zip_path(no exe)
	
	Zipperman *zipper = new Zipperman();
	
	const FString Name = "HTML5LaunchHelper.zip";
	processPercent = 3.0f;
	OnProcess.Broadcast(processPercent, TEXT("zip begin"));
	zipper->ToZippedd(m_NeedPakedPath, zip_path, Name);
	
	processPercent = 15.0f;
	OnProcess.Broadcast(processPercent, TEXT("zip end"));
	

	
if (!PythonValue) {
	//fixed;
	processPercent += FMath::RandRange(1.0f, 3.0f);
	OnProcess.Broadcast(processPercent, TEXT("Find the Python Lib Are not fixed,Begin To fixed"));


	TWeakPtr<Pythonrun> CurrentRunner;
	
	
	Pythonrun::Ptr runner = Pythonrun::AddRunner(
		Pythonrun::EPriorityLevel::Normal,
		"DataProcessor", fix_py_path
	);

	if (runner.IsValid())
	{
		runner->Start();

		// 检查状态
		if (runner->RunState == Pythonrun::ERunState::Running)
		{
			if (processPercent < 28.0f)
			{
				UE_LOG(LogTemp, Warning, TEXT("hebness 1 ！"));
				processPercent += FMath::RandRange(0.01f, 0.03f);
				OnProcess.Broadcast(processPercent, TEXT("pythonfix"));
			}
		}
	}
	GConfig->SetBool(TEXT("/Plugins/MakeDumpling.Settings"), TEXT("PythonLibInstall"), true, GGameIni);
	GConfig->Flush(true);
}

	
	processPercent = 29.0f;
	processPercent += FMath::RandRange(1.0f, 3.0f);
	OnProcess.Broadcast(processPercent, TEXT("begin to upload"));

		OnProcess.Broadcast(processPercent, TEXT("begin to upload"));
	Pythonrun::Ptr runner_pak = Pythonrun::AddRunner(
		Pythonrun::EPriorityLevel::Normal,
		"DataProcessor", NeedRunPath
	);
	if (runner_pak.IsValid())
	{
		runner_pak->Start();

		// 检查状态 
		while (runner_pak->RunState == Pythonrun::ERunState::Running)
		{
			if (processPercent < 65.0f)
			{
				UE_LOG(LogTemp, Warning, TEXT("hebness 2 ！"));
				processPercent += FMath::RandRange(0.01f, 0.03f);
				OnProcess.Broadcast(processPercent, TEXT("upload"));
			}
			runner_pak->GetState();
		}
	}

	processPercent = 70.0f;
	processPercent += FMath::RandRange(1.0f, 3.0f);
	OnProcess.Broadcast(processPercent, TEXT("upload"));
	
	
	FString TargetDir = FPaths::ConvertRelativePathToFull(m_AimPakedPath);
	FPaths::NormalizeDirectoryName(TargetDir); 
	FString TargetPath = FPaths::Combine(TargetDir, m_PakedName + ".exe"); 
	
	
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*TargetDir)) {
		PlatformFile.CreateDirectoryTree(*TargetDir);
	}
	
	IFileManager& FileManager = IFileManager::Get();
	
	if (FileManager.FileExists(*TargetPath)) {
		FileManager.Delete(*TargetPath);
	}
	
	
	if (FileManager.Copy(*TargetPath, *final_app_path)) {
		FileManager.Delete(*final_app_path); 
		UE_LOG(LogTemp, Display, TEXT("文件已移动至：%s"), *TargetPath);
		OnSuccess.Broadcast(TargetPath);
	
	}
	else {
		UE_LOG(LogTemp, Error, TEXT("文件移动失败！"));
		OnFailed.Broadcast(TargetPath);
	}

	processPercent = 100.0f;
	OnProcess.Broadcast(processPercent, TEXT("complete"));
	
}

