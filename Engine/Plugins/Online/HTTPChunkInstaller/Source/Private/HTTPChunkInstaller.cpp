// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "HTTPChunkInstaller.h"
#include "HTTPChunkInstallerLog.h"
#include "ChunkInstall.h"
#include "LocalTitleFile.h"
#include "HTTPOnlineTitleFile.h"
#include "Misc/EngineVersion.h"
#include "Templates/UniquePtr.h"
#include "Interfaces/IBuildInstaller.h"
#include "BuildPatchManifest.h"
#include "Misc/ScopeLock.h"
#include "HAL/RunnableThread.h"
#include "Misc/FileHelper.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"

// Gross hack:
// If games haven't updated their OSS to IWYU, and they're using the engine version
// of this plugin, they'll trigger IWYU monolithic errors, so we're temporarily
// suppressing those warnings since we can't force-update our games.

// Added 2017-02-26; remove as soon as all of our games have taken an Online plugins update
#pragma push_macro("SUPPRESS_MONOLITHIC_HEADER_WARNINGS")
#  define SUPPRESS_MONOLITHIC_HEADER_WARNINGS 1
#  include "OnlineSubsystem.h"
#  include "Online.h"
#pragma pop_macro("SUPPRESS_MONOLITHIC_HEADER_WARNINGS")

#if PLATFORM_ANDROID
#include "OpenGLDrv.h"
#endif

#if PLATFORM_IOS
#include <SystemConfiguration/SystemConfiguration.h>
#endif

#define LOCTEXT_NAMESPACE "HTTPChunkInstaller"

// helper to grab the installer service
static IBuildPatchServicesModule* GetBuildPatchServices()
{
	static IBuildPatchServicesModule* BuildPatchServices = nullptr;
	if (BuildPatchServices == nullptr)
	{
		BuildPatchServices = &FModuleManager::LoadModuleChecked<IBuildPatchServicesModule>(TEXT("BuildPatchServices"));
	}
	return BuildPatchServices;
}

// Helper class to find all pak file manifests.
class FChunkSearchVisitor: public IPlatformFile::FDirectoryVisitor
{
public:
	TArray<FString>				PakManifests;

	FChunkSearchVisitor()
	{}

	virtual bool Visit(const TCHAR* FilenameOrDirectory,bool bIsDirectory)
	{
		if(bIsDirectory == false)
		{
			FString Filename(FilenameOrDirectory);
			if(FPaths::GetBaseFilename(Filename).MatchesWildcard("*.manifest"))
			{
				PakManifests.AddUnique(Filename);
			}
		}
		return true;
	}
};


FHTTPChunkInstall::FHTTPChunkInstall()
	: InstallingChunkID(-1)
	, InstallerState(ChunkInstallState::Setup)
	, InstallSpeed(EChunkInstallSpeed::Fast)
	, bFirstRun(true)
	, bSystemInitialised(false)
#if !UE_BUILD_SHIPPING
	, bDebugNoInstalledRequired(false)
#endif
	, bEnabled(false)
	, bNeedsRetry(false)
	, bPreloaded(false)
{
}


FHTTPChunkInstall::~FHTTPChunkInstall()
{
	if (InstallService.IsValid())
	{
		InstallService->CancelInstall();
		InstallService.Reset();
	}
}

