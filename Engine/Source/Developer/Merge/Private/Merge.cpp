// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MergePrivatePCH.h"

#include "BlueprintEditor.h"
#include "ISourceControlModule.h"
#include "SBlueprintMerge.h"
#include "Toolkits/ToolkitManager.h"
#include "Toolkits/IToolkit.h"
#include "SDockTab.h"
#include "SNotificationList.h"
#include "NotificationManager.h"

#define LOCTEXT_NAMESPACE "Merge"

const FName MergeToolTabId = FName(TEXT("MergeTool"));

static void DisplayErrorMessage( const FText& ErrorMessage )
{
	FNotificationInfo Info(ErrorMessage);
	Info.ExpireDuration = 5.0f;
	FSlateNotificationManager::Get().AddNotification(Info);
}

static FSourceControlStatePtr GetSourceControlState(const FString& PackageName)
{
	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();
	TSharedRef<FUpdateStatus, ESPMode::ThreadSafe> UpdateStatusOperation = ISourceControlOperation::Create<FUpdateStatus>();
	UpdateStatusOperation->SetUpdateHistory(true);
	SourceControlProvider.Execute(UpdateStatusOperation, SourceControlHelpers::PackageFilename(PackageName));

	FSourceControlStatePtr State = SourceControlProvider.GetState(SourceControlHelpers::PackageFilename(PackageName), EStateCacheUsage::Use);
	if (!State.IsValid() || !State->IsSourceControlled() || !FPackageName::DoesPackageExist(PackageName))
	{
		return FSourceControlStatePtr();
	}
	else
	{
		return State;
	}
}

static UObject* LoadRevision(const FString& AssetName, const ISourceControlRevision& DesiredRevision)
{
	// Get the head revision of this package from source control
	FString TempFileName;
	if (DesiredRevision.Get(TempFileName))
	{
		// Forcibly disable compile on load in case we are loading old blueprints that might try to update/compile
		TGuardValue<bool> DisableCompileOnLoad(GForceDisableBlueprintCompileOnLoad, true);

		// Try and load that package
		UPackage* TempPackage = LoadPackage(NULL, *TempFileName, LOAD_None);
		if (TempPackage != NULL)
		{
			// Grab the old asset from that old package
			UObject* OldObject = FindObject<UObject>(TempPackage, *AssetName);
			if (OldObject)
			{
				return OldObject;
			}
			else
			{
				DisplayErrorMessage( 
					FText::Format(
						LOCTEXT("MergedFailedToFindObject", "Aborted Load of {0} because we could not find an object named {1}" )
						, FText::FromString(TempFileName)
						, FText::FromString(AssetName) 
					)
				);
			}
		}
		else
		{
			DisplayErrorMessage(
				FText::Format(
					LOCTEXT("MergedFailedToLoadPackage", "Aborted Load of {0} because we could not load the package")
					, FText::FromString(TempFileName)
				)
			);
		}
	}
	else
	{
		DisplayErrorMessage(
			FText::Format(
				LOCTEXT("MergedFailedToFindRevision", "Aborted Load of {0} because we could not get the requested revision")
				, FText::FromString(TempFileName)
			)
		);
	}
	return NULL;
}

static FRevisionInfo GetRevisionInfo(ISourceControlRevision const& FromRevision)
{
	FRevisionInfo Ret = { FromRevision.GetRevisionNumber(), FromRevision.GetCheckInIdentifier(), FromRevision.GetDate() };
	return Ret;
}

static UObject* LoadHeadRev(const FString& PackageName, const FString& AssetName, const ISourceControlState& SourceControlState, FRevisionInfo& OutRevInfo)
{
	// HistoryItem(0) is apparently the head revision:
	TSharedPtr<ISourceControlRevision, ESPMode::ThreadSafe> Revision = SourceControlState.GetHistoryItem(0);
	check(Revision.IsValid());
	OutRevInfo = GetRevisionInfo(*Revision);
	return LoadRevision(AssetName, *Revision);
}

