// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MergePrivatePCH.h"

#include "BlueprintEditor.h"
#include "ISourceControlModule.h"
#include "SBlueprintMerge.h"
#include "Toolkits/ToolkitManager.h"
#include "Toolkits/IToolkit.h"

#define LOCTEXT_NAMESPACE "Merge"

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
				// @todo DO: toast? i love toast..
			}
		}
		else
		{
			// @todo DO: toast? i love toast..
		}
	}
	else
	{
		// @todo DO: toast? i love toast..
	}
	return NULL;
}

static FRevisionInfo GetRevisionInfo(ISourceControlRevision const& FromRevision)
{
	FRevisionInfo Ret = { FromRevision.GetRevisionNumber(), FromRevision.GetCheckInIdentifier(), FromRevision.GetDate() };
	return Ret;
}

static UObject* LoadHeadRev(const FString& PackageName, const FString& AssetName, FRevisionInfo& OutRevInfo)
{
	FSourceControlStatePtr SourceControlState = GetSourceControlState(PackageName);
	if (SourceControlState.IsValid())
	{
		// HistoryItem(0) is apparently the head revision:
		TSharedPtr<ISourceControlRevision, ESPMode::ThreadSafe> Revision = SourceControlState->GetHistoryItem(0);
		check(Revision.IsValid());
		OutRevInfo = GetRevisionInfo(*Revision);
		return LoadRevision(AssetName, *Revision);
	}
	else
	{
		// @todo DO: toast? i love toast..
	}

	return NULL;
}

static UObject* LoadBaseRev(const FString& PackageName, const FString& AssetName, FRevisionInfo& OutRevInfo)
{
	FSourceControlStatePtr SourceControlState = GetSourceControlState(PackageName);
	if (SourceControlState.IsValid())
	{
		TSharedPtr<ISourceControlRevision, ESPMode::ThreadSafe> Revision = SourceControlState->GetBaseRevForMerge();
		check(Revision.IsValid());
		OutRevInfo = GetRevisionInfo(*Revision);
		return LoadRevision(AssetName, *Revision);
	}
	else
	{
		// @todo DO: toast? i love toast..
	}
	return NULL;
}

class FMerge : public IMerge
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual TSharedPtr<SWidget> GenerateMergeWidget(const UBlueprint& Object, TSharedRef<FBlueprintEditor> Editor) override;
	virtual bool PendingMerge(const UBlueprint& BlueprintObj) const override;
};
IMPLEMENT_MODULE( FMerge, Merge )

void FMerge::StartupModule()
{
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
}

void FMerge::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

TSharedPtr<SWidget> FMerge::GenerateMergeWidget(const UBlueprint& Object, TSharedRef<FBlueprintEditor> Editor)
{
	// merge the local asset with the depot, SCC provides us with the last common revision as
	// a basis for the merge:

	// @todo DO: this will probably need to be async.. pulling down some old versions of assets:
	const FString& PackageName = Object.GetOutermost()->GetName();
	const FString& AssetName = Object.GetName();

	FRevisionInfo CurrentRevInfo = FRevisionInfo::InvalidRevision();
	UBlueprint* RemoteBlueprint = Cast< UBlueprint >( LoadHeadRev(PackageName, AssetName, CurrentRevInfo) );
	FRevisionInfo BaseRevInfo = FRevisionInfo::InvalidRevision();
	UBlueprint* BaseBlueprint = Cast< UBlueprint >( LoadBaseRev(PackageName, AssetName, BaseRevInfo) );

	if (RemoteBlueprint && BaseBlueprint)
	{
		FMergeDisplayArgs DisplayArgs = { CurrentRevInfo, BaseRevInfo };

		SBlueprintDiff::FArguments BaseArgs;
		BaseArgs.BlueprintOld(BaseBlueprint)
			.OldRevision(CurrentRevInfo)
			.BlueprintNew(RemoteBlueprint)
			.NewRevision(BaseRevInfo);

		return SNew(SBlueprintMerge)
			.BlueprintLocal(const_cast<UBlueprint*>(&Object))
			.OwningEditor(Editor)
			.BaseArgs(BaseArgs);
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

		FText ErrorMessage = FText::Format(LOCTEXT("MergeRevisionLoadFailed", "Aborted Merge of {0} because we could not load {1}"), FText::FromString(Object.GetName()), MissingFiles);
		FNotificationInfo Info(ErrorMessage);
		Info.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
		return TSharedPtr<SWidget>();
	}
}

bool FMerge::PendingMerge(const UBlueprint& BlueprintObj) const
{
	bool bIsMergeEnabled = false;
	GConfig->GetBool(TEXT("AssetMerge"), TEXT("EnableAssetMerge"), bIsMergeEnabled, GEngineIni);
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