bool FHTTPChunkInstall::Tick(float DeltaSeconds)
{
	static float CountDown = 60.f;

	if (!bEnabled)
		return true;

	switch (InstallerState)
	{
	case ChunkInstallState::Setup:
		{
			check(OnlineTitleFile.IsValid());
			if (!EnumFilesCompleteHandle.IsValid())
			{
				EnumFilesCompleteHandle = OnlineTitleFile->AddOnEnumerateFilesCompleteDelegate_Handle(FOnEnumerateFilesCompleteDelegate::CreateRaw(this, &FHTTPChunkInstall::OSSEnumerateFilesComplete));
			}
			if (!ReadFileCompleteHandle.IsValid())
			{
				ReadFileCompleteHandle = OnlineTitleFile->AddOnReadFileCompleteDelegate_Handle(FOnReadFileCompleteDelegate::CreateRaw(this, &FHTTPChunkInstall::OSSReadFileComplete));
			}
			InstallerState = ChunkInstallState::SetupWait;
		} break;
	case ChunkInstallState::SetupWait:
		{
			InstalledManifests.Reset();
			PrevInstallManifests.Reset();
			MountedPaks.Reset();
			InstallerState = ChunkInstallState::QueryRemoteManifests;
		} break;
	case ChunkInstallState::QueryRemoteManifests:
		{
			//Now query the title file service for the chunk manifests. This should return the list of expected chunk manifests
			check(OnlineTitleFile.IsValid());
			OnlineTitleFile->ClearFiles();
			InstallerState = ChunkInstallState::RequestingTitleFiles;
			UE_LOG(LogHTTPChunkInstaller, Log, TEXT("Enumerating manifest files"));
			OnlineTitleFile->EnumerateFiles();
		} break;
	case ChunkInstallState::SearchTitleFiles:
		{
			FString CleanName;
			TArray<FCloudFileHeader> FileList;
			TitleFilesToRead.Reset();
			RemoteManifests.Reset();
			ExpectedChunks.Empty();
			PendingReads.Reset();
			ManifestsInMemory.Empty();
			OnlineTitleFile->GetFileList(FileList);
			for (int32 FileIndex = 0, FileCount = FileList.Num(); FileIndex < FileCount; ++FileIndex)
			{
				if (FileList[FileIndex].FileName.MatchesWildcard(TEXT("*.manifest")))
				{
					UE_LOG(LogHTTPChunkInstaller, Log, TEXT("Found manifest %s"), *FileList[FileIndex].FileName);
					TitleFilesToRead.Add(FileList[FileIndex]);
					ExpectedChunks.Add(FileList[FileIndex].ChunkID);
				}
			}
			InstallerState = ChunkInstallState::ReadTitleFiles;
		} break;
	case ChunkInstallState::ReadTitleFiles:
		{
/*			if (bFirstRun)
			{
				ChunkMountTask.SetupWork(BPSModule, ContentDir, MountedPaks, ExpectedChunks);
				ChunkMountTaskThread.Reset(FRunnableThread::Create(&ChunkMountTask, TEXT("Chunk mounting thread")));
			}*/
			InstallerState = ChunkInstallState::PostSetup;
		} break;
	case ChunkInstallState::EnterOfflineMode:
		{
/*			if (bFirstRun)
			{
				for (auto It = InstalledManifests.CreateConstIterator(); It; ++It)
				{
					ExpectedChunks.Add(It.Key());
				}
				ChunkMountTask.SetupWork(BPSModule, ContentDir, MountedPaks, ExpectedChunks);
				ChunkMountTaskThread.Reset(FRunnableThread::Create(&ChunkMountTask, TEXT("Chunk mounting thread")));
			}*/
			InstallerState = ChunkInstallState::OfflineMode;
		} break;
	case ChunkInstallState::OfflineMode:
	{
		if (bFirstRun)
		{
			bFirstRun = false;
		}
		/*
			if (ChunkMountTask.IsDone())
			{
				ChunkMountTaskThread->WaitForCompletion();
				ChunkMountTaskThread.Reset();
				MountedPaks.Append(ChunkMountTask.MountedPaks);

				// unmount the out of data paks
				for (auto PairData : RemoteManifests)
				{
					// unmount the previous chunk if it exists
					TArray<FString> Files = PairData.Value->GetBuildFileList();
					auto ChunkFolderName = FString::Printf(TEXT("%s%d"), TEXT("base"), PairData.Key);
					auto ChunkInstallDir = FPaths::Combine(*InstallDir, *ChunkFolderName);
					FString ChunkPak = ChunkInstallDir + TEXT("/") + Files[0];
					if (MountedPaks.Contains(ChunkPak))
					{
						if (FCoreDelegates::OnUnmountPak.IsBound())
						{
							if (FCoreDelegates::OnUnmountPak.Execute(ChunkPak))
							{
								MountedPaks.Remove(ChunkPak);
							}
						}
					}
				}

				UE_LOG(LogHTTPChunkInstaller, Log, TEXT("Completed First Run"));
				bFirstRun = false;
			}
		}*/
		else if (bNeedsRetry)
		{
			CountDown -= DeltaSeconds;
			if (CountDown <= 0.f)
			{
				InstallerState = ChunkInstallState::Retry;
			}
		}
	} break;
	case ChunkInstallState::Retry:
	{
		CountDown = 60.f;
		bNeedsRetry = false;
		RetryDelegate.Broadcast();
		// check for network connection, if restored, try initializing again
		if (PreviousInstallerState < ChunkInstallState::SearchTitleFiles)
		{
			InstallerState = ChunkInstallState::Setup;
			bFirstRun = true;
		}
		else if (PreviousInstallerState < ChunkInstallState::ReadComplete)
		{
			InstallerState = ChunkInstallState::SearchTitleFiles;
			bFirstRun = true;
		}
		else
		{
			InstallerState = ChunkInstallState::Idle;
		}
	} break;
	case ChunkInstallState::PostSetup:
		{
			if (bFirstRun)
			{
/*				if (ChunkMountTask.IsDone())
				{
					ChunkMountTaskThread->WaitForCompletion();
					ChunkMountTaskThread.Reset();
					MountedPaks.Append(ChunkMountTask.MountedPaks);

					// unmount the out of data paks
					for (auto PairData : RemoteManifests)
					{
						// unmount the previous chunk if it exists
						TArray<FString> Files = PairData.Value->GetBuildFileList();
						auto ChunkFolderName = FString::Printf(TEXT("%s%d"), TEXT("base"), PairData.Key);
						auto ChunkInstallDir = FPaths::Combine(*InstallDir, *ChunkFolderName);
						FString ChunkPak = ChunkInstallDir + TEXT("/") + Files[0];
						if (MountedPaks.Contains(ChunkPak))
						{
							if (FCoreDelegates::OnUnmountPak.IsBound())
							{
								if (FCoreDelegates::OnUnmountPak.Execute(ChunkPak))
								{
									MountedPaks.Remove(ChunkPak);
								}
							}
						}
					}

					UE_LOG(LogHTTPChunkInstaller, Log, TEXT("Completed First Run"));
					bFirstRun = false;
				}*/
				bFirstRun = false;
			}
			else
			{
				InstallerState = ChunkInstallState::Idle;
			}
		} break;
	case ChunkInstallState::Idle:
		{
			UpdatePendingInstallQueue();
		} break;
	case ChunkInstallState::CopyToContent:
		{
			if (!ChunkCopyInstall.IsDone() || !InstallService->IsComplete())
			{
				break;
			}
			check(InstallingChunkID != -1);
			if (InstallService.IsValid())
			{
				InstallService.Reset();
			}
			ChunkCopyInstallThread.Reset();
			check(RemoteManifests.Find(InstallingChunkID));
			UE_LOG(LogHTTPChunkInstaller, Log, TEXT("Adding Chunk %d to installed manifests"), InstallingChunkID);
			InstalledManifests.Add(InstallingChunkID, InstallingChunkManifest);
			UE_LOG(LogHTTPChunkInstaller, Log, TEXT("Removing Chunk %d from remote manifests"), InstallingChunkID);
			RemoteManifests.Remove(InstallingChunkID, InstallingChunkManifest);
			MountedPaks.Append(ChunkCopyInstall.MountedPaks);
			if (!RemoteManifests.Contains(InstallingChunkID))
			{
				// No more manifests relating to the chunk ID are left to install.
				// Inform any listeners that the install has been completed.
				FPlatformChunkInstallCompleteMultiDelegate* FoundDelegate = DelegateMap.Find(InstallingChunkID);
				if (FoundDelegate)
				{
					FoundDelegate->Broadcast(InstallingChunkID);
				}
			}
			EndInstall();

		} break;
	case ChunkInstallState::Installing:
	{
		// get the current progress and send out to the progress delegate
		check(InstallService.IsValid());
		FText Percentage = InstallService->GetPercentageText();
		float Progress = InstallService->GetUpdateProgress();
		int64 TotalSize = InstallService->GetInitialDownloadSize();
		int64 CurrentSize = InstallService->GetTotalDownloaded();
		FPlatformChunkInstallProgressMultiDelegate* FoundDelegate = ProgressDelegateMap.Find(InstallingChunkID);
		if (FoundDelegate)
		{
			FoundDelegate->Broadcast(InstallingChunkID, Progress, TotalSize, CurrentSize, Percentage);
		}
	}
	case ChunkInstallState::RequestingTitleFiles:
	case ChunkInstallState::WaitingOnRead:
	default:
		break;
	}

	if (OnlineTitleFileHttp.IsValid())
	{
		static_cast<FOnlineTitleFileHttp*>(OnlineTitleFileHttp.Get())->Tick(DeltaSeconds);
	}

	return true;
}

bool FHTTPChunkInstall::IsConnectedToWiFiorLAN() const
{
	return FPlatformMisc::HasActiveWiFiConnection();
}

