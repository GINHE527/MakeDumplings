// DumplingOneFeetFunc.cpp
#include "DumplingOneFeetFunc.h"
#include "DesktopPlatformModule.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"
#if PLATFORM_WINDOWS // 确保Windows平台限定
#if WITH_EDITOR
FString UDumplingOneFeetFunc::OpenFolderDialog(const FString& DialogTitle)
{
	FString SelectedPath = "";
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

	if (DesktopPlatform) {
		// 关键参数设置
		FString DefaultPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
		uint32 DialogFlags = EFileDialogFlags::None;

		// 调用文件夹选择对话框 [3](@ref)
		DesktopPlatform->OpenDirectoryDialog(
			nullptr,
			DialogTitle,
			DefaultPath,
			SelectedPath
		);
	}
	return SelectedPath;
}

TArray<FString> UDumplingOneFeetFunc::OpenFileDialogWithFilter(const FString& DialogTitle, const FString& FileTypeFilter)
{
	TArray<FString> AbsoluteOpenFileNames;//获取的文件绝对路径
	FString ExtensionStr = TEXT("*.*");//文件类型

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	DesktopPlatform->OpenFileDialog(nullptr, TEXT("文件管理器"), FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()), TEXT(""), *ExtensionStr, EFileDialogFlags::None, AbsoluteOpenFileNames);
	return AbsoluteOpenFileNames;
}




#endif
#endif