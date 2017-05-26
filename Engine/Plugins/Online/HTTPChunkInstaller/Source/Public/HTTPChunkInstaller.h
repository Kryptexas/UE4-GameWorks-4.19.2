// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GenericPlatform/GenericPlatformChunkInstall.h"
#include "Templates/UniquePtr.h"
#include "ChunkInstall.h"
#include "ChunkSetup.h"
#include "Containers/Ticker.h"

// Gross hack:
// If games haven't updated their OSS to IWYU, and they're using the engine version
// of this plugin, they'll trigger IWYU monolithic errors, so we're temporarily
// suppressing those warnings since we can't force-update our games.

// Added 2017-02-26; remove as soon as all of our games have taken an Online plugins update
#pragma push_macro("SUPPRESS_MONOLITHIC_HEADER_WARNINGS")
#  define SUPPRESS_MONOLITHIC_HEADER_WARNINGS 1
#  include "OnlineSubsystem.h"
#pragma pop_macro("SUPPRESS_MONOLITHIC_HEADER_WARNINGS")

struct FBuildInstallStats;

enum class EChunkInstallErrorCode
{
	NoError,
	GenericFailure,
	RemoteFailure, // download errors, mcp errors, etc
	LocalFailure, // disk errors, out of space errors, etc
	NeedsClientUpdate, // manifest says we need a new client
	ManifestDownloadFailure, // manifest failed to download
	ClientIsTooNew, // client is newer than MCP (wrong environment?)
};

struct FChunkInstallError
{
	FChunkInstallError()
		: ErrorCode(EChunkInstallErrorCode::NoError)
	{
	}
	FChunkInstallError(EChunkInstallErrorCode InErrorCode, const FText& InErrorMessage)
		: ErrorCode(InErrorCode)
		, ErrorMessage(InErrorMessage)
	{
	}

	EChunkInstallErrorCode ErrorCode;
	FText ErrorMessage;
};

DECLARE_DELEGATE_OneParam(FPlatformChunkInstallErrorDelegate, const FChunkInstallError&);
DECLARE_DELEGATE_FiveParams(FPlatformChunkInstallProgressDelegate, uint32, float, int64, int64, const FText&);
DECLARE_DELEGATE_FiveParams(FPlatformChunkAnalyticsDelegate, bool, uint32, float, int64, const FBuildInstallStats&);
DECLARE_DELEGATE(FPlatformChunkInstallRetryDelegate);

/**
* HTTP based implementation of chunk based install
**/
class HTTPCHUNKINSTALLER_API FHTTPChunkInstall : public IPlatformChunkInstall, public FTickerObjectBase
{
public:
	FHTTPChunkInstall();
	~FHTTPChunkInstall();

	virtual EChunkLocation::Type GetChunkLocation( uint32 ChunkID ) override;

	virtual bool GetProgressReportingTypeSupported(EChunkProgressReportingType::Type ReportType) override
	{
		if (ReportType == EChunkProgressReportingType::PercentageComplete)
		{
			return true;
		}

		return false;
	}

	virtual float GetChunkProgress( uint32 ChunkID, EChunkProgressReportingType::Type ReportType ) override;

	virtual EChunkInstallSpeed::Type GetInstallSpeed() override
	{
		return InstallSpeed;
	}

	virtual bool SetInstallSpeed( EChunkInstallSpeed::Type InInstallSpeed ) override
	{
		if (InstallSpeed != InInstallSpeed)
		{
			InstallSpeed = InInstallSpeed;
			if (InstallService.IsValid() &&
				((InstallSpeed == EChunkInstallSpeed::Paused && !InstallService->IsPaused()) || (InstallSpeed != EChunkInstallSpeed::Paused && InstallService->IsPaused())))
			{
				InstallService->TogglePauseInstall();
			}
		}
		return true;
	}