void FHTTPChunkInstall::UpdatePendingInstallQueue()
{
	// force a pak mount right here
	if (PaksToMount.Num() > 0 && ChunkMountOnlyTask.IsDone())
	{
		MountedPaks.Append(ChunkMountOnlyTask.MountedPaks);
		while (PaksToMount.Num() > 0)
		{
			ChunkMountOnlyTask.AddWork(PaksToMount[0].DestDir, PaksToMount[0].BuildManifest.ToSharedRef(), MountedPaks);
			PaksToMount.RemoveAt(0);
		}
		ChunkMountOnlyTaskThread.Reset(FRunnableThread::Create(&ChunkMountOnlyTask, TEXT("Chunk Mount Only Thread")));
	}
	else if (ChunkMountOnlyTask.IsDone())
	{
		if (ChunkMountOnlyTask.MountedPaks.Num() > 0)
		{
			MountedPaks.Append(ChunkMountOnlyTask.MountedPaks);
			ChunkMountOnlyTask.MountedPaks.Reset();
		}
	}

	if (InstallingChunkID != -1
#if !UE_BUILD_SHIPPING
		|| bDebugNoInstalledRequired
#endif
	)
	{
		return;
	}

	check(!InstallService.IsValid());
	bool bPatch = false;
	while (PriorityQueue.Num() > 0 && InstallerState != ChunkInstallState::Installing)
	{
		const FChunkPrio& NextChunk = PriorityQueue[0];

		FScopeLock Lock(&ReadFileGuard);
		if (PendingReads.Num() > 0)
		{
			for (int Index = 0; Index < PendingReads.Num(); Index++)
			{
				if (PendingReads[Index].ChunkID == NextChunk.ChunkID)
				{
					// current manifest is in flight, return early and we'll check on the next update
					return;
				}
			}
		}

		TArray<IBuildManifestPtr> FoundChunkManifests;
		RemoteManifests.MultiFind(NextChunk.ChunkID, FoundChunkManifests);
		if (FoundChunkManifests.Num() > 0)
		{
			auto ChunkManifest = FoundChunkManifests[0];
			auto ChunkIDField = ChunkManifest->GetCustomField("ChunkID");
			if (ChunkIDField.IsValid())
			{
				BeginChunkInstall(NextChunk.ChunkID, ChunkManifest, FindPreviousInstallManifest(ChunkManifest));
			}
			else
			{
				PriorityQueue.RemoveAt(0);
			}
		}
		else
		{
			PriorityQueue.RemoveAt(0);
		}
	}
	if (InstallingChunkID == -1)
	{
		// check to see if we need to mount or get some title files
		int32 MountCount = 10;
		while (TitleFilesToRead.Num() > 0 && MountCount > 0)
		{
			// check for an already installed manifest
			FString ChunkFdrName;
			FString ManifestName;
			if (BuildChunkFolderName(ChunkFdrName, ManifestName, TitleFilesToRead[0].ChunkID))
			{
				FString ManifestPath = FPaths::Combine(*ContentDir, *ChunkFdrName, *ManifestName);
				FString HoldingPath = FPaths::Combine(*HoldingDir, *ChunkFdrName, *ManifestName);
				IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
				if (PlatformFile.FileExists(*ManifestPath))
				{
					auto Manifest = BPSModule->LoadManifestFromFile(ManifestPath);
					if (!Manifest.IsValid())
					{
						//Something is wrong, suggests corruption so mark the folder for delete
						PlatformFile.DeleteFile(*ManifestPath);
					}
					else
					{
						auto ChunkIDField = Manifest->GetCustomField("ChunkID");
						if (!ChunkIDField.IsValid())
						{
							//Something is wrong, suggests corruption so mark the folder for delete
							PlatformFile.DeleteFile(*ManifestPath);
						}
						else
						{
							InstalledManifests.Add(TitleFilesToRead[0].ChunkID, Manifest);
						}
					}
				}
			}

			if (!IsDataInFileCache(TitleFilesToRead[0].Hash))
			{
				// add to the read list
				FScopeLock Lock(&ReadFileGuard);
				NeedsRead.Add(TitleFilesToRead[0]);
				TitleFilesToRead.RemoveAt(0);
			}
			else
			{
				// read the local file and mount the pak
				FScopeLock Lock(&ReadFileGuard);
				PendingReads.Add(TitleFilesToRead[0]);
				OSSReadFileComplete(true, TitleFilesToRead[0].DLName);
				TitleFilesToRead.RemoveAt(0);
			}
			MountCount--;
		}

		// read any pending files one at a time
		{
			if (NeedsRead.Num() > 0)
			{
				FScopeLock Lock(&ReadFileGuard);
				UE_LOG(LogHTTPChunkInstaller, Log, TEXT("Reading manifest %s from remote source"), *NeedsRead[0].FileName);
				PendingReads.Add(NeedsRead[0]);
				OnlineTitleFile->ReadFile(NeedsRead[0].DLName);
				NeedsRead.RemoveAt(0);
				return;
			}
		}

		// Install the first available chunk
		if (IsConnectedToWiFiorLAN() && TitleFilesToRead.Num() == 0)
		{
			for (auto It = RemoteManifests.CreateConstIterator(); It; ++It)
			{
				if (It)
				{
					IBuildManifestPtr ChunkManifest = It.Value();
					auto ChunkIDField = ChunkManifest->GetCustomField("ChunkID");
					if (ChunkIDField.IsValid())
					{
						BeginChunkInstall(ChunkIDField->AsInteger(), ChunkManifest, FindPreviousInstallManifest(ChunkManifest));
						return;
					}
				}
			}
		}
	}
}


EChunkLocation::Type FHTTPChunkInstall::GetChunkLocation(uint32 ChunkID)
{
#if !UE_BUILD_SHIPPING
	if(bDebugNoInstalledRequired)
	{
		return EChunkLocation::BestLocation;
	}
#endif

	// Safe to assume Chunk0 is ready
	if (ChunkID == 0)
	{
		return EChunkLocation::BestLocation;
	}

#if !PLATFORM_IOS
	if ((ChunkID == 3 || ChunkID == 4 || ChunkID == 6 || ChunkID == 7 || ChunkID == 8 || ChunkID == 9 || ChunkID == 16
		|| ChunkID == 101 || ChunkID == 104 || ChunkID == 106 || ChunkID == 110 || ChunkID == 112 || ChunkID == 118 || ChunkID == 119
		|| ChunkID == 120 || ChunkID == 121 || ChunkID == 123 || ChunkID == 128 || ChunkID == 130 || ChunkID == 131 || ChunkID == 900
		|| ChunkID == 901 || ChunkID == 902 || ChunkID == 903 || ChunkID == 904 || ChunkID == 905 || ChunkID == 906 || ChunkID == 907
		|| ChunkID == 908 || ChunkID == 1002) && bPreloaded)
	{
		return EChunkLocation::BestLocation;
	}
#else
	if ((ChunkID == 3 || ChunkID == 4 || ChunkID == 6 || ChunkID == 8 || ChunkID == 16
		|| ChunkID == 101 || ChunkID == 104 || ChunkID == 106 || ChunkID == 110 || ChunkID == 112 || ChunkID == 118 || ChunkID == 119
		|| ChunkID == 120 || ChunkID == 121 || ChunkID == 123 || ChunkID == 128 || ChunkID == 130 || ChunkID == 131 || ChunkID == 900
		|| ChunkID == 901 || ChunkID == 902 || ChunkID == 903 || ChunkID == 904 || ChunkID == 905 || ChunkID == 906 || ChunkID == 907
		|| ChunkID == 908 || ChunkID == 1002) && bPreloaded)
	{
		return EChunkLocation::BestLocation;
	}
#endif // PLATFORM_IOS

	if (bFirstRun || !bSystemInitialised)
	{
		/** Still waiting on setup to finish, report that nothing is installed yet... */
		return EChunkLocation::DoesNotExist;
	}
	TArray<IBuildManifestPtr> FoundManifests;
	RemoteManifests.MultiFind(ChunkID, FoundManifests);
	if (FoundManifests.Num() > 0)
	{
		return EChunkLocation::NotAvailable;
	}

	InstalledManifests.MultiFind(ChunkID, FoundManifests);
	if (FoundManifests.Num() > 0)
	{
		return EChunkLocation::BestLocation;
	}

	// new version: not already installed or needs update, need to check the remote file headers for the chunk and then see if we have an installed version of that chunk
	{
		FScopeLock Lock(&ReadFileGuard);
		for (int32 Index = 0; Index < PendingReads.Num(); ++Index)
		{
			if (PendingReads[Index].ChunkID == ChunkID)
			{
				return EChunkLocation::NotAvailable;
			}
		}
	}

	for (int32 Lists = 0; Lists < 2; ++Lists)
	{
		TArray<FCloudFileHeader>* FilesToRead = Lists ? &NeedsRead : &TitleFilesToRead;
		for (int32 Index = 0; Index < FilesToRead->Num(); ++Index)
		{
			if ((*FilesToRead)[Index].ChunkID == ChunkID)
			{
				// check for an already installed manifest
				FString ChunkFdrName;
				FString ManifestName;
				if (BuildChunkFolderName(ChunkFdrName, ManifestName, ChunkID))
				{
					FString ManifestPath = FPaths::Combine(*ContentDir, *ChunkFdrName, *ManifestName);
					FString HoldingPath = FPaths::Combine(*HoldingDir, *ChunkFdrName, *ManifestName);
					IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
					if (PlatformFile.FileExists(*ManifestPath))
					{
						auto Manifest = BPSModule->LoadManifestFromFile(ManifestPath);
						if (!Manifest.IsValid())
						{
							//Something is wrong, suggests corruption so mark the folder for delete
							PlatformFile.DeleteFile(*ManifestPath);
						}
						else
						{
							auto ChunkIDField = Manifest->GetCustomField("ChunkID");
							if (!ChunkIDField.IsValid())
							{
								//Something is wrong, suggests corruption so mark the folder for delete
								PlatformFile.DeleteFile(*ManifestPath);
							}
							else
							{
								InstalledManifests.Add(ChunkID, Manifest);
							}
						}
					}
				}

				// kick off a read of the manifest
				if (!IsDataInFileCache((*FilesToRead)[Index].Hash))
				{
					FScopeLock Lock(&ReadFileGuard);
					PendingReads.Add((*FilesToRead)[Index]);
					UE_LOG(LogHTTPChunkInstaller, Log, TEXT("Reading manifest %s from remote source"), *(*FilesToRead)[Index].FileName);
					OnlineTitleFile->ReadFile((*FilesToRead)[Index].DLName);
					(*FilesToRead).RemoveAt(Index);
				}
				else
				{
					FScopeLock Lock(&ReadFileGuard);
					PendingReads.Add((*FilesToRead)[Index]);
					OSSReadFileComplete(true, (*FilesToRead)[Index].DLName);
					(*FilesToRead).RemoveAt(Index);

					// force a pak mount right here
					if (PaksToMount.Num() > 0)
					{
						while (!ChunkMountOnlyTask.IsDone())
						{
							FPlatformProcess::Sleep(0.0f);
						}
						MountedPaks.Append(ChunkMountOnlyTask.MountedPaks);
						ChunkMountOnlyTask.MountedPaks.Reset();
						while (PaksToMount.Num() > 0)
						{
							ChunkMountOnlyTask.AddWork(PaksToMount[0].DestDir, PaksToMount[0].BuildManifest.ToSharedRef(), MountedPaks);
							PaksToMount.RemoveAt(0);
						}
						ChunkMountOnlyTaskThread.Reset(FRunnableThread::Create(&ChunkMountOnlyTask, TEXT("Chunk Mount Only Thread")));
						while (!ChunkMountOnlyTask.IsDone())
						{
							FPlatformProcess::Sleep(0.0f);
						}
						MountedPaks.Append(ChunkMountOnlyTask.MountedPaks);
						ChunkMountOnlyTask.MountedPaks.Reset();
					}

					// do another check of the remote vs installed
					RemoteManifests.MultiFind(ChunkID, FoundManifests);
					if (FoundManifests.Num() > 0)
					{
						return EChunkLocation::NotAvailable;
					}

					InstalledManifests.MultiFind(ChunkID, FoundManifests);
					if (FoundManifests.Num() > 0)
					{
						return EChunkLocation::BestLocation;
					}
				}
				return EChunkLocation::NotAvailable;
			}
		}
	}

	return EChunkLocation::DoesNotExist;
}


