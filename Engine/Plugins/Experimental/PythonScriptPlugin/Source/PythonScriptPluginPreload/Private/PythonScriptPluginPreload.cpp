// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

#if WITH_PYTHON
THIRD_PARTY_INCLUDES_START
// We include this file to get PY_MAJOR_VERSION
// We don't include Python.h as that will trigger a link dependency which we don't want
// as this module exists to pre-load the Python DLLs, so can't link to Python itself
#include "patchlevel.h"
THIRD_PARTY_INCLUDES_END
#endif	// WITH_PYTHON

class FPythonScriptPluginPreload : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
#if WITH_PYTHON
		LoadPythonLibraries();
#endif	// WITH_PYTHON
	}

	virtual void ShutdownModule() override
	{
#if WITH_PYTHON
		UnloadPythonLibraries();
#endif	// WITH_PYTHON
	}

private:
#if WITH_PYTHON
	void LoadPythonLibraries()
	{
#if PLATFORM_WINDOWS
		// Load the DLLs
		{
			// Build the full Python directory (UE_PYTHON_DIR may be relative to UE4 engine directory for portability)
			FString PythonDir = UTF8_TO_TCHAR(UE_PYTHON_DIR);
			PythonDir.ReplaceInline(TEXT("{ENGINE_DIR}"), *FPaths::EngineDir(), ESearchCase::CaseSensitive);
			FPaths::NormalizeDirectoryName(PythonDir);
			FPaths::RemoveDuplicateSlashes(PythonDir);

			const FString PythonDllWildcard = FString::Printf(TEXT("python%d*.dll"), PY_MAJOR_VERSION);
			auto FindPythonDLLs = [&PythonDllWildcard](const FString& InPath)
			{
				TArray<FString> PythonDLLNames;
				IFileManager::Get().FindFiles(PythonDLLNames, *(InPath / PythonDllWildcard), true, false);
				for (FString& PythonDLLName : PythonDLLNames)
				{
					PythonDLLName = InPath / PythonDLLName;
					FPaths::NormalizeFilename(PythonDLLName);
				}
				return PythonDLLNames;
			};

			TArray<FString> PythonDLLPaths = FindPythonDLLs(PythonDir);
			if (PythonDLLPaths.Num() == 0)
			{
				// If we didn't find anything, check the Windows directory as the DLLs can sometimes be installed there
				FString WinDir;
				{
					TCHAR WinDirA[1024] = {0};
					FPlatformMisc::GetEnvironmentVariable(TEXT("WINDIR"), WinDirA, ARRAY_COUNT(WinDirA) - 1);
					WinDir = WinDirA;
				}

				if (!WinDir.IsEmpty())
				{
					PythonDLLPaths = FindPythonDLLs(WinDir / TEXT("System32"));
				}
			}

			for (const FString& PythonDLLPath : PythonDLLPaths)
			{
				void* DLLHandle = FPlatformProcess::GetDllHandle(*PythonDLLPath);
				check(DLLHandle != nullptr);
				DLLHandles.Add(DLLHandle);
			}
		}
#endif	// PLATFORM_WINDOWS
	}

	void UnloadPythonLibraries()
	{
		for (void* DLLHandle : DLLHandles)
		{
			FPlatformProcess::FreeDllHandle(DLLHandle);
		}
		DLLHandles.Reset();
	}

	TArray<void*> DLLHandles;
#endif	// WITH_PYTHON
};

IMPLEMENT_MODULE(FPythonScriptPluginPreload, PythonScriptPluginPreload)