	virtual bool PrioritizeChunk( uint32 ChunkID, EChunkPriority::Type Priority ) override;
	virtual FDelegateHandle SetChunkInstallDelgate(uint32 ChunkID, FPlatformChunkInstallCompleteDelegate Delegate) override;
	virtual void RemoveChunkInstallDelgate(uint32 ChunkID, FDelegateHandle Delegate) override;
	FDelegateHandle SetErrorDelegate(FPlatformChunkInstallErrorDelegate Delegate);
	void RemoveErrorDelegate(FDelegateHandle Delegate);
	FDelegateHandle SetChunkInstallProgressDelgate(uint32 ChunkID, FPlatformChunkInstallProgressDelegate Delegate);
	void RemoveChunkInstallProgressDelgate(uint32 ChunkID, FDelegateHandle Delegate);
	FDelegateHandle SetChunkAnalyticsDelegate(FPlatformChunkAnalyticsDelegate Delegate);
	void RemoveChunkAnalyticsDelegate(FDelegateHandle Delegate);
	FDelegateHandle SetRetryDelegate(FPlatformChunkInstallRetryDelegate Delegate);
	void RemoveRetryDelegate(FDelegateHandle Delegate);

	virtual bool DebugStartNextChunk()
	{
		/** Unless paused we are always installing! */
		InstallerState = ChunkInstallState::ReadTitleFiles;
		return false;
	}

	bool Tick(float DeltaSeconds) override;

	void EndInstall();

	void Initialize(const FString& InBaseUrl, const FString& InManifestData);
	bool IsEnabled() const { return bEnabled; }
	bool IsReadyToInstall() const { return InstallerState > ChunkInstallState::PostSetup;  }

	void SetupMasterManifest(const FString& InBaseUrl, const FString& ManifestData);

	inline bool IsRetryAvailable() const { return InstallerState == ChunkInstallState::OfflineMode; }
	void Retry() { check(IsRetryAvailable()); InstallerState = ChunkInstallState::Retry; }

	void SetPreloaded(bool bPreload) { bPreloaded = bPreload; }

	void FlushPakMount();

private:

	void InitialiseSystem(const FString& InBaseUrl, const FString& InManifestData);
	bool AddDataToFileCache(const FString& ManifestHash,const TArray<uint8>& Data);
	bool IsDataInFileCache(const FString& ManifestHash);
	bool GetDataFromFileCache(const FString& ManifestHash,TArray<uint8>& Data);
	bool RemoveDataFromFileCache(const FString& ManifestHash);
	void UpdatePendingInstallQueue();
	IBuildManifestPtr FindPreviousInstallManifest(const IBuildManifestPtr& ChunkManifest);
	void BeginChunkInstall(uint32 NextChunkID,IBuildManifestPtr ChunkManifest, IBuildManifestPtr PrevInstallChunkManifest);
	void ParseTitleFileManifest(const FString& ManifestFileHash);
	bool BuildChunkFolderName(IBuildManifestRef Manifest, FString& ChunkFdrName, FString& ManifestName, uint32& ChunkID, bool& bIsPatch);
	bool BuildChunkFolderName(FString& ChunkFdrName, FString& ManifestName, uint32 ChunkID);
	void OSSEnumerateFilesComplete(bool bSuccess);
	void OSSReadFileComplete(bool bSuccess, const FString& Filename);
	void OSSInstallComplete(bool, IBuildManifestRef);
	bool FilesHaveChanged(IBuildManifestPtr LocalManifest, IBuildManifestPtr RemoteManifest, int32 ChunkID);
	bool IsConnectedToWiFiorLAN() const;

	DECLARE_MULTICAST_DELEGATE_OneParam(FPlatformChunkInstallCompleteMultiDelegate, uint32);
	DECLARE_MULTICAST_DELEGATE_OneParam(FPlatformChunkInstallErrorMultiDelegate, const FChunkInstallError&);
	DECLARE_MULTICAST_DELEGATE_FiveParams(FPlatformChunkInstallProgressMultiDelegate, uint32, float, int64, int64, const FText&);
	DECLARE_MULTICAST_DELEGATE_FiveParams(FPlatformChunkAnalyticsMultiDelegate, bool, uint32, float, int64, const FBuildInstallStats&);
	DECLARE_MULTICAST_DELEGATE(FPlatformChunkInstallRetryMultiDelegate);