static UObject* LoadBaseRev(const FString& PackageName, const FString& AssetName, const ISourceControlState& SourceControlState, FRevisionInfo& OutRevInfo)
{
	TSharedPtr<ISourceControlRevision, ESPMode::ThreadSafe> Revision = SourceControlState.GetBaseRevForMerge();
	check(Revision.IsValid());
	OutRevInfo = GetRevisionInfo(*Revision);
	return LoadRevision(AssetName, *Revision);
}

static TSharedPtr<SWidget> GenerateMergeTabContents(TSharedRef<FBlueprintEditor> Editor,
                                                    const UBlueprint*       BaseBlueprint,
                                                    const FRevisionInfo&    BaseRevInfo,
													const UBlueprint*       RemoteBlueprint,
													const FRevisionInfo&    RemoteRevInfo,
													const UBlueprint*       LocalBlueprint,
													const FOnMergeResolved& MereResolutionCallback)
{
	TSharedPtr<SWidget> TabContent;

	if (RemoteBlueprint && BaseBlueprint)
	{
		FBlueprintMergeData Data(Editor
			, LocalBlueprint
			, BaseBlueprint
			, RemoteRevInfo
			, RemoteBlueprint
			, BaseRevInfo);

		TabContent = SNew(SBlueprintMerge, Data)
			.OnMergeResolved(MereResolutionCallback);
	}
	else
	{
		FText MissingFiles;
		if (!RemoteBlueprint && !BaseBlueprint)
		{
			MissingFiles = LOCTEXT("MergeBothRevisionsFailed", "common base, nor conflicting revision");
		}
		else if (!RemoteBlueprint)
		{
			MissingFiles = LOCTEXT("MergeConflictingRevisionsFailed", "conflicting revision");
		}
		else
		{
			MissingFiles = LOCTEXT("MergeBaseRevisionsFailed", "common base");
		}

		DisplayErrorMessage(
			FText::Format(
				LOCTEXT("MergeRevisionLoadFailed", "Aborted Merge of {0} because we could not load {1}")
				, FText::FromString(LocalBlueprint->GetName())
				, MissingFiles
			)
		);

		TabContent = SNew(SHorizontalBox);
	}

	return TabContent;
}

class FMerge : public IMerge
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual TSharedRef<SDockTab> GenerateMergeWidget(const UBlueprint& Object, TSharedRef<FBlueprintEditor> Editor) override;
	virtual TSharedRef<SDockTab> GenerateMergeWidget(const UBlueprint* BaseAsset, const UBlueprint* RemoteAsset, const UBlueprint* LocalAsset, const FOnMergeResolved& MergeResolutionCallback, TSharedRef<FBlueprintEditor> Editor) override;
	virtual bool PendingMerge(const UBlueprint& BlueprintObj) const override;
	//virtual FOnMergeResolved& OnMergeResolved() const override;

	// Simplest to only allow one merge operation at a time, we could easily make this a map of Blueprint=>MergeTab
	// but doing so will complicate the tab management
	TWeakPtr<SDockTab> ActiveTab;
};
IMPLEMENT_MODULE( FMerge, Merge )

void FMerge::StartupModule()
{
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		MergeToolTabId, 
		FOnSpawnTab::CreateStatic( [](const FSpawnTabArgs&) { return SNew(SDockTab); } ) )
		.SetDisplayName(NSLOCTEXT("MergeTool", "TabTitle", "Merge Tool"))
		.SetTooltipText(NSLOCTEXT("MergeTool", "TooltipText", "Used to display several versions of a blueprint that need to be merged into a single version.")
	);
}

void FMerge::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(MergeToolTabId);
}