float FHTTPChunkInstall::GetChunkProgress(uint32 ChunkID,EChunkProgressReportingType::Type ReportType)
{
#if !UE_BUILD_SHIPPING
	if (bDebugNoInstalledRequired)
	{
		return 100.f;
	}
#endif

	// Safe to assume Chunk0 is ready
	if (ChunkID == 0)
	{
		return 100.f;
	}

	if (bFirstRun || !bSystemInitialised)
	{
		/** Still waiting on setup to finish, report that nothing is installed yet... */
		return 0.f;
	}
	TArray<IBuildManifestPtr> FoundManifests;
	RemoteManifests.MultiFind(ChunkID, FoundManifests);
	if (FoundManifests.Num() > 0)
	{
		float Progress = 0;
		if (InstallingChunkID == ChunkID && InstallService.IsValid())
		{
			Progress = InstallService->GetUpdateProgress();
		}
		return Progress / FoundManifests.Num();
	}

	InstalledManifests.MultiFind(ChunkID, FoundManifests);
	if (FoundManifests.Num() > 0)
	{
		return 100.f;
	}

	return 0.f;
}

void FHTTPChunkInstall::OSSEnumerateFilesComplete(bool bSuccess)
{
	if (bSuccess)
	{
		InstallerState = ChunkInstallState::SearchTitleFiles;
	}
	else
	{
		if (!bNeedsRetry)
		{
			bNeedsRetry = true;
			PreviousInstallerState = InstallerState;
		}
		if (InstallerState != ChunkInstallState::OfflineMode)
		{
			InstallerState = ChunkInstallState::EnterOfflineMode;
		}
		FOnlineTitleFileHttp* TitleFileHttp = static_cast<FOnlineTitleFileHttp*>(OnlineTitleFileHttp.Get());
		EChunkInstallErrorCode ErrorCode = EChunkInstallErrorCode::RemoteFailure;
		switch (TitleFileHttp->IsClientCompatible())
		{
		case FOnlineTitleFileHttp::Compatible::UnknownCompatibility:
			// we didn't get far enough to download. let the other error stand
			break;
		case FOnlineTitleFileHttp::Compatible::ClientIsCompatible:
			// version is good, something else failed
			break;
		case FOnlineTitleFileHttp::Compatible::ClientUpdateRequired:
			// don't retry if we detect incompatible client
			bNeedsRetry = false;
			ErrorCode = EChunkInstallErrorCode::NeedsClientUpdate;
			break;
		case FOnlineTitleFileHttp::Compatible::ClientIsNewerThanMcp:
			// don't retry if we detect incompatible client
			bNeedsRetry = false;
			ErrorCode = EChunkInstallErrorCode::ClientIsTooNew;
			break;
		}
		ErrorDelegate.Broadcast(FChunkInstallError(ErrorCode, TitleFileHttp->GetErrorMsg()));
	}
}

void FHTTPChunkInstall::OSSReadFileComplete(bool bSuccess, const FString& Filename)
{
	FScopeLock Lock(&ReadFileGuard);

	// find the cloud file header
	int TitleFileIndex = -1;
	for (int Index = 0; Index < PendingReads.Num(); ++Index)
	{
		if (PendingReads[Index].DLName == Filename)
		{
			TitleFileIndex = Index;
			break;
		}
	}
	if (TitleFileIndex == -1)
	{
		return;
	}

	if (!bSuccess) // || (InstallerState != ChunkInstallState::WaitingOnRead && InstallerState != ChunkInstallState::ReadTitleFiles))
	{
		if (!bNeedsRetry)
		{
			bNeedsRetry = true;
			PreviousInstallerState = InstallerState;
		}
		if (InstallerState != ChunkInstallState::OfflineMode)
		{
			InstallerState = ChunkInstallState::EnterOfflineMode;
		}

		// remove the pending read
		PendingReads.RemoveAt(TitleFileIndex);

		// fire the error
		FText ErrorMsg = static_cast<FOnlineTitleFileHttp*>(OnlineTitleFileHttp.Get())->GetErrorMsg();
		ErrorDelegate.Broadcast(FChunkInstallError(EChunkInstallErrorCode::RemoteFailure, ErrorMsg));
		return;
	}

	FileContentBuffer.Reset();
	bool bReadOK = false;
	bool bAlreadyLoaded = ManifestsInMemory.Contains(PendingReads[TitleFileIndex].Hash);
	if (!IsDataInFileCache(PendingReads[TitleFileIndex].Hash))
	{
		bReadOK = OnlineTitleFile->GetFileContents(PendingReads[TitleFileIndex].DLName, FileContentBuffer);
		if (bReadOK)
		{
			AddDataToFileCache(PendingReads[TitleFileIndex].Hash, FileContentBuffer);
		}
	}
	else if (!bAlreadyLoaded)
	{
		bReadOK = GetDataFromFileCache(PendingReads[TitleFileIndex].Hash, FileContentBuffer);
		if (!bReadOK)
		{
			RemoveDataFromFileCache(PendingReads[TitleFileIndex].Hash);
		}
	}
	if (bReadOK || bAlreadyLoaded)
	{
		if (!bAlreadyLoaded)
		{
			ParseTitleFileManifest(PendingReads[TitleFileIndex].Hash);
		}
		// Even if the Parse failed remove the file from the list
		PendingReads.RemoveAt(TitleFileIndex);
	}
}