	enum struct ChunkInstallState
	{
		Setup,
		SetupWait,
		QueryRemoteManifests,
		MoveInstalledChunks,
		RequestingTitleFiles,
		SearchTitleFiles,
		ReadTitleFiles,
		WaitingOnRead,
		ReadComplete,
		EnterOfflineMode,
		OfflineMode,
		Retry,
		PostSetup,
		Idle,
		Installing,
		CopyToContent,
	};

	struct FChunkPrio
	{
		FChunkPrio(uint32 InChunkID, EChunkPriority::Type InChunkPrio)
			: ChunkID(InChunkID)
			, ChunkPrio(InChunkPrio)
		{

		}

		uint32					ChunkID;
		EChunkPriority::Type	ChunkPrio;

		bool operator == (const FChunkPrio& RHS) const
		{
			return ChunkID == RHS.ChunkID;
		}

		bool operator < (const FChunkPrio& RHS) const
		{
			return ChunkPrio < RHS.ChunkPrio;
		}
	};

	FChunkInstallTask											ChunkCopyInstall;
	TUniquePtr<FRunnableThread>									ChunkCopyInstallThread;
	FChunkSetupTask												ChunkSetupTask;
	TUniquePtr<FRunnableThread>									ChunkSetupTaskThread;
	FChunkMountTask												ChunkMountTask;
	TUniquePtr<FRunnableThread>									ChunkMountTaskThread;
	FChunkMountOnlyTask											ChunkMountOnlyTask;
	TUniquePtr<FRunnableThread>									ChunkMountOnlyTaskThread;
	TMultiMap<uint32, IBuildManifestPtr>						InstalledManifests;
	TMultiMap<uint32, IBuildManifestPtr>						PrevInstallManifests;
	TMultiMap<uint32, IBuildManifestPtr>						RemoteManifests;
	TMap<uint32, FPlatformChunkInstallCompleteMultiDelegate>	DelegateMap;
	TMap<uint32, FPlatformChunkInstallProgressMultiDelegate>	ProgressDelegateMap;
	TSet<FString>												ManifestsInMemory;
	TSet<uint32>												ExpectedChunks;
	TArray<FCloudFileHeader>									TitleFilesToRead;
	TArray<FCloudFileHeader>									RemoteManifestsToBeRead;
	TArray<uint8>												FileContentBuffer;
	TArray<FChunkPrio>											PriorityQueue;
	TArray<FString>												MountedPaks;
	IOnlineTitleFilePtr											OnlineTitleFile;
	IOnlineTitleFilePtr											OnlineTitleFileHttp;
	IBuildInstallerPtr											InstallService;
	IBuildManifestPtr											InstallingChunkManifest;
	FDelegateHandle												EnumFilesCompleteHandle;
	FDelegateHandle												ReadFileCompleteHandle;
	FString														CloudDir;
	FString														CloudDirectory;
	FString														StageDir;
	FString														InstallDir;
	FString														BackupDir;
	FString														ContentDir;
	FString														CacheDir;
	FString														HoldingDir;
	IBuildPatchServicesModule*									BPSModule;
	uint32														InstallingChunkID;
	ChunkInstallState											InstallerState;
	ChunkInstallState											PreviousInstallerState;
	EChunkInstallSpeed::Type									InstallSpeed;
	bool														bFirstRun;
	bool														bSystemInitialised;
#if !UE_BUILD_SHIPPING
	bool														bDebugNoInstalledRequired;
#endif
	FCriticalSection											ReadFileGuard;
	TArray<FCloudFileHeader>									PendingReads;
	TArray<FCloudFileHeader>									NeedsRead;
	FPlatformChunkInstallErrorMultiDelegate						ErrorDelegate;
	bool														bEnabled;
	bool														bNeedsRetry;
	FPlatformChunkAnalyticsMultiDelegate						AnalyticsDelegate;
	double														StartTime;
	FPlatformChunkInstallRetryMultiDelegate						RetryDelegate;
	bool														bPreloaded;
	struct MountTask
	{
		FString DestDir;
		IBuildManifestPtr BuildManifest;
	};
	TArray<MountTask>											PaksToMount;
};