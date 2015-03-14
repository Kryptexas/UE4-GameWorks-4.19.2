// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "PrivatePCH.h"
#include "IDirectoryWatcher.h"
#include "DirectoryWatcherModule.h"
#include "TestDirectoryWatcher.h"

struct FChangeDetector
{
	void OnDirectoryChanged(const TArray<FFileChangeData>& FileChanges)
	{
		UE_LOG(LogTestPAL, Display, TEXT("%d change(s) detected"), static_cast<int32>(FileChanges.Num()));

		int ChangeIdx = 0;
		for (const auto& ThisEntry : FileChanges)
		{
			UE_LOG(LogTestPAL, Display, TEXT("  Change %d: %s was %s"),
				++ChangeIdx,
				*ThisEntry.Filename,
				ThisEntry.Action == FFileChangeData::FCA_Added ? TEXT("added") :
					(ThisEntry.Action == FFileChangeData::FCA_Removed ? TEXT("removed") :
						(ThisEntry.Action == FFileChangeData::FCA_Modified ? TEXT("modified") : TEXT("??? (unknown)")
						)
					)
			);
		}
	}
};

/**
 * Kicks directory watcher test/
 */
int32 DirectoryWatcherTest(const TCHAR* CommandLine)
{
	FPlatformMisc::SetCrashHandler(NULL);
	FPlatformMisc::SetGracefulTerminationHandler();

	GEngineLoop.PreInit(CommandLine);
	UE_LOG(LogTestPAL, Display, TEXT("Running directory watcher test."));

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	FString TestDir = FString::Printf(TEXT("/tmp/DirectoryWatcherTest%d"), FPlatformProcess::GetCurrentProcessId());

	if (PlatformFile.CreateDirectory(*TestDir) && PlatformFile.CreateDirectory(*(TestDir + TEXT("/subtest"))))
	{
		FChangeDetector Detector;
		FDelegateHandle DirectoryChangedHandle;

		IDirectoryWatcher* DirectoryWatcher = FModuleManager::Get().LoadModuleChecked<FDirectoryWatcherModule>(TEXT("DirectoryWatcher")).Get();
		if (DirectoryWatcher)
		{
			auto Callback = IDirectoryWatcher::FDirectoryChanged::CreateRaw(&Detector, &FChangeDetector::OnDirectoryChanged);
			DirectoryWatcher->RegisterDirectoryChangedCallback_Handle(TestDir, Callback, DirectoryChangedHandle);
		}
		else
		{
			UE_LOG(LogTestPAL, Fatal, TEXT("Could not get DirectoryWatcher module"));
		}

		FPlatformProcess::Sleep(1.0f);
		DirectoryWatcher->Tick(1.0f);

		// create and remove directory
		UE_LOG(LogTestPAL, Log, TEXT("Creating '%s'"), *(TestDir+TEXT("/test")));
		PlatformFile.CreateDirectory(*(TestDir+TEXT("/test")));
		FPlatformProcess::Sleep(1.0f);
		DirectoryWatcher->Tick(1.0f);

		UE_LOG(LogTestPAL, Log, TEXT("Deleting '%s'"), *(TestDir+TEXT("/test")));
		PlatformFile.DeleteDirectory(*(TestDir+TEXT("/test")));
		FPlatformProcess::Sleep(1.0f);
		DirectoryWatcher->Tick(1.0f);

		// create and remove in a sub directory
		UE_LOG(LogTestPAL, Log, TEXT("Creating '%s'"), *(TestDir+TEXT("/subtest/blah")));
		PlatformFile.CreateDirectory(*(TestDir+TEXT("/subtest/blah")));
		FPlatformProcess::Sleep(1.0f);
		DirectoryWatcher->Tick(1.0f);

		UE_LOG(LogTestPAL, Log, TEXT("Deleting '%s'"), *(TestDir+TEXT("/subtest/blah")));
		PlatformFile.DeleteDirectory(*(TestDir+TEXT("/subtest/blah")));
		FPlatformProcess::Sleep(1.0f);
		DirectoryWatcher->Tick(1.0f);

		// clean up
		DirectoryWatcher->UnregisterDirectoryChangedCallback_Handle(TestDir, DirectoryChangedHandle);
		// remove dirs as well
		PlatformFile.DeleteDirectory(*(TestDir + TEXT("/subtest")));
		PlatformFile.DeleteDirectory(*TestDir);
	}
	else
	{
		UE_LOG(LogTestPAL, Fatal, TEXT("Could not create test directory %s."), *TestDir);
	}

	FEngineLoop::AppPreExit();
	FEngineLoop::AppExit();
	return 0;
}