void FHTTPChunkInstall::OSSInstallComplete(bool bSuccess, IBuildManifestRef BuildManifest)
{
	if (bSuccess)
	{
		// Completed OK. Write the manifest. If the chunk doesn't exist, copy to the content dir.
		// Otherwise, writing the manifest will prompt a copy on next start of the game
		FString ManifestName;
		FString ChunkFdrName;
		uint32 ChunkID;
		bool bIsPatch;
		if (!BuildChunkFolderName(BuildManifest, ChunkFdrName, ManifestName, ChunkID, bIsPatch))
		{
			//Something bad has happened, bail
			EndInstall();
			return;
		}
		UE_LOG(LogHTTPChunkInstaller, Log, TEXT("Chunk %d install complete, preparing to copy to content directory"), ChunkID);
		FString ManifestPath = FPaths::Combine(*InstallDir, *ChunkFdrName, *ManifestName);
		FString HoldingManifestPath = FPaths::Combine(*HoldingDir, *ChunkFdrName, *ManifestName);
		FString SrcDir = FPaths::Combine(*InstallDir, *ChunkFdrName);
		FString DestDir = FPaths::Combine(*ContentDir, *ChunkFdrName);
		bool bCopyDir = InstallDir != ContentDir;
		TArray<IBuildManifestPtr> FoundManifests;
		InstalledManifests.MultiFind(ChunkID, FoundManifests);
		for (const auto& It : FoundManifests)
		{
			auto FoundPatchField = It->GetCustomField("bIsPatch");
			bool bFoundPatch = FoundPatchField.IsValid() ? FoundPatchField->AsString() == TEXT("true") : false;
			if (bFoundPatch == bIsPatch)
			{
				bCopyDir = false;
			}
		}
		ChunkCopyInstall.SetupWork(ManifestPath, HoldingManifestPath, SrcDir, DestDir, BPSModule, BuildManifest, MountedPaks, bCopyDir);
		UE_LOG(LogHTTPChunkInstaller, Log, TEXT("Copying Chunk %d to content directory"), ChunkID);
		ChunkCopyInstallThread.Reset(FRunnableThread::Create(&ChunkCopyInstall, TEXT("Chunk Install Copy Thread")));
		InstallerState = ChunkInstallState::CopyToContent;

		check(InstallService.IsValid());
		FText Percentage = FText::FromString(TEXT("100%"));
		float Progress = 1.0f;
		int64 TotalSize = InstallService->GetInitialDownloadSize();
		int64 CurrentSize = InstallService->GetTotalDownloaded();
		double CurrTime = FPlatformTime::Seconds();
		double DeltaTime = (CurrTime - StartTime);

		FPlatformChunkInstallProgressMultiDelegate* FoundDelegate = ProgressDelegateMap.Find(InstallingChunkID);
		if (FoundDelegate)
		{
			FoundDelegate->Broadcast(InstallingChunkID, Progress, TotalSize, CurrentSize, Percentage);
		}
		AnalyticsDelegate.Broadcast(true, InstallingChunkID, DeltaTime, TotalSize, InstallService->GetBuildStatistics());
	}
	else
	{
		//Something bad has happed, return to the Idle state. We'll re-attempt the install
		if (InstallService->HasError())
		{
			switch (InstallService->GetErrorType())
			{
			case EBuildPatchInstallError::DownloadError:
				ErrorDelegate.Broadcast(FChunkInstallError(EChunkInstallErrorCode::RemoteFailure, LOCTEXT("ReadFiles", "Failure to download files, retrying...")));
				break;
			case EBuildPatchInstallError::OutOfDiskSpace:
				ErrorDelegate.Broadcast(FChunkInstallError(EChunkInstallErrorCode::LocalFailure, LOCTEXT("Diskspace", "No space on device.")));
				break;
			case EBuildPatchInstallError::MoveFileToInstall:
				ErrorDelegate.Broadcast(FChunkInstallError(EChunkInstallErrorCode::LocalFailure, LOCTEXT("InstallFailure", "Failed to install file, retrying...")));
				break;
			default:
				ErrorDelegate.Broadcast(FChunkInstallError(EChunkInstallErrorCode::GenericFailure, LOCTEXT("UnknownFailure", "Unknown Failure, retrying...")));
				break;
			}
		}
		int64 TotalSize = InstallService->GetInitialDownloadSize();
		double CurrTime = FPlatformTime::Seconds();
		double DeltaTime = (CurrTime - StartTime);
		AnalyticsDelegate.Broadcast(false, InstallingChunkID, DeltaTime, TotalSize, InstallService->GetBuildStatistics());
		EndInstall();

		// set to the retry state
		if (!bNeedsRetry)
		{
			bNeedsRetry = true;
			PreviousInstallerState = InstallerState;
		}
		if (InstallerState != ChunkInstallState::OfflineMode)
		{
			InstallerState = ChunkInstallState::EnterOfflineMode;
		}
	}
	FPlatformMisc::ControlScreensaver(FGenericPlatformMisc::Enable);
}

bool FHTTPChunkInstall::FilesHaveChanged(IBuildManifestPtr LocalManifest, IBuildManifestPtr RemoteManifest, int32 InInstallingChunkID)
{
#if (!WITH_EDITORONLY_DATA || IS_PROGRAM)
	if (LocalManifest->GetDownloadSize() != RemoteManifest->GetDownloadSize())
	{
		return true;
	}

	// same number of files check to see if the file StartBuis out dated
	auto ChunkFolderName = FString::Printf(TEXT("%s%d"), TEXT("base"), InInstallingChunkID);
	auto ChunkInstallDir = FPaths::Combine(*InstallDir, *ChunkFolderName);
	FBuildPatchAppManifestPtr CurrentManifest = StaticCastSharedPtr< FBuildPatchAppManifest >(LocalManifest);
	FBuildPatchAppManifestPtr InstallManifest = StaticCastSharedPtr< FBuildPatchAppManifest >(RemoteManifest);
	TSet<FString> OutOfDateFiles;
	CurrentManifest->GetOutdatedFiles(InstallManifest.ToSharedRef(), ChunkInstallDir, OutOfDateFiles);
	return OutOfDateFiles.Num() > 0;
#else
	return false;
#endif
}

