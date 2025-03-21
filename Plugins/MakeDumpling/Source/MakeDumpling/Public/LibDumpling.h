// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Zipperman.h"
#include "LibDumpling.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDumplingSuccess, FString ,Mesg);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDumplingProcess, float , percent , FString ,message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDumplingFailied, FString ,ErrorMesg);
/**
 * 
 */
UCLASS()
class MAKEDUMPLING_API ULibDumpling : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
	UFUNCTION(BlueprintCallable, meta =(BlueprintInternalUseOnly = "true", DisplayName = "MakeDumpling打包",Keywords = "Package"),Category = "MakeDumpling")
	static ULibDumpling* BeginPack(
	UPARAM(meta =(DisplayName = "需要打包的程序的绝对地址")) const FString &NeedPakedPath,
	UPARAM(meta =(DisplayName = "打包位置")) const FString &AimPakedPath,
	UPARAM(meta =(DisplayName = "打包后的名称" )) const FString &PakedName,
	UPARAM(meta = (DisplayName = "打包后的图标地址(带上文件)"))const FString &PakedImgPath
	);
	UPROPERTY(BlueprintAssignable, meta = (DisplayName = "OnProcess"))
	FDumplingProcess OnProcess;

	UPROPERTY(BlueprintAssignable , meta = (DisplayName = "OnSuccess"))
	FDumplingSuccess OnSuccess;
	
	UPROPERTY(BlueprintAssignable , meta = (DisplayName = "OnFailed"))
	FDumplingFailied OnFailed;
	
	virtual void Activate() override;

	float processPercent;

	FString LogMess;

private:
	TSharedPtr<Zipperman> Zipper;
	FString CurrentMessage;


};


