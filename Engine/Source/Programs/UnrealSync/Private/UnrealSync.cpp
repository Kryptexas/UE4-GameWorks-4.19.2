// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealSync.h"
#include "GUI.h"
#include "P4DataCache.h"
#include "P4Env.h"
#include "XmlParser.h"

#include "RequiredProgramMainCPPInclude.h"

DEFINE_LOG_CATEGORY_STATIC(LogUnrealSync, Log, All);

IMPLEMENT_APPLICATION(UnrealSync, "UnrealSync");

bool FUnrealSync::UpdateOriginalUS(const FString& OriginalUSPath)
{
	FString Output;
	return FP4Env::RunP4Output(TEXT("sync ") + OriginalUSPath, Output);
}

bool FUnrealSync::RunDetachedUS(const FString& USPath, bool bDoNotRunFromCopy, bool bDoNotUpdateOnStartUp, bool bPassP4Env)
{
	FString CommandLine = FString()
		+ (bDoNotRunFromCopy ? TEXT("-DoNotRunFromCopy ") : TEXT(""))
		+ (bDoNotUpdateOnStartUp ? TEXT("-DoNotUpdateOnStartUp ") : TEXT(""))
		+ (FUnrealSync::IsDebugParameterSet() ? TEXT("-Debug ") : TEXT(""))
		+ (bPassP4Env ? *FP4Env::Get().GetCommandLine() : TEXT(""));

	return RunProcess(USPath, CommandLine);
}

bool FUnrealSync::DeleteIfExistsAndCopy(const FString& To, const FString& From)
{
	auto& PlatformPhysical = IPlatformFile::GetPlatformPhysical();

	if (PlatformPhysical.FileExists(*To) && !PlatformPhysical.DeleteFile(*To))
	{
		UE_LOG(LogUnrealSync, Warning, TEXT("Deleting existing file '%s' failed."), *To);
		return false;
	}
	else if (PlatformPhysical.DirectoryExists(*To) && !PlatformPhysical.DeleteDirectoryRecursively(*To))
	{
		return false;
	}

	if (PlatformPhysical.FileExists(*From))
	{
		return PlatformPhysical.CopyFile(*To, *From);
	}
	else if (PlatformPhysical.DirectoryExists(*From))
	{
		return PlatformPhysical.CopyDirectoryTree(*To, *From, true);
	}

	return false;
}

void FUnrealSync::RunUnrealSync(const TCHAR* CommandLine)
{
	// start up the main loop
	GEngineLoop.PreInit(CommandLine);

	if (!FP4Env::Init(CommandLine))
	{
		GLog->Flush();
		InitGUI(CommandLine, true);
	}
	else
	{
		if (FUnrealSync::Initialization(CommandLine))
		{
			InitGUI(CommandLine);
		}

		if (!FUnrealSync::GetInitializationError().IsEmpty())
		{
			UE_LOG(LogUnrealSync, Fatal, TEXT("Error: %s"), *FUnrealSync::GetInitializationError());
		}
		else
		{
			GIsRequestingExit = true;
		}
	}
}

FString FUnrealSync::GetLatestLabelForGame(const FString& GameName)
{
	auto Labels = GetPromotedLabelsForGame(GameName);
	return Labels->Num() == 0 ? "" : (*Labels)[0];
}

/**
 * Tells if label is in the format: <BranchPath>/<Prefix>-<GameNameIfNotEmpty>-CL-*
 *
 * @param LabelName Label name to check.
 * @param LabelNamePrefix Prefix to use.
 * @param GameName Game name to use.
 *
 * @returns True if label is in the format. False otherwise.
 */
bool IsPrefixedGameLabelName(const FString& LabelName, const FString& LabelNamePrefix, const FString& GameName)
{
	int32 BranchNameEnd = 0;

	if (!LabelName.FindLastChar('/', BranchNameEnd))
	{
		return false;
	}

	FString Rest = LabelName.Mid(BranchNameEnd + 1);

	FString LabelNameGamePart;
	if (!GameName.Equals(""))
	{
		LabelNameGamePart = "-" + GameName;
	}

	return Rest.StartsWith(LabelNamePrefix + LabelNameGamePart + "-CL-", ESearchCase::CaseSensitive);
}