void FHTTPChunkInstall::ParseTitleFileManifest(const FString& ManifestFileHash)
{
#if !UE_BUILD_SHIPPING
	if (bDebugNoInstalledRequired)
	{
		// Forces the installer to think that no remote manifests exist, so nothing needs to be installed.
		return;
	}
#endif
	FString JsonBuffer;
	FFileHelper::BufferToString(JsonBuffer, FileContentBuffer.GetData(), FileContentBuffer.Num());
	auto RemoteManifest = BPSModule->MakeManifestFromJSON(JsonBuffer);
	if (!RemoteManifest.IsValid())
	{
		UE_LOG(LogHTTPChunkInstaller, Warning, TEXT("Manifest was invalid"));
		return;
	}
	auto RemoteChunkIDField = RemoteManifest->GetCustomField("ChunkID");
	if (!RemoteChunkIDField.IsValid())
	{
		UE_LOG(LogHTTPChunkInstaller, Warning, TEXT("Manifest ChunkID was invalid or missing"));
		return;
	}
	//Compare to installed manifests and add to the remote if it needs to be installed.
	uint32 ChunkID = (uint32)RemoteChunkIDField->AsInteger();
	if (!ExpectedChunks.Contains(ChunkID))
	{
		ExpectedChunks.Add(ChunkID);
	}

	// new version: check for an already installed manifest
	TArray<IBuildManifestPtr> FoundManifests;
	InstalledManifests.MultiFind(ChunkID, FoundManifests);
	uint32 FoundCount = FoundManifests.Num();
	if (FoundCount > 0)
	{
		auto RemotePatchManifest = RemoteManifest->GetCustomField("bIsPatch");
		auto RemoteVersion = RemoteManifest->GetVersionString();
		bool bRemoteIsPatch = RemotePatchManifest.IsValid() ? RemotePatchManifest->AsString() == TEXT("true") : false;
		for (uint32 FoundIndex = 0; FoundIndex < FoundCount; ++FoundIndex)
		{
			const auto& InstalledManifest = FoundManifests[FoundIndex];
			auto InstalledVersion = InstalledManifest->GetVersionString();
			auto InstallPatchManifest = InstalledManifest->GetCustomField("bIsPatch");
			bool bInstallIsPatch = InstallPatchManifest.IsValid() ? InstallPatchManifest->AsString() == TEXT("true") : false;
			if ((InstalledVersion != RemoteVersion && bInstallIsPatch == bRemoteIsPatch) || FilesHaveChanged(InstalledManifest, RemoteManifest, ChunkID))
			{
				UE_LOG(LogHTTPChunkInstaller, Log, TEXT("Adding Chunk %d to remote manifests"), ChunkID);
				RemoteManifests.Add(ChunkID, RemoteManifest);
				if(!ManifestFileHash.IsEmpty())
				{
					ManifestsInMemory.Add(ManifestFileHash);
				}
				//Remove from the installed map
//				if (bFirstRun)
				{
					// Prevent the paks from being mounted by removing the manifest file
					FString ChunkFdrName;
					FString ManifestName;
					bool bIsPatch;
					if (BuildChunkFolderName(InstalledManifest.ToSharedRef(), ChunkFdrName, ManifestName, ChunkID, bIsPatch))
					{
						FString ManifestPath = FPaths::Combine(*ContentDir, *ChunkFdrName, *ManifestName);
						FString HoldingPath = FPaths::Combine(*HoldingDir, *ChunkFdrName, *ManifestName);
						IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
						PlatformFile.CreateDirectoryTree(*FPaths::Combine(*HoldingDir, *ChunkFdrName));
						PlatformFile.MoveFile(*HoldingPath, *ManifestPath);
					}
					UE_LOG(LogHTTPChunkInstaller, Log, TEXT("Adding Chunk %d to previous installed manifests"), ChunkID);
					PrevInstallManifests.Add(ChunkID, InstalledManifest);
					UE_LOG(LogHTTPChunkInstaller, Log, TEXT("Removing Chunk %d from installed manifests"), ChunkID);
					InstalledManifests.Remove(ChunkID, InstalledManifest);
				}
			}
			else
			{
				// mount the found chunk
				FString ChunkFdrName;
				FString ManifestName;
				bool bIsPatch;
				if (BuildChunkFolderName(InstalledManifest.ToSharedRef(), ChunkFdrName, ManifestName, ChunkID, bIsPatch))
				{
					FString DestDir = FPaths::Combine(*ContentDir, *ChunkFdrName);
					MountTask Task = { DestDir, InstalledManifest };
					PaksToMount.Add(Task);
					UE_LOG(LogHTTPChunkInstaller, Log, TEXT("Mounting Chunk %d"), ChunkID);
				}
			}
		}
	}
	else
	{
		UE_LOG(LogHTTPChunkInstaller, Log, TEXT("Adding Chunk %d to remote manifests"), ChunkID);
		RemoteManifests.Add(ChunkID, RemoteManifest);
		if (!ManifestFileHash.IsEmpty())
		{
			ManifestsInMemory.Add(ManifestFileHash);
		}
	}
}

bool FHTTPChunkInstall::BuildChunkFolderName(IBuildManifestRef Manifest, FString& ChunkFdrName, FString& ManifestName, uint32& ChunkID, bool& bIsPatch)
{
	auto ChunkIDField = Manifest->GetCustomField("ChunkID");
	auto ChunkPatchField = Manifest->GetCustomField("bIsPatch");

	if (!ChunkIDField.IsValid())
	{
		return false;
	}
	ChunkID = ChunkIDField->AsInteger();
	bIsPatch = ChunkPatchField.IsValid() ? ChunkPatchField->AsString() == TEXT("true") : false;
	ManifestName = FString::Printf(TEXT("chunk_%u"), ChunkID);
	if (bIsPatch)
	{
		ManifestName += TEXT("_patch");
	}
	ManifestName += TEXT(".manifest");
	ChunkFdrName = FString::Printf(TEXT("%s%d"), !bIsPatch ? TEXT("base") : TEXT("patch"), ChunkID);
	return true;
}

bool FHTTPChunkInstall::BuildChunkFolderName(FString& ChunkFdrName, FString& ManifestName, uint32 ChunkID)
{
	ManifestName = FString::Printf(TEXT("chunk_%u"), ChunkID);
	ManifestName += TEXT(".manifest");
	ChunkFdrName = FString::Printf(TEXT("%s%d"), TEXT("base"), ChunkID);
	return true;
}

bool FHTTPChunkInstall::PrioritizeChunk(uint32 ChunkID, EChunkPriority::Type Priority)
{
	int32 FoundIndex;
	PriorityQueue.Find(FChunkPrio(ChunkID, Priority), FoundIndex);
	if (FoundIndex != INDEX_NONE)
	{
		PriorityQueue.RemoveAt(FoundIndex);
	}
	// Low priority is assumed if the chunk ID doesn't exist in the queue
	if (Priority != EChunkPriority::Low)
	{
		PriorityQueue.AddUnique(FChunkPrio(ChunkID, Priority));
		PriorityQueue.Sort();
	}
	return true;
}

FDelegateHandle FHTTPChunkInstall::SetChunkInstallDelgate(uint32 ChunkID, FPlatformChunkInstallCompleteDelegate Delegate)
{
	FPlatformChunkInstallCompleteMultiDelegate* FoundDelegate = DelegateMap.Find(ChunkID);
	if (FoundDelegate)
	{
		return FoundDelegate->Add(Delegate);
	}
	else
	{
		FPlatformChunkInstallCompleteMultiDelegate MC;
		auto RetVal = MC.Add(Delegate);
		DelegateMap.Add(ChunkID, MC);
		return RetVal;
	}
	return FDelegateHandle();
}

void FHTTPChunkInstall::RemoveChunkInstallDelgate(uint32 ChunkID, FDelegateHandle Delegate)
{
	FPlatformChunkInstallCompleteMultiDelegate* FoundDelegate = DelegateMap.Find(ChunkID);
	if (!FoundDelegate)
	{
		return;
	}
	FoundDelegate->Remove(Delegate);
}

FDelegateHandle FHTTPChunkInstall::SetChunkInstallProgressDelgate(uint32 ChunkID, FPlatformChunkInstallProgressDelegate Delegate)
{
	FPlatformChunkInstallProgressMultiDelegate* FoundDelegate = ProgressDelegateMap.Find(ChunkID);
	if (FoundDelegate)
	{
		return FoundDelegate->Add(Delegate);
	}
	else
	{
		FPlatformChunkInstallProgressMultiDelegate MC;
		auto RetVal = MC.Add(Delegate);
		ProgressDelegateMap.Add(ChunkID, MC);
		return RetVal;
	}
	return FDelegateHandle();
}

void FHTTPChunkInstall::RemoveChunkInstallProgressDelgate(uint32 ChunkID, FDelegateHandle Delegate)
{
	FPlatformChunkInstallProgressMultiDelegate* FoundDelegate = ProgressDelegateMap.Find(ChunkID);
	if (!FoundDelegate)
	{
		return;
	}
	FoundDelegate->Remove(Delegate);
}

FDelegateHandle FHTTPChunkInstall::SetErrorDelegate(FPlatformChunkInstallErrorDelegate Delegate)
{
	return ErrorDelegate.Add(Delegate);
}

void FHTTPChunkInstall::RemoveErrorDelegate(FDelegateHandle Delegate)
{
	ErrorDelegate.Remove(Delegate);
}

FDelegateHandle FHTTPChunkInstall::SetChunkAnalyticsDelegate(FPlatformChunkAnalyticsDelegate Delegate)
{
	return AnalyticsDelegate.Add(Delegate);
}

void FHTTPChunkInstall::RemoveChunkAnalyticsDelegate(FDelegateHandle Delegate)
{
	AnalyticsDelegate.Remove(Delegate);
}

FDelegateHandle FHTTPChunkInstall::SetRetryDelegate(FPlatformChunkInstallRetryDelegate Delegate)
{
	return RetryDelegate.Add(Delegate);
}

void FHTTPChunkInstall::RemoveRetryDelegate(FDelegateHandle Delegate)
{
	RetryDelegate.Remove(Delegate);
}

void FHTTPChunkInstall::BeginChunkInstall(uint32 ChunkID,IBuildManifestPtr ChunkManifest, IBuildManifestPtr PrevInstallChunkManifest)
{
	check(ChunkManifest->GetCustomField("ChunkID").IsValid());
	InstallingChunkID = ChunkID;
	check(ChunkID > 0);
	InstallingChunkManifest = ChunkManifest;
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	auto PatchField = ChunkManifest->GetCustomField("bIsPatch");
	bool bIsPatch = PatchField.IsValid() ? PatchField->AsString() == TEXT("true") : false;
	auto ChunkFolderName = FString::Printf(TEXT("%s%d"),!bIsPatch ? TEXT("base") : TEXT("patch"), InstallingChunkID);
	auto ChunkInstallDir = FPaths::Combine(*InstallDir,*ChunkFolderName);
	auto ChunkStageDir = FPaths::Combine(*StageDir,*ChunkFolderName);
	if(!PlatformFile.DirectoryExists(*ChunkStageDir))
	{
		PlatformFile.CreateDirectoryTree(*ChunkStageDir);
	}
	if(!PlatformFile.DirectoryExists(*ChunkInstallDir))
	{
		PlatformFile.CreateDirectoryTree(*ChunkInstallDir);
	}
	CloudDir = static_cast<FOnlineTitleFileHttp*>(OnlineTitleFileHttp.Get())->GetBaseUrl();
	BPSModule->SetCloudDirectory(CloudDir);
	BPSModule->SetStagingDirectory(ChunkStageDir);

	TArray<FString> Files = ChunkManifest->GetBuildFileList();
	FString ChunkPak = ChunkInstallDir + TEXT("/") + Files[0];
	ensure(!MountedPaks.Contains(ChunkPak));

	UE_LOG(LogHTTPChunkInstaller, Log, TEXT("Starting Chunk %d install"), InstallingChunkID);
	StartTime = FPlatformTime::Seconds();
	InstallService = BPSModule->StartBuildInstall(PrevInstallChunkManifest,ChunkManifest,ChunkInstallDir,FBuildPatchBoolManifestDelegate::CreateRaw(this,&FHTTPChunkInstall::OSSInstallComplete));
	if(InstallSpeed == EChunkInstallSpeed::Paused && !InstallService->IsPaused())
	{
		InstallService->TogglePauseInstall();
	}
	InstallerState = ChunkInstallState::Installing;
	FPlatformMisc::ControlScreensaver(FGenericPlatformMisc::Disable);
}

/**
 * Note: the following cache functions are synchronous and may need to become asynchronous...
 */
bool FHTTPChunkInstall::AddDataToFileCache(const FString& ManifestHash,const TArray<uint8>& Data)
{
	if (ManifestHash.IsEmpty())
	{
		return false;
	}
	UE_LOG(LogHTTPChunkInstaller, Log, TEXT("Adding data hash %s to file cache"), *ManifestHash);
	return FFileHelper::SaveArrayToFile(Data, *FPaths::Combine(*CacheDir, *ManifestHash));
}

bool FHTTPChunkInstall::IsDataInFileCache(const FString& ManifestHash)
{
	if(ManifestHash.IsEmpty())
	{
		return false;
	}
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	return PlatformFile.FileExists(*FPaths::Combine(*CacheDir, *ManifestHash));
}

bool FHTTPChunkInstall::GetDataFromFileCache(const FString& ManifestHash, TArray<uint8>& Data)
{
	if(ManifestHash.IsEmpty())
	{
		return false;
	}
	UE_LOG(LogHTTPChunkInstaller, Log, TEXT("Reading data hash %s from file cache"), *ManifestHash);
	return FFileHelper::LoadFileToArray(Data, *FPaths::Combine(*CacheDir, *ManifestHash));
}

bool FHTTPChunkInstall::RemoveDataFromFileCache(const FString& ManifestHash)
{
	if(ManifestHash.IsEmpty())
	{
		return false;
	}
	UE_LOG(LogHTTPChunkInstaller, Log, TEXT("Removing data hash %s from file cache"), *ManifestHash);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	auto ManifestPath = FPaths::Combine(*CacheDir, *ManifestHash);
	if (PlatformFile.FileExists(*ManifestPath))
	{
		return PlatformFile.DeleteFile(*ManifestPath);
	}
	return false;
}

void FHTTPChunkInstall::SetupMasterManifest(const FString& InBaseUrl, const FString& ManifestData)
{
	EnumFilesCompleteHandle.Reset();
	ReadFileCompleteHandle.Reset();

	FString TitleFileSource;
	bool bValidTitleFileSource = GConfig->GetString(TEXT("HTTPChunkInstall"), TEXT("TitleFileSource"), TitleFileSource, GEngineIni);
	if (bValidTitleFileSource && TitleFileSource == TEXT("Http"))
	{
		OnlineTitleFile = OnlineTitleFileHttp = MakeShareable(new FOnlineTitleFileHttp(InBaseUrl, ManifestData));
	}
	else if (bValidTitleFileSource && TitleFileSource != TEXT("Local"))
	{
		OnlineTitleFile = Online::GetTitleFileInterface(*TitleFileSource);
	}
	else
	{
		FString LocalTileFileDirectory = FPaths::GameConfigDir();
		auto bGetConfigDir = GConfig->GetString(TEXT("HTTPChunkInstall"), TEXT("LocalTitleFileDirectory"), LocalTileFileDirectory, GEngineIni);
		OnlineTitleFile = MakeShareable(new FLocalTitleFile(LocalTileFileDirectory));
#if !UE_BUILD_SHIPPING
		bDebugNoInstalledRequired = !bGetConfigDir;
#endif
	}

	// parse config
	CloudDirectory = TEXT("");
	CloudDir = FPaths::Combine(*FPaths::GameContentDir(), TEXT("Cloud"));
	StageDir = FPaths::Combine(*FPaths::GameSavedDir(), TEXT("Chunks"), TEXT("Staged"));
	InstallDir = FPaths::Combine(*FPaths::GameSavedDir(), TEXT("Chunks"), TEXT("Installed")); // By default this should match ContentDir
	BackupDir = FPaths::Combine(*FPaths::GameSavedDir(), TEXT("Chunks"), TEXT("Backup"));
	CacheDir = FPaths::Combine(*FPaths::GameSavedDir(), TEXT("Chunks"), TEXT("Cache"));
	HoldingDir = FPaths::Combine(*FPaths::GameSavedDir(), TEXT("Chunks"), TEXT("Hold"));
	ContentDir = FPaths::Combine(*FPaths::GameSavedDir(), TEXT("Chunks"), TEXT("Installed")); // By default this should match InstallDir

	FString TmpString1;
	FString TmpString2;
	if (GConfig->GetString(TEXT("HTTPChunkInstall"), TEXT("CloudDirectory"), TmpString1, GEngineIni))
	{
		CloudDirectory = CloudDir = TmpString1;
#if PLATFORM_ANDROID
		if (FOpenGL::SupportsASTC())
		{
			CloudDirectory = CloudDir = CloudDir.Replace(TEXT("^FORMAT"), TEXT("ASTC"));
		}
		else if (FOpenGL::SupportsATITC())
		{
			CloudDirectory = CloudDir = CloudDir.Replace(TEXT("^FORMAT"), TEXT("ATC"));
		}
		else if (FOpenGL::SupportsDXT())
		{
			CloudDirectory = CloudDir = CloudDir.Replace(TEXT("^FORMAT"), TEXT("DXT"));
		}
		else if (FOpenGL::SupportsPVRTC())
		{
			CloudDirectory = CloudDir = CloudDir.Replace(TEXT("^FORMAT"), TEXT("PVRTC"));
		}
		else if (FOpenGL::SupportsETC2())
		{
			CloudDirectory = CloudDir = CloudDir.Replace(TEXT("^FORMAT"), TEXT("ETC2"));
		}
		else
		{
			CloudDirectory = CloudDir = CloudDir.Replace(TEXT("^FORMAT"), TEXT("ETC1"));
		}
#endif
	}
	if ((GConfig->GetString(TEXT("HTTPChunkInstall"), TEXT("CloudProtocol"), TmpString1, GEngineIni)) && (GConfig->GetString(TEXT("HTTPChunkInstall"), TEXT("CloudDomain"), TmpString2, GEngineIni)))
	{
		if (TmpString1 == TEXT("mcp"))
		{
			CloudDir = static_cast<FOnlineTitleFileHttp*>(OnlineTitleFileHttp.Get())->GetBaseUrl();
		}
		else
		{
			CloudDir = FString::Printf(TEXT("%s://%s"), *TmpString1, *TmpString2);
		}
	}
	if (GConfig->GetString(TEXT("HTTPChunkInstall"), TEXT("StageDirectory"), TmpString1, GEngineIni))
	{
		StageDir = TmpString1;
	}
	if (GConfig->GetString(TEXT("HTTPChunkInstall"), TEXT("InstallDirectory"), TmpString1, GEngineIni))
	{
		InstallDir = TmpString1;
	}
	if (GConfig->GetString(TEXT("HTTPChunkInstall"), TEXT("BackupDirectory"), TmpString1, GEngineIni))
	{
		BackupDir = TmpString1;
	}
	if (GConfig->GetString(TEXT("HTTPChunkInstall"), TEXT("ContentDirectory"), TmpString1, GEngineIni))
	{
		ContentDir = TmpString1;
	}
	if (GConfig->GetString(TEXT("HTTPChunkInstall"), TEXT("HoldingDirectory"), TmpString1, GEngineIni))
	{
		HoldingDir = TmpString1;
	}
}