TSharedRef<SDockTab> FMerge::GenerateMergeWidget(const UBlueprint& Object, TSharedRef<FBlueprintEditor> Editor)
{
	auto ActiveTabPtr = ActiveTab.Pin();
	if( ActiveTabPtr.IsValid() )
	{
		// just bring the tab to the foreground:
		auto CurrentTab = FGlobalTabmanager::Get()->InvokeTab(MergeToolTabId);
		check( CurrentTab == ActiveTabPtr );
		return ActiveTabPtr.ToSharedRef();
	}

	// merge the local asset with the depot, SCC provides us with the last common revision as
	// a basis for the merge:

	// @todo DO: this will probably need to be async.. pulling down some old versions of assets:
	const FString& PackageName = Object.GetOutermost()->GetName();
	const FString& AssetName = Object.GetName();
	
	TSharedPtr<SWidget> Contents;

	FSourceControlStatePtr SourceControlState = GetSourceControlState(PackageName);
	if (!SourceControlState.IsValid())
	{
		DisplayErrorMessage(
			FText::Format(
				LOCTEXT("MergeFailedNoSourceControl", "Aborted Load of {0} from {1} because the source control state was invalidated")
				, FText::FromString(AssetName)
				, FText::FromString(PackageName)
			)
		);

		Contents = SNew(SHorizontalBox);
	}
	else
	{
		ISourceControlState const& SourceControlStateRef = *SourceControlState;

		FRevisionInfo CurrentRevInfo = FRevisionInfo::InvalidRevision();
		const UBlueprint* RemoteBlueprint = Cast< UBlueprint >(LoadHeadRev(PackageName, AssetName, SourceControlStateRef, CurrentRevInfo));
		FRevisionInfo BaseRevInfo = FRevisionInfo::InvalidRevision();
		const UBlueprint* BaseBlueprint = Cast< UBlueprint >(LoadBaseRev(PackageName, AssetName, SourceControlStateRef, BaseRevInfo));

		Contents = GenerateMergeTabContents(Editor, BaseBlueprint, BaseRevInfo, RemoteBlueprint, CurrentRevInfo, &Object, FOnMergeResolved());
	}

	TSharedRef<SDockTab> Tab =  FGlobalTabmanager::Get()->InvokeTab(MergeToolTabId);
	Tab->SetContent(Contents.ToSharedRef());
	ActiveTab = Tab;
	return Tab;

}

TSharedRef<SDockTab> FMerge::GenerateMergeWidget(const UBlueprint* BaseBlueprint, const UBlueprint* RemoteBlueprint, const UBlueprint* LocalBlueprint, const FOnMergeResolved& MergeResolutionCallback, TSharedRef<FBlueprintEditor> Editor)
{
	if (ActiveTab.IsValid())
	{
		TSharedPtr<SDockTab> ActiveTabPtr = ActiveTab.Pin();
		// just bring the tab to the foreground:
		TSharedRef<SDockTab> CurrentTab = FGlobalTabmanager::Get()->InvokeTab(MergeToolTabId);
		check(CurrentTab == ActiveTabPtr);
		return ActiveTabPtr.ToSharedRef();
	}

	// @TODO: pipe revision info through
	TSharedPtr<SWidget> TabContents = GenerateMergeTabContents(Editor, BaseBlueprint, FRevisionInfo::InvalidRevision(), RemoteBlueprint, FRevisionInfo::InvalidRevision(), LocalBlueprint, MergeResolutionCallback);

	TSharedRef<SDockTab> Tab = FGlobalTabmanager::Get()->InvokeTab(MergeToolTabId);
	Tab->SetContent(TabContents.ToSharedRef());
	ActiveTab = Tab;
	return Tab;
}

bool FMerge::PendingMerge(const UBlueprint& BlueprintObj) const
{
	bool bIsMergeEnabled = GetDefault<UEditorExperimentalSettings>()->bEnableBlueprintMergeTool;
	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();

	bool bPendingMerge = false;
	if( bIsMergeEnabled && SourceControlProvider.IsEnabled() )
	{
		FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(BlueprintObj.GetOutermost(), EStateCacheUsage::Use);
		bPendingMerge = SourceControlState.IsValid() && SourceControlState->IsConflicted();
	}
	return bPendingMerge;
}

#undef LOCTEXT_NAMESPACE