/**
 * Tells if label is in the format: <BranchPath>/Promoted-<GameNameIfNotEmpty>-CL-*
 *
 * @param LabelName Label name to check.
 * @param GameName Game name to use.
 *
 * @returns True if label is in the format. False otherwise.
 */
bool IsPromotedGameLabelName(const FString& LabelName, const FString& GameName)
{
	return IsPrefixedGameLabelName(LabelName, "Promoted", GameName);
}

/**
 * Tells if label is in the format: <BranchPath>/Promotable-<GameNameIfNotEmpty>-CL-*
 *
 * @param LabelName Label name to check.
 * @param GameName Game name to use.
 *
 * @returns True if label is in the format. False otherwise.
 */
bool IsPromotableGameLabelName(const FString& LabelName, const FString& GameName)
{
	return IsPrefixedGameLabelName(LabelName, "Promotable", GameName);
}

/**
 * Helper function to construct and get array of labels if data is fetched from the P4
 * using given policy.
 *
 * @template_param TFillLabelsPolicy Policy class that has to implement:
 *     static void Fill(TArray<FString>& OutLabelNames, const FString& GameName, const TArray<FP4Label>& Labels)
 *     that will fill the array on request.
 * @param GameName Game name to pass to the Fill method from policy.
 * @returns Filled out labels.
 */
template <class TFillLabelsPolicy>
TSharedPtr<TArray<FString> > GetLabelsForGame(const FString& GameName)
{
	TSharedPtr<TArray<FString> > OutputLabels = MakeShareable(new TArray<FString>());

	if (FUnrealSync::HasValidData())
	{
		TFillLabelsPolicy::Fill(*OutputLabels, GameName, FUnrealSync::GetLabels());
	}

	return OutputLabels;
}

TSharedPtr<TArray<FString> > FUnrealSync::GetPromotedLabelsForGame(const FString& GameName)
{
	struct TFillLabelsPolicy 
	{
		static void Fill(TArray<FString>& OutLabelNames, const FString& InGameName, const TArray<FP4Label>& InLabels)
		{
			for (auto Label : InLabels)
			{
				if (IsPromotedGameLabelName(Label.GetName(), InGameName))
				{
					OutLabelNames.Add(Label.GetName());
				}
			}
		}
	};

	return GetLabelsForGame<TFillLabelsPolicy>(GameName);
}

TSharedPtr<TArray<FString> > FUnrealSync::GetPromotableLabelsForGame(const FString& GameName)
{
	struct TFillLabelsPolicy
	{
		static void Fill(TArray<FString>& OutLabelNames, const FString& InGameName, const TArray<FP4Label>& InLabels)
		{
			for (auto Label : InLabels)
			{
				if (IsPromotedGameLabelName(Label.GetName(), InGameName))
				{
					break;
				}

				if (IsPromotableGameLabelName(Label.GetName(), InGameName))
				{
					OutLabelNames.Add(Label.GetName());
				}
			}
		}
	};

	return GetLabelsForGame<TFillLabelsPolicy>(GameName);
}

TSharedPtr<TArray<FString> > FUnrealSync::GetAllLabels()
{
	struct TFillLabelsPolicy
	{
		static void Fill(TArray<FString>& OutLabelNames, const FString& InGameName, const TArray<FP4Label>& InLabels)
		{
			for (auto Label : InLabels)
			{
				OutLabelNames.Add(Label.GetName());
			}
		}
	};

	return GetLabelsForGame<TFillLabelsPolicy>("");
}

TSharedPtr<TArray<FString> > FUnrealSync::GetPossibleGameNames()
{
	TSharedPtr<TArray<FString> > PossibleGames = MakeShareable(new TArray<FString>());

	FP4Env& Env = FP4Env::Get();

	FString FileList;
	if (!Env.RunP4Output(FString::Printf(TEXT("files -e %s/.../Build/ArtistSyncRules.xml"), *Env.GetBranch()), FileList) || FileList.IsEmpty())
	{
		return PossibleGames;
	}

	FString Line, Rest = FileList;
	while (Rest.Split(LINE_TERMINATOR, &Line, &Rest, ESearchCase::CaseSensitive))
	{
		if (!Line.StartsWith(Env.GetBranch()))
		{
			continue;
		}

		int32 ArtistSyncRulesPos = Line.Find("/Build/ArtistSyncRules.xml#", ESearchCase::IgnoreCase);

		if (ArtistSyncRulesPos == INDEX_NONE)
		{
			continue;
		}

		FString MiddlePart = Line.Mid(Env.GetBranch().Len(), ArtistSyncRulesPos - Env.GetBranch().Len());

		int32 LastSlash = INDEX_NONE;
		MiddlePart.FindLastChar('/', LastSlash);

		PossibleGames->Add(MiddlePart.Mid(LastSlash + 1));
	}

	return PossibleGames;
}

const FString& FUnrealSync::GetSharedPromotableDisplayName()
{
	static const FString DispName = "Shared Promotable";

	return DispName;
}

const FString& FUnrealSync::GetSharedPromotableP4FolderName()
{
	static const FString DispName = "Samples";

	return DispName;
}

void FUnrealSync::RegisterOnDataLoaded(const FOnDataLoaded& InOnDataLoaded)
{
	FUnrealSync::OnDataLoaded = InOnDataLoaded;
}

void FUnrealSync::RegisterOnDataReset(const FOnDataReset& InOnDataReset)
{
	FUnrealSync::OnDataReset = InOnDataReset;
}

void FUnrealSync::StartLoadingData()
{
	bLoadingFinished = false;
	Data.Reset();
	LoaderThread.Reset();

	OnDataReset.ExecuteIfBound();

	LoaderThread = MakeShareable(new FP4DataLoader(
		FP4DataLoader::FOnLoadingFinished::CreateStatic(&FUnrealSync::OnP4DataLoadingFinished)
		));
}

void FUnrealSync::TerminateLoadingProcess()
{
	if (LoaderThread.IsValid())
	{
		LoaderThread->Terminate();
	}
}

void FUnrealSync::OnP4DataLoadingFinished(TSharedPtr<FP4DataCache> InData)
{
	FUnrealSync::Data = InData;

	bLoadingFinished = true;

	OnDataLoaded.ExecuteIfBound();
}

bool FUnrealSync::LoadingFinished()
{
	return bLoadingFinished;
}

bool FUnrealSync::HasValidData()
{
	return Data.IsValid();
}

/**
 * Class to store info of syncing thread.
 */
class FSyncingThread : public FRunnable
{
public:
	/**
	 * Constructor
	 *
	 * @param InSettings Sync settings.
	 * @param InLabelNameProvider Label name provider.
	 * @param InOnSyncFinished Delegate to run when syncing process has finished.
	 * @param InOnSyncProgress Delegate to run when syncing process has made progress.
	 */
	FSyncingThread(FSyncSettings InSettings, ILabelNameProvider& InLabelNameProvider, const FUnrealSync::FOnSyncFinished& InOnSyncFinished, const FUnrealSync::FOnSyncProgress& InOnSyncProgress)
		: bTerminate(false), Settings(MoveTemp(InSettings)), LabelNameProvider(InLabelNameProvider), OnSyncFinished(InOnSyncFinished), OnSyncProgress(InOnSyncProgress)
	{
		Thread = FRunnableThread::Create(this, TEXT("Syncing thread"));
	}

	/**
	 * Main thread function.
	 */
	uint32 Run() override
	{
		if (OnSyncProgress.IsBound())
		{
			OnSyncProgress.Execute(LabelNameProvider.GetStartedMessage() + "\n");
		}

		FString Label = LabelNameProvider.GetLabelName();
		FString Game = LabelNameProvider.GetGameName();

		struct FProcessStopper
		{
			FProcessStopper(bool& bInStop, FUnrealSync::FOnSyncProgress& InOuterSyncProgress)
				: bStop(bInStop), OuterSyncProgress(InOuterSyncProgress) {}

			bool OnProgress(const FString& Text)
			{
				if (OuterSyncProgress.IsBound())
				{
					if (!OuterSyncProgress.Execute(Text))
					{
						bStop = true;
					}
				}

				return !bStop;
			}

		private:
			bool& bStop;
			FUnrealSync::FOnSyncProgress& OuterSyncProgress;
		};
		
		FProcessStopper Stopper(bTerminate, OnSyncProgress);
		bool bSuccess = FUnrealSync::Sync(Settings, Label, Game, FUnrealSync::FOnSyncProgress::CreateRaw(&Stopper, &FProcessStopper::OnProgress));

		if (OnSyncProgress.IsBound())
		{
			OnSyncProgress.Execute(LabelNameProvider.GetFinishedMessage() + "\n");
		}

		OnSyncFinished.ExecuteIfBound(bSuccess);

		return 0;
	}

	/**
	 * Stops process runnning in the background and terminates wait for the
	 * watcher thread to finish.
	 */
	void Terminate()
	{
		bTerminate = true;
		Thread->WaitForCompletion();
	}

private:
	/* Tells the thread to terminate the process. */
	bool bTerminate;

	/* Handle for thread object. */
	FRunnableThread* Thread;

	/* Sync settings. */
	FSyncSettings Settings;

	/* Label name provider. */
	ILabelNameProvider& LabelNameProvider;

	/* Delegate that will be run when syncing process has finished. */
	FUnrealSync::FOnSyncFinished OnSyncFinished;

	/* Delegate that will be run when syncing process has finished. */
	FUnrealSync::FOnSyncProgress OnSyncProgress;
};

void FUnrealSync::TerminateSyncingProcess()
{
	if (SyncingThread.IsValid())
	{
		SyncingThread->Terminate();
	}
}

void FUnrealSync::LaunchSync(FSyncSettings Settings, ILabelNameProvider& LabelNameProvider, const FOnSyncFinished& OnSyncFinished, const FOnSyncProgress& OnSyncProgress)
{
	SyncingThread = MakeShareable(new FSyncingThread(MoveTemp(Settings), LabelNameProvider, OnSyncFinished, OnSyncProgress));
}

/**
 * Helper function to send message through sync progress delegate.
 *
 * @param OnSyncProgress Delegate to use.
 * @param Message Message to send.
 */
void SyncingMessage(const FUnrealSync::FOnSyncProgress& OnSyncProgress, const FString& Message)
{
	if (OnSyncProgress.IsBound())
	{
		OnSyncProgress.Execute(Message + "\n");
	}
}

/**
 * Contains file and revision specification as sync step.
 */
struct FSyncStep
{
	FSyncStep(FString InFileSpec, FString InRevSpec)
		: FileSpec(MoveTemp(InFileSpec)), RevSpec(MoveTemp(InRevSpec))
	{}

	const FString& GetFileSpec() const { return FileSpec; }
	const FString& GetRevSpec() const { return RevSpec; }

private:
	FString FileSpec;
	FString RevSpec;
};

/**
 * Parses sync rules XML content and fills array with sync steps
 * appended with content and program revision specs.
 *
 * @param SyncSteps Array to append.
 * @param SyncRules String with sync rules XML.
 * @param ContentRevisionSpec Content revision spec.
 * @param ProgramRevisionSpec Program revision spec.
 */
void FillWithSyncSteps(TArray<FSyncStep>& SyncSteps, const FString& SyncRules, const FString& ContentRevisionSpec, const FString& ProgramRevisionSpec)
{
	TSharedRef<FXmlFile> Doc = MakeShareable(new FXmlFile(SyncRules, EConstructMethod::ConstructFromBuffer));

	for (FXmlNode* ChildNode : Doc->GetRootNode()->GetChildrenNodes())
	{
		if (ChildNode->GetTag() != "Rules")
		{
			continue;
		}

		for (FXmlNode* Rule : ChildNode->GetChildrenNodes())
		{
			if (Rule->GetTag() != "string")
			{
				continue;
			}

			auto SyncStep = Rule->GetContent();

			SyncStep = SyncStep.Replace(TEXT("%LABEL_TO_SYNC_TO%"), *ProgramRevisionSpec);

			// If there was no label in sync step.
			// TODO: This is hack for messy ArtistSyncRules.xml format.
			// Needs to be changed soon.
			if (!SyncStep.Contains("@"))
			{
				SyncStep += ContentRevisionSpec;
			}

			int32 AtLastPos = INDEX_NONE;
			SyncStep.FindLastChar('@', AtLastPos);

			int32 HashLastPos = INDEX_NONE;
			SyncStep.FindLastChar('#', HashLastPos);

			int32 RevSpecSeparatorPos = FMath::Max<int32>(AtLastPos, HashLastPos);

			check(RevSpecSeparatorPos != INDEX_NONE); // At least one rev specifier needed here.
			
			SyncSteps.Add(FSyncStep(SyncStep.Mid(0, RevSpecSeparatorPos), SyncStep.Mid(RevSpecSeparatorPos)));
		}
	}
}

/**
 * Gets latest P4 submitted changelist.
 *
 * @param CL Output parameter. Found changelist.
 *
 * @returns True if succeeded. False otherwise.
 */
static bool GetLatestChangelist(FString& CL)
{
	FString Left;
	FString Temp;
	FString Rest;

	if (!FP4Env::RunP4Output("changes -s submitted -m1", Temp)
		|| !Temp.Split(" on ", &Left, &Rest)
		|| !Left.Split("Change ", &Rest, &Temp))
	{
		return false;
	}

	CL = Temp;
	return true;
}

bool FUnrealSync::Sync(const FSyncSettings& Settings, const FString& Label, const FString& Game, const FOnSyncProgress& OnSyncProgress)
{
	SyncingMessage(OnSyncProgress, "Syncing to label: " + Label);

	auto ProgramRevisionSpec = "@" + Label;
	TArray<FSyncStep> SyncSteps;

	if (Settings.bArtist)
	{
		SyncingMessage(OnSyncProgress, "Performing artist sync.");
		
		// Get latest CL number to sync cause @head can change during
		// different syncs and it could create integrity problems in
		// workspace.
		FString CL;
		if (!GetLatestChangelist(CL))
		{
			return false;
		}

		SyncingMessage(OnSyncProgress, "Latest CL found: " + CL);

		auto ContentRevisionSpec = "@" + CL;

		auto ArtistSyncRulesPath = FString::Printf(TEXT("%s/%s/Build/ArtistSyncRules.xml"),
			*FP4Env::Get().GetBranch(), Game.IsEmpty() ? TEXT("Samples") : *Game);

		FString SyncRules;
		if (!FP4Env::RunP4Output("print -q " + ArtistSyncRulesPath + "#head", SyncRules) || SyncRules.IsEmpty())
		{
			return false;
		}

		SyncingMessage(OnSyncProgress, "Got sync rules from: " + ArtistSyncRulesPath);

		FillWithSyncSteps(SyncSteps, SyncRules, ContentRevisionSpec, ProgramRevisionSpec);
	}
	else
	{
		SyncSteps.Add(FSyncStep(Settings.OverrideSyncStep.IsEmpty() ? "/..." : Settings.OverrideSyncStep, ProgramRevisionSpec)); // all files to label
	}

	class FSyncCollectAndPassThrough
	{
	public:
		FSyncCollectAndPassThrough(const FOnSyncProgress& InProgress)
			: Progress(InProgress)
		{ }

		operator FOnSyncProgress() const
		{
			return FOnSyncProgress::CreateRaw(this, &FSyncCollectAndPassThrough::OnProgress);
		}

		bool OnProgress(const FString& Text)
		{
			Log += Text;

			return Progress.Execute(Text);
		}

		void ProcessLog()
		{
			Log = Log.Replace(TEXT("\r"), TEXT(""));
			const FRegexPattern CantClobber(TEXT("Can't clobber writable file ([^\\n]+)"));

			FRegexMatcher Match(CantClobber, Log);

			while (Match.FindNext())
			{
				CantClobbers.Add(Match.GetCaptureGroup(1));
			}
		}

		const TArray<FString>& GetCantClobbers() const { return CantClobbers; }

	private:
		const FOnSyncProgress& Progress;

		FString Log;

		TArray<FString> CantClobbers;
	};

	FString CommandPrefix = FString("sync ") + (Settings.bPreview ? "-n " : "") + FP4Env::Get().GetBranch();
	for(const auto& SyncStep : SyncSteps)
	{
		FSyncCollectAndPassThrough ErrorsCollector(OnSyncProgress);
		if (!FP4Env::RunP4Progress(CommandPrefix + SyncStep.GetFileSpec() + SyncStep.GetRevSpec(), ErrorsCollector))
		{
			if (!Settings.bAutoClobber)
			{
				return false;
			}

			ErrorsCollector.ProcessLog();

			FString AutoClobberCommandPrefix("sync -f ");
			for (const auto& CantClobber : ErrorsCollector.GetCantClobbers())
			{
				if (!FP4Env::RunP4Progress(AutoClobberCommandPrefix + CantClobber + SyncStep.GetRevSpec(), OnSyncProgress))
				{
					return false;
				}
			}
		}
	}

	return true;
}

