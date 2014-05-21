// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealSync.h"
#include "GUI.h"
#include "P4DataCache.h"

#include "RequiredProgramMainCPPInclude.h"

DEFINE_LOG_CATEGORY_STATIC(LogUnrealSync, Log, All);

IMPLEMENT_APPLICATION(UnrealSync, "UnrealSync");

/**
 * WinMain, called when the application is started
 */
int WINAPI WinMain(_In_ HINSTANCE hInInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR, _In_ int nCmdShow)
{
	InitGUI();

	return 0;
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
		static void Fill(TArray<FString>& OutLabelNames, const FString& GameName, const TArray<FP4Label>& Labels)
		{
			for (auto Label : Labels)
			{
				if (IsPromotedGameLabelName(Label.GetName(), GameName))
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
		static void Fill(TArray<FString>& OutLabelNames, const FString& GameName, const TArray<FP4Label>& Labels)
		{
			for (auto Label : Labels)
			{
				if (IsPromotedGameLabelName(Label.GetName(), GameName))
				{
					break;
				}

				if (IsPromotableGameLabelName(Label.GetName(), GameName))
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
		static void Fill(TArray<FString>& OutLabelNames, const FString& GameName, const TArray<FP4Label>& Labels)
		{
			for (auto Label : Labels)
			{
				OutLabelNames.Add(Label.GetName());
			}
		}
	};

	return GetLabelsForGame<TFillLabelsPolicy>("");
}

TSharedPtr<TArray<FString> > FUnrealSync::GetPossibleGameNames()
{
	/* TODO: Hardcoded game names. Needs to be fixed! */
	TSharedPtr<TArray<FString> > PossibleGames = MakeShareable(new TArray<FString>());

	PossibleGames->Add("FortniteGame");
	PossibleGames->Add("OrionGame");
	PossibleGames->Add("Shadow");
	PossibleGames->Add("Soul");

	return PossibleGames;
}

const FString& FUnrealSync::GetSharedPromotableDisplayName()
{
	static const FString DispName = "Shared Promotable";

	return DispName;
}

void FUnrealSync::RegisterOnDataRefresh(FOnDataRefresh OnDataRefresh)
{
	FUnrealSync::OnDataRefresh = OnDataRefresh;
}

void FUnrealSync::StartLoadingData()
{
	LoaderThread = MakeShareable(new FP4DataLoader(
		FP4DataLoader::FOnLoadingFinished::CreateStatic(&FUnrealSync::OnP4DataLoadingFinished)
		));
}

void FUnrealSync::TerminateLoadingProcess()
{
	LoaderThread->Terminate();
}

void FUnrealSync::OnP4DataLoadingFinished(TSharedPtr<FP4DataCache> Data)
{
	FUnrealSync::Data = Data;

	if (Data.IsValid())
	{
		bLoadingFinished = true;
	}

	OnDataRefresh.ExecuteIfBound();
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
 * Class to store date needed for sync monitoring thread.
 */
class FSyncingThread : public FRunnable
{
public:
	/**
	 * Constructor
	 *
	 * @param CommandLine Command line to run UAT syncing with.
	 * @param OnSyncFinished Delegate to run when syncing process has finished.
	 * @param OnSyncProgress Delegate to run when syncing process has made progress.
	 */
	FSyncingThread(const FString& CommandLine, const FUnrealSync::FOnSyncFinished& OnSyncFinished, const FUnrealSync::FOnSyncProgress& OnSyncProgress)
		: CommandLine(CommandLine), OnSyncFinished(OnSyncFinished), OnSyncProgress(OnSyncProgress)
	{
	}

	/**
	 * Main thread function.
	 */
	uint32 Run() OVERRIDE
	{
		bool bSuccess = RunUATProgress(CommandLine, OnSyncProgress);
		OnSyncFinished.ExecuteIfBound(bSuccess);

		return 0;
	}

	/**
	 * Clean-up function.
	 */
	void Exit() OVERRIDE
	{
		delete this;
	}

private:
	/* Command line that UAT UnrealSync will be run with. */
	FString CommandLine;

	/* Delegate that will be run when syncing process has finished. */
	FUnrealSync::FOnSyncFinished OnSyncFinished;

	/* Delegate that will be run when syncing process has finished. */
	FUnrealSync::FOnSyncProgress OnSyncProgress;
};

void FUnrealSync::LaunchSync(bool bArtist, bool bPreview, const FString& CommandLine, FOnSyncFinished OnSyncFinished, FOnSyncProgress OnSyncProgress)
{
	FString FinalCommandLine = FString("UnrealSync ") + FString(bArtist ? "-artist " : "") + FString(bPreview ? "-preview " : "") + CommandLine;

	FRunnableThread::Create(new FSyncingThread(FinalCommandLine, OnSyncFinished, OnSyncProgress), TEXT("Syncing thread"));
}

/* Static fields initialization. */
FUnrealSync::FOnDataRefresh FUnrealSync::OnDataRefresh;
TSharedPtr<FP4DataCache> FUnrealSync::Data;
bool FUnrealSync::bLoadingFinished = false;
TSharedPtr<FP4DataLoader> FUnrealSync::LoaderThread;