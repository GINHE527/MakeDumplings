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





void ULibDumpling::Activate() {
	
	bool PythonValue = false;
	//First Cheak python
	if (!GConfig->GetBool(TEXT("/Plugins/MakeDumpling.Settings"), TEXT("PythonLibInstall"), PythonValue, GGameIni)) {
		GConfig->SetBool(TEXT("/Plugins/MakeDumpling.Settings"), TEXT("PythonLibInstall"), false, GGameIni);
		GConfig->Flush(true);
	}
	GConfig->GetBool(TEXT("/Plugins/MakeDumpling.Settings"), TEXT("PythonLibInstall"), PythonValue, GGameIni);
	if (!PythonValue) {
		//fixed;
		processPercent += FMath::RandRange(1.0f, 3.0f);
		OnProcess.Broadcast(processPercent, TEXT("Find the Python Lib Are not fixed,Begin To fixed"));

	}
	processPercent = 29.0f;
	processPercent += FMath::RandRange(1.0f, 3.0f);
	OnProcess.Broadcast(processPercent, TEXT("begin to upload"));

}
