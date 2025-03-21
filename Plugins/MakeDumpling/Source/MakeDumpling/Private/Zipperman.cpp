// Fill out your copyright notice in the Description page of Project Settings.


#include "Zipperman.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformProcess.h"
#define PTRLOG(contentlog) UE_LOG(LogTemp,Log,TEXT(contentlog)) 


bool nyAddfiletoZip(zipFile zfile, const std::string& fileNameinZip, const std::string& srcfile)
{
	// 目录如果为空则直接返回
	if (NULL == zfile || fileNameinZip.empty())
	{
		return 0;
	}

	int nErr = 0;
	zip_fileinfo zinfo = { 0 };
	tm_zip tmz = { 0 };
	zinfo.tmz_date = tmz;
	zinfo.dosDate = 0;
	zinfo.internal_fa = 0;
	zinfo.external_fa = 0;

	char sznewfileName[MAX_PATH] = { 0 };
	memset(sznewfileName, 0x00, sizeof(sznewfileName));
	strcat_s(sznewfileName, fileNameinZip.c_str());
	if (srcfile.empty())
	{
		strcat_s(sznewfileName, "\\");
	}

	nErr = zipOpenNewFileInZip(zfile, sznewfileName, &zinfo, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION);
	if (nErr != ZIP_OK)
	{
		return false;
	}
	if (!srcfile.empty())
	{
		// 打开源文件
		FILE* srcfp = _fsopen(srcfile.c_str(), "rb", _SH_DENYNO);
		if (NULL == srcfp)
		{
			PTRLOG("Cant open source file(ignore when compressing document )");
			return false;
		}

		// 读入源文件写入zip文件
		int numBytes = 0;
		char* pBuf = new char[1024 * 100];
		if (NULL == pBuf)
		{
			PTRLOG("cant create temp zone");
			return 0;
		}
		while (!feof(srcfp))
		{
			memset(pBuf, 0x00, sizeof(pBuf));
			numBytes = fread(pBuf, 1, sizeof(pBuf), srcfp);
			nErr = zipWriteInFileInZip(zfile, pBuf, numBytes);
			if (ferror(srcfp))
			{
				break;
			}
		}
		delete[] pBuf;
		fclose(srcfp);
	}
	zipCloseFileInZip(zfile);

	return true;
}

bool nyCollectfileInDirtoZip(zipFile zfile, const std::string& filepath, const std::string& parentdirName)
{
	if (NULL == zfile || filepath.empty())
	{
		return false;
	}
	bool bFile = false;
	std::string relativepath = "";
	WIN32_FIND_DATAA findFileData;

	char szpath[MAX_PATH] = { 0 };
	if (::PathIsDirectoryA(filepath.c_str()))
	{
		strcpy_s(szpath, sizeof(szpath) / sizeof(szpath[0]), filepath.c_str());
		int len = strlen(szpath) + strlen("\\*.*") + 1;
		strcat_s(szpath, len, "\\*.*");
	}
	else
	{
		bFile = true;
		strcpy_s(szpath, sizeof(szpath) / sizeof(szpath[0]), filepath.c_str());
	}

	HANDLE hFile = ::FindFirstFileA(szpath, &findFileData);
	if (NULL == hFile)
	{
		return false;
	}
	do
	{
		if (parentdirName.empty())
			relativepath = findFileData.cFileName;
		else
			// 生成zip文件中的相对路径
			relativepath = parentdirName + "\\" + findFileData.cFileName;

		// 如果是目录
		if (findFileData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
		{
			// 去掉目录中的.当前目录和..前一个目录
			if (strcmp(findFileData.cFileName, ".") != 0 && strcmp(findFileData.cFileName, "..") != 0)
			{
				nyAddfiletoZip(zfile, relativepath, "");

				char szTemp[MAX_PATH] = { 0 };
				strcpy_s(szTemp, filepath.c_str());
				strcat_s(szTemp, "\\");
				strcat_s(szTemp, findFileData.cFileName);
				nyCollectfileInDirtoZip(zfile, szTemp, relativepath);
			}
			continue;
		}
		char szTemp[MAX_PATH] = { 0 };
		if (bFile)
		{
			//注意：处理单独文件的压缩
			strcpy_s(szTemp, filepath.c_str());
		}
		else
		{
			//注意：处理目录文件的压缩
			strcpy_s(szTemp, filepath.c_str());
			strcat_s(szTemp, "\\");
			strcat_s(szTemp, findFileData.cFileName);
		}

		nyAddfiletoZip(zfile, relativepath, szTemp);

	} while (::FindNextFileA(hFile, &findFileData));
	FindClose(hFile);

	return true;
}

bool nyCreateZipfromDir(const std::string& dirpathName, const std::string& zipfileName, const std::string& parentdirName)
{
	bool bRet = false;

	/*
	APPEND_STATUS_CREATE    创建追加
	APPEND_STATUS_CREATEAFTER 创建后追加（覆盖方式）
	APPEND_STATUS_ADDINZIP    直接追加
	*/
	zipFile zFile = NULL;
	if (!::PathFileExistsA(zipfileName.c_str()))
	{
		zFile = zipOpen(zipfileName.c_str(), APPEND_STATUS_CREATE);
	}
	else
	{
		zFile = zipOpen(zipfileName.c_str(), APPEND_STATUS_ADDINZIP);
	}
	if (NULL == zFile)
	{
		PTRLOG("cant create zip");
		return bRet;
	}

	if (nyCollectfileInDirtoZip(zFile, dirpathName, parentdirName))
	{
		bRet = true;
	}

	zipClose(zFile, NULL);

	return bRet;
}

Zipperman::Zipperman(FString inTheInFolderPath, FString inTheOutFolderPath, FString inTheProjectName)
{
}

Zipperman::~Zipperman()
{
}

bool Zipperman::Init()
{
	return false;
}

uint32 Zipperman::Run()
{
	return uint32();
}

void Zipperman::Stop()
{
}

void Zipperman::Exit()
{
}

bool Zipperman::ToZippedd(const FString& InFolderPath, const FString& OutFolderPath, const FString& ProjectName)
{
	std::string dirpath = TCHAR_TO_UTF8(*InFolderPath);                   // 源文件/文件夹
	std::string zipfileName = TCHAR_TO_UTF8(*OutFolderPath);           // 目的压缩包
	static const std::string  projectname = TCHAR_TO_UTF8(*ProjectName);
	bool ref = nyCreateZipfromDir(dirpath, zipfileName, projectname);          // 包内文件名<如果为空则压缩时不指定目录>
	return ref;

}

#undef PTRLOG