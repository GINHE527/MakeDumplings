// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Zipperman.h"
#include "LibDumpling.generated.h"
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDumplingSuccess, FString ,Mesg);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDumplingProcess, float , percent , FString ,message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDumplingFailied, FString ,ErrorMesg);
DECLARE_DYNAMIC_DELEGATE_OneParam(FFixedState, bool ,fixstate);
DECLARE_DYNAMIC_DELEGATE(FProcessing);
/**
 * 
 */
UCLASS(BlueprintType)
class MAKEDUMPLING_API ULibDumpling : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
	UFUNCTION(BlueprintCallable, meta =( DisplayName = "MakeDumpling打包",Keywords = "Package"),Category = "MakeDumpling")
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
	

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "MakeDumpling打包修复", Keywords = "Package"), Category = "MakeDumpling")
	static void FixPyRunEnv(UPARAM(meta = (DisplayName = "修复信息")) TArray<FString> &Mesg,
		 UPARAM(meta = (DisplayName = "修复回调")) FFixedState FixState
	);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "测试函数", Keywords = "Package" ,WorldContext = "WorldContextObject"), Category = "MakeDumpling")

	static void TestFunc(
		UObject* WorldContextObject, // 新增 World Context 参数
		UPARAM(meta = (DisplayName = "信息")) FString& Mesg,
		UPARAM(meta = (DisplayName = "回调")) FFixedState FixState,
		UPARAM(meta = (DisplayName = "进程信息")) const FProcessing processing
	);
	virtual void Activate() override;

	float processPercent;

	FString LogMess;

private:
	TSharedPtr<Zipperman> Zipper;
	FString CurrentMessage;


};


