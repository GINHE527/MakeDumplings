// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
//Copyright Ginhe, Inc.All Rights Reserved.

#pragma once
#define ZLIB_WINAPI
#include "CoreMinimal.h"
#include <string>
#include <Shlwapi.h> 
#include "zip.h"
#include "unzip.h"
#include "zlib.h"

#pragma comment(lib, "Shlwapi.lib")
//#pragma comment(lib, "zlibstat.lib")

/**
 * 
 */
class MAKEDUMPLING_API Zipperman : public FRunnable
{
public:

	Zipperman();
	virtual ~Zipperman();

	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;

	static bool ToZippedd(const FString& InFolderPath, const FString& OutFolderPath, const FString& ProjectName);

	FString TheInFolderPath;
	FString TheOutFolderPath;
	FString TheProjectName;

};