bool FUnrealSync::IsDebugParameterSet()
{
	const auto bDebug = FParse::Param(FCommandLine::Get(), TEXT("Debug"));

	return bDebug;
}

bool FUnrealSync::Initialization(const TCHAR* CommandLine)
{
	const TCHAR* Platform =
#if PLATFORM_WINDOWS
	#if PLATFORM_64BITS
		TEXT("Win64")
	#else
		TEXT("Win32")
	#endif
#elif PLATFORM_MAC
		TEXT("Mac")
#elif PLATFORM_LINUX
		TEXT("Linux")
#endif
		;

	FString CommonExecutablePath =
		FPaths::ConvertRelativePathToFull(
		FPaths::Combine(*FPaths::EngineDir(), TEXT("Binaries"), Platform,
#if !UE_BUILD_DEBUG
		TEXT("UnrealSync")
#else
		*FString::Printf(TEXT("UnrealSync-%s-Debug"), Platform)
#endif
		));

	const TCHAR* AppPostfix =
#if PLATFORM_WINDOWS
		TEXT(".exe")
#elif PLATFORM_MAC
		TEXT(".app")
#elif PLATFORM_LINUX
		TEXT("")
#endif
		;

	FString OriginalExecutablePath = CommonExecutablePath + AppPostfix;
	FString TemporaryExecutablePath = CommonExecutablePath + TEXT(".Temporary") + AppPostfix;

	bool bDoNotRunFromCopy = FParse::Param(CommandLine, TEXT("DoNotRunFromCopy"));
	bool bDoNotUpdateOnStartUp = FParse::Param(CommandLine, TEXT("DoNotUpdateOnStartUp"));

	if (!bDoNotRunFromCopy)
	{
		if (!DeleteIfExistsAndCopy(*TemporaryExecutablePath, *OriginalExecutablePath))
		{
			InitializationError = TEXT("Copying UnrealSync to temp location failed.");
		}
		else if (!RunDetachedUS(TemporaryExecutablePath, true, bDoNotUpdateOnStartUp, true))
		{
			InitializationError = TEXT("Running remote UnrealSync failed.");
		}

		return false;
	}

	if (!bDoNotUpdateOnStartUp && FP4Env::CheckIfFileNeedsUpdate(OriginalExecutablePath))
	{
		if (!UpdateOriginalUS(OriginalExecutablePath))
		{
			InitializationError = TEXT("UnrealSync update failed.");
		}
		else if (!RunDetachedUS(OriginalExecutablePath, false, true, true))
		{
			InitializationError = TEXT("Running UnrealSync failed.");
		}

		return false;
	}

	return true;
}


/* Static fields initialization. */
FUnrealSync::FOnDataLoaded FUnrealSync::OnDataLoaded;
FUnrealSync::FOnDataReset FUnrealSync::OnDataReset;
TSharedPtr<FP4DataCache> FUnrealSync::Data;
bool FUnrealSync::bLoadingFinished = false;
TSharedPtr<FP4DataLoader> FUnrealSync::LoaderThread;
TSharedPtr<FSyncingThread> FUnrealSync::SyncingThread;
FString FUnrealSync::InitializationError;
