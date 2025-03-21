// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DumplingOneFeetFunc.generated.h"

/**
 * 
 */
UCLASS()
class MAKEDUMPLING_API UDumplingOneFeetFunc : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "FileSystem", meta = (DisplayName = "Open Folder Dialog"))
	static FString OpenFolderDialog(const FString& DialogTitle = TEXT("选择文件夹"));

	UFUNCTION(BlueprintCallable, Category = "FileDialog",
		meta = (Tooltip = "打开Windows资源管理器选择文件",
			DefaultToSelf = "WorldContextObject"))
	static TArray<FString> OpenFileDialogWithFilter(
		const FString& DialogTitle = TEXT("选择文件"),
		const FString& FileTypeFilter = TEXT(""));
};