void FHTTPChunkInstall::Initialize(const FString& InBaseUrl, const FString& ManifestData)
{
	InitialiseSystem(InBaseUrl, ManifestData);
	bEnabled = true;
}

void FHTTPChunkInstall::InitialiseSystem(const FString& InBaseUrl, const FString& ManifestData)
{
	BPSModule = GetBuildPatchServices();

#if !UE_BUILD_SHIPPING
	const TCHAR* CmdLine = FCommandLine::Get();
	if (!FPlatformProperties::RequiresCookedData() || FParse::Param(CmdLine, TEXT("NoPak")) || FParse::Param(CmdLine, TEXT("NoChunkInstall")))
	{
		bDebugNoInstalledRequired = true;
	}
#endif

	// set up the master manifest
	SetupMasterManifest(InBaseUrl, ManifestData);

	// clean up the stage directory to force re-downloading partial data
//	IFileManager::Get().DeleteDirectory(*StageDir, false, true);

	bFirstRun = true;
	bSystemInitialised = true;
}

IBuildManifestPtr FHTTPChunkInstall::FindPreviousInstallManifest(const IBuildManifestPtr& ChunkManifest)
{
	auto ChunkIDField = ChunkManifest->GetCustomField("ChunkID");
	if (!ChunkIDField.IsValid())
	{
		return IBuildManifestPtr();
	}
	auto ChunkID = ChunkIDField->AsInteger();
	TArray<IBuildManifestPtr> FoundManifests;
	PrevInstallManifests.MultiFind(ChunkID, FoundManifests);
	return FoundManifests.Num() == 0 ? IBuildManifestPtr() : FoundManifests[0];
}

void FHTTPChunkInstall::EndInstall()
{
	if (InstallService.IsValid())
	{
		//InstallService->CancelInstall();
		InstallService.Reset();
	}
	InstallingChunkID = -1;
	InstallingChunkManifest.Reset();
	InstallerState = ChunkInstallState::Idle;
}

void FHTTPChunkInstall::FlushPakMount()
{
	// check to see if we need to mount or get some title files
	while (TitleFilesToRead.Num() > 0)
	{
		// check for an already installed manifest
		FString ChunkFdrName;
		FString ManifestName;
		if (BuildChunkFolderName(ChunkFdrName, ManifestName, TitleFilesToRead[0].ChunkID))
		{
			FString ManifestPath = FPaths::Combine(*ContentDir, *ChunkFdrName, *ManifestName);
			FString HoldingPath = FPaths::Combine(*HoldingDir, *ChunkFdrName, *ManifestName);
			IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
			if (PlatformFile.FileExists(*ManifestPath))
			{
				auto Manifest = BPSModule->LoadManifestFromFile(ManifestPath);
				if (!Manifest.IsValid())
				{
					//Something is wrong, suggests corruption so mark the folder for delete
					PlatformFile.DeleteFile(*ManifestPath);
				}
				else
				{
					auto ChunkIDField = Manifest->GetCustomField("ChunkID");
					if (!ChunkIDField.IsValid())
					{
						//Something is wrong, suggests corruption so mark the folder for delete
						PlatformFile.DeleteFile(*ManifestPath);
					}
					else
					{
						InstalledManifests.Add(TitleFilesToRead[0].ChunkID, Manifest);
					}
				}
			}
		}

		if (!IsDataInFileCache(TitleFilesToRead[0].Hash))
		{
			// add to the read list
			FScopeLock Lock(&ReadFileGuard);
			NeedsRead.Add(TitleFilesToRead[0]);
			TitleFilesToRead.RemoveAt(0);
		}
		else
		{
			// read the local file and mount the pak
			FScopeLock Lock(&ReadFileGuard);
			PendingReads.Add(TitleFilesToRead[0]);
			OSSReadFileComplete(true, TitleFilesToRead[0].DLName);
			TitleFilesToRead.RemoveAt(0);
		}
	}

	// force a pak mount right here
	if (PaksToMount.Num() > 0)
	{
		while (!ChunkMountOnlyTask.IsDone())
		{
			FPlatformProcess::Sleep(0.0f);
		}
		MountedPaks.Append(ChunkMountOnlyTask.MountedPaks);
		while (PaksToMount.Num() > 0)
		{
			ChunkMountOnlyTask.AddWork(PaksToMount[0].DestDir, PaksToMount[0].BuildManifest.ToSharedRef(), MountedPaks);
			PaksToMount.RemoveAt(0);
		}
		ChunkMountOnlyTaskThread.Reset(FRunnableThread::Create(&ChunkMountOnlyTask, TEXT("Chunk Mount Only Thread")));
		while (!ChunkMountOnlyTask.IsDone())
		{
			FPlatformProcess::Sleep(0.0f);
		}
		MountedPaks.Append(ChunkMountOnlyTask.MountedPaks);
	}
	else
	{
		while (!ChunkMountOnlyTask.IsDone())
		{
			FPlatformProcess::Sleep(0.0f);
		}
		MountedPaks.Append(ChunkMountOnlyTask.MountedPaks);
	}
}

/**
 * Module for the HTTP Chunk Installer
 */
class FHTTPChunkInstallerModule : public IPlatformChunkInstallModule
{
public:
	TUniquePtr<IPlatformChunkInstall> ChunkInstaller;

	FHTTPChunkInstallerModule()
		: ChunkInstaller(new FHTTPChunkInstall())
	{

	}

	virtual IPlatformChunkInstall* GetPlatformChunkInstall()
	{
		return ChunkInstaller.Get();
	}
};

IMPLEMENT_MODULE(FHTTPChunkInstallerModule, HTTPChunkInstaller);

#undef LOCTEXT_NAMESPACE
