// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MergePrivatePCH.h"

#include "BlueprintEditor.h"
#include "BlueprintEditorModes.h"
#include "IDiffControl.h"
#include "ISourceControlModule.h"
#include "SBlueprintEditorToolbar.h"
#include "SBlueprintMerge.h"
#include "SMergeDetailsView.h"
#include "SMergeGraphView.h"
#include "SMergeTreeView.h"
#include "SNotificationList.h"
#include "NotificationManager.h"

#define LOCTEXT_NAMESPACE "SBlueprintMerge"

SBlueprintMerge::SBlueprintMerge()
	: Data()
	, CurrentDiffControl( nullptr )
	, CurrentMergeControl( nullptr )
{
}

static FString WriteBackup(UPackage& Package, const FString& Directory, const FString& Filename)
{
	if (GEditor)
	{
		const FString DestinationFilename = Directory + FString("/") + Filename;
		FString OriginalFilename;
		if (FPackageName::DoesPackageExist(Package.GetName(), nullptr, &OriginalFilename))
		{
			if (IFileManager::Get().Copy(*DestinationFilename, *OriginalFilename) == COPY_OK)
			{
				return DestinationFilename;
			}
		}
	}
	return FString();
}

static EAppReturnType::Type PromptUserIfUndoBufferToBeCleared(UBlueprint* MergeTarget)
{
	EAppReturnType::Type OkToMerge = EAppReturnType::Yes;
	if (FKismetEditorUtilities::IsReferencedByUndoBuffer(MergeTarget))
	{
		FText TargetName = FText::FromName(MergeTarget->GetFName());

		const FText WarnMessage = FText::Format(LOCTEXT("WarnOfUndoClear",
			"{0} has undo actions associated with it. The undo buffer must be cleared to complete this merge. \n\n\
			You will not be able to undo previous actions after this. Would you like to continue?"), TargetName);
		OkToMerge = FMessageDialog::Open(EAppMsgType::YesNo, WarnMessage);
	}
	return OkToMerge;
}

void SBlueprintMerge::Construct(const FArguments InArgs, const FBlueprintMergeData& InData)
{
	check( InData.OwningEditor.Pin().IsValid() );

	Data = InData;
	OnMergeResolved = InArgs._OnMergeResolved;
	LocalBackupPath.Empty();

	// we want to make a read only copy of the local blueprint, since we allow 
	// the user to modify/mutate the merge result (which starts as the local copy)
	if (InData.BlueprintLocal == GetTargetBlueprint())
	{
		UBlueprint* LocalBlueprint = GetTargetBlueprint();
		// we use this to suppress blueprint compilation during StaticDuplicateObject()
		// @TODO: we don't remember exactly why this was needed, but I saw the 
		//        the blueprint become unplacable and the file become unloadable
		//        without this, but don't know exactly why
		TGuardValue<bool> GuardDuplicatingReadOnly(LocalBlueprint->bDuplicatingReadOnly, true);

		UPackage* TransientPackage = GetTransientPackage();
		Data.BlueprintLocal = Cast<const UBlueprint>( StaticDuplicateObject(LocalBlueprint, TransientPackage,
			*MakeUniqueObjectName(TransientPackage, LocalBlueprint->GetClass(), LocalBlueprint->GetFName()).ToString()) );
	}

	BackupSubDir = FPaths::GameSavedDir() / TEXT("Backup") / TEXT("Resolve_Backup[") + FDateTime::Now().ToString(TEXT("%Y-%m-%d-%H-%M-%S")) + TEXT("]");

	// Because merge operations are so destructive and can be confusing, lets write backups of the files involved:
	WriteBackup(*InData.BlueprintRemote->GetOutermost(), BackupSubDir, TEXT("RemoteAsset") + FPackageName::GetAssetPackageExtension());
	WriteBackup(*InData.BlueprintBase->GetOutermost(), BackupSubDir, TEXT("CommonBaseAsset") + FPackageName::GetAssetPackageExtension());
	LocalBackupPath = WriteBackup(*InData.BlueprintLocal->GetOutermost(), BackupSubDir, TEXT("LocalAsset") + FPackageName::GetAssetPackageExtension());

	auto GraphView = SNew(SMergeGraphView, Data);
	GraphControl.Widget = GraphView;
	GraphControl.DiffControl = &GraphView.Get();
	GraphControl.MergeControl = &GraphView.Get();

	auto TreeView = SNew(SMergeTreeView, Data);
	TreeControl.Widget = TreeView;
	TreeControl.DiffControl = &TreeView.Get();
	TreeControl.MergeControl = &TreeView.Get();

	auto DetailsView = SNew(SMergeDetailsView, Data);
	DetailsControl.Widget = DetailsView;
	DetailsControl.DiffControl = &DetailsView.Get();
	DetailsControl.MergeControl = &DetailsView.Get();

	FToolBarBuilder ToolbarBuilder(TSharedPtr< FUICommandList >(), FMultiBoxCustomization::None);
	ToolbarBuilder.AddToolBarButton( 
		FUIAction( FExecuteAction::CreateSP(this, &SBlueprintMerge::PrevDiff), FCanExecuteAction::CreateSP(this, &SBlueprintMerge::HasPrevDiff) )
		, NAME_None
		, LOCTEXT("PrevMergeLabel", "Prev")
		, LOCTEXT("PrevMergeTooltip", "Go to previous difference")
		, FSlateIcon(FEditorStyle::GetStyleSetName(), "BlueprintMerge.PrevDiff")
	);
	ToolbarBuilder.AddToolBarButton(
		FUIAction(FExecuteAction::CreateSP(this, &SBlueprintMerge::NextDiff), FCanExecuteAction::CreateSP(this, &SBlueprintMerge::HasNextDiff))
		, NAME_None
		, LOCTEXT("NextMergeLabel", "Next")
		, LOCTEXT("NextMergeTooltip", "Go to next difference")
		, FSlateIcon(FEditorStyle::GetStyleSetName(), "BlueprintMerge.NextDiff") 
	);

	// conflict navigation buttons:
	ToolbarBuilder.AddSeparator();
	ToolbarBuilder.AddToolBarButton( 
		FUIAction( FExecuteAction::CreateSP(this, &SBlueprintMerge::PrevConflict), FCanExecuteAction::CreateSP(this, &SBlueprintMerge::HasPrevConflict) )
		, NAME_None
		, LOCTEXT("PrevConflictLabel", "Prev Conflict")
		, LOCTEXT("PrevConflictTooltip", "Go to previous conflict")
		, FSlateIcon(FEditorStyle::GetStyleSetName(), "BlueprintMerge.PrevDiff")
	);
	ToolbarBuilder.AddToolBarButton(
		FUIAction(FExecuteAction::CreateSP(this, &SBlueprintMerge::NextConflict), FCanExecuteAction::CreateSP(this, &SBlueprintMerge::HasNextConflict))
		, NAME_None
		, LOCTEXT("NextConflictLabel", "Next Conflict")
		, LOCTEXT("NextConflictTooltip", "Go to next conflict")
		, FSlateIcon(FEditorStyle::GetStyleSetName(), "BlueprintMerge.NextDiff") 
	);

	// buttons for finishing the merge:
	ToolbarBuilder.AddSeparator(); 
	ToolbarBuilder.AddToolBarButton(
		FUIAction(FExecuteAction::CreateSP(this, &SBlueprintMerge::OnAcceptRemote))
		, NAME_None
		, LOCTEXT("AcceptRemoteLabel", "Accept Source")
		, LOCTEXT("AcceptRemoteTooltip", "Complete the merge operation - Replaces the Blueprint with a copy of the remote file.")
		, FSlateIcon(FEditorStyle::GetStyleSetName(), "BlueprintMerge.AcceptSource")
	);
	ToolbarBuilder.AddToolBarButton(
		FUIAction(FExecuteAction::CreateSP(this, &SBlueprintMerge::OnAcceptLocal))
		, NAME_None
		, LOCTEXT("AcceptLocalLabel", "Accept Target")
		, LOCTEXT("AcceptLocalTooltip", "Complete the merge operation - Leaves the target Blueprint unchanged.")
		, FSlateIcon(FEditorStyle::GetStyleSetName(), "BlueprintMerge.AcceptTarget")
	);
	ToolbarBuilder.AddToolBarButton(
		FUIAction(FExecuteAction::CreateSP(this, &SBlueprintMerge::OnFinishMerge))
		, NAME_None
		, LOCTEXT("FinishMergeLabel", "Finish Merge")
		, LOCTEXT("FinishMergeTooltip", "Complete the merge operation - saves the blueprint and resolves the conflict with the SCC provider")
		, FSlateIcon(FEditorStyle::GetStyleSetName(), "BlueprintMerge.Finish")
	);
	ToolbarBuilder.AddToolBarButton(
		FUIAction(FExecuteAction::CreateSP(this, &SBlueprintMerge::OnCancelClicked))
		, NAME_None
		, LOCTEXT("CancelMergeLabel", "Cancel")
		, LOCTEXT("CancelMergeTooltip", "Abort the merge operation")
		, FSlateIcon(FEditorStyle::GetStyleSetName(), "BlueprintMerge.Cancel")
	);

	ChildSlot [
		SNew( SVerticalBox )
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew( SHorizontalBox )
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				ToolbarBuilder.MakeWidget()
			]
		]
		+ SVerticalBox::Slot()
		[
			SAssignNew(MainView, SBorder)
			.BorderImage(FEditorStyle::GetBrush("NoBorder"))
		]
	];

	Data.OwningEditor.Pin()->OnModeSet().AddSP( this, &SBlueprintMerge::OnModeChanged );
	OnModeChanged( Data.OwningEditor.Pin()->GetCurrentMode() );
}

UBlueprint* SBlueprintMerge::GetTargetBlueprint()
{
	return Data.OwningEditor.Pin()->GetBlueprintObj();
}

void SBlueprintMerge::NextDiff()
{
	CurrentDiffControl->NextDiff();
}

void SBlueprintMerge::PrevDiff()
{
	CurrentDiffControl->PrevDiff();
}

bool SBlueprintMerge::HasNextDiff() const
{
	return CurrentDiffControl && CurrentDiffControl->HasNextDifference();
}

bool SBlueprintMerge::HasPrevDiff() const
{
	return CurrentDiffControl && CurrentDiffControl->HasPrevDifference();
}

void SBlueprintMerge::NextConflict()
{
	CurrentMergeControl->HighlightNextConflict();
}

void SBlueprintMerge::PrevConflict()
{
	CurrentMergeControl->HighlightPrevConflict();
}

bool SBlueprintMerge::HasNextConflict() const
{
	return CurrentMergeControl && CurrentMergeControl->HasNextConflict();
}

bool SBlueprintMerge::HasPrevConflict() const
{
	return CurrentMergeControl && CurrentMergeControl->HasPrevConflict();
}

void SBlueprintMerge::OnFinishMerge()
{
	ResolveMerge(GetTargetBlueprint());
}

void SBlueprintMerge::OnCancelClicked()
{
	// should come before CloseMergeTool(), because CloseMergeTool() makes its
	// own call to OnMergeResolved (with an "Unknown" state).
	OnMergeResolved.ExecuteIfBound(GetTargetBlueprint()->GetOutermost(), EMergeResult::Cancelled);

	// if we're using the merge command-line, it might close everything once
	// a resolution is found (so the editor may be invalid now)
	if (Data.OwningEditor.IsValid())
	{
		Data.OwningEditor.Pin()->CloseMergeTool();
	}
}

void SBlueprintMerge::OnModeChanged(FName NewMode)
{
	if (NewMode == FBlueprintEditorApplicationModes::StandardBlueprintEditorMode ||
		NewMode == FBlueprintEditorApplicationModes::BlueprintMacroMode)
	{
		MainView->SetContent( GraphControl.Widget.ToSharedRef() );
		CurrentDiffControl = GraphControl.DiffControl;
		CurrentMergeControl = GraphControl.MergeControl;
	}
	else if (NewMode == FBlueprintEditorApplicationModes::BlueprintComponentsMode)
	{
		MainView->SetContent(TreeControl.Widget.ToSharedRef());
		CurrentDiffControl = TreeControl.DiffControl;
		CurrentMergeControl = TreeControl.MergeControl;
	}
	else if (NewMode == FBlueprintEditorApplicationModes::BlueprintDefaultsMode ||
			NewMode == FBlueprintEditorApplicationModes::BlueprintInterfaceMode)
	{
		MainView->SetContent(DetailsControl.Widget.ToSharedRef());
		CurrentDiffControl = DetailsControl.DiffControl;
		CurrentMergeControl = DetailsControl.MergeControl;
	}
	else
	{
		ensureMsgf(false, TEXT("Diff panel does not support mode %s"), *NewMode.ToString());
	}
}

void SBlueprintMerge::OnAcceptRemote()
{
	UBlueprint* TargetBlueprint = GetTargetBlueprint();
	if (PromptUserIfUndoBufferToBeCleared(TargetBlueprint) == EAppReturnType::Yes)
	{
		const UBlueprint* RemoteBlueprint = Data.BlueprintRemote;
		if (UBlueprint* NewBlueprint = FKismetEditorUtilities::ReplaceBlueprint(TargetBlueprint, RemoteBlueprint))
		{
			ResolveMerge(NewBlueprint);
		}
	}
}

void SBlueprintMerge::OnAcceptLocal()
{
	UBlueprint* TargetBlueprint = GetTargetBlueprint();
	UPackage* TargetPackage = TargetBlueprint->GetOutermost();

	FString PackageFilename;
	if (FPackageName::DoesPackageExist(TargetPackage->GetName(), /*Guid =*/nullptr, &PackageFilename))
	{
		if (PromptUserIfUndoBufferToBeCleared(TargetBlueprint) == EAppReturnType::Yes)
		{
			FString SrcFilename = LocalBackupPath;
			auto OverwriteBlueprintFile = [TargetBlueprint, &PackageFilename, &SrcFilename](UBlueprint* /*UnloadedBP*/)
			{
				if (IFileManager::Get().Copy(*PackageFilename, *SrcFilename) != COPY_OK)
				{
					const FText TargetName = FText::FromName(TargetBlueprint->GetFName());
					const FText ErrorMessage = FText::Format(LOCTEXT("FailedMergeLocalCopy", "Failed to overwrite {0} target file."), TargetName);
					FSlateNotificationManager::Get().AddNotification(FNotificationInfo(ErrorMessage));
				}
			};
			// we have to wait until the package is unloaded (and the file is 
			// unlocked) before we can overwrite the file; so we use defer the
			// copy until then by adding it to the OnBlueprintUnloaded delegate
			FKismetEditorUtilities::FOnBlueprintUnloaded::FDelegate OnPackageUnloaded = FKismetEditorUtilities::FOnBlueprintUnloaded::FDelegate::CreateLambda(OverwriteBlueprintFile);
			FKismetEditorUtilities::OnBlueprintUnloaded.Add(OnPackageUnloaded);


			if (UBlueprint* NewBlueprint = FKismetEditorUtilities::ReloadBlueprint(TargetBlueprint))
			{
				ResolveMerge(NewBlueprint);
			}
			FKismetEditorUtilities::OnBlueprintUnloaded.Remove(OnPackageUnloaded);
		}
	}
}

void SBlueprintMerge::ResolveMerge(UBlueprint* ResultantBlueprint)
{
	UPackage* Package = ResultantBlueprint->GetOutermost();
	TArray<UPackage*> PackagesToSave;
	PackagesToSave.Add(Package);

	// Perform the resolve with the SCC plugin, we do this first so that the editor doesn't warn about writing to a file that is unresolved:
	ISourceControlModule::Get().GetProvider().Execute(ISourceControlOperation::Create<FResolve>(), SourceControlHelpers::PackageFilenames(PackagesToSave), EConcurrency::Synchronous);

	FEditorFileUtils::EPromptReturnCode const SaveResult = FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, /*bCheckDirty=*/ false, /*bPromptToSave=*/ false);
	if (SaveResult != FEditorFileUtils::PR_Success)
	{
		const FText ErrorMessage = FText::Format(LOCTEXT("MergeWriteFailedError", "Failed to write merged files, please look for backups in {0}"), FText::FromString(BackupSubDir));

		FNotificationInfo ErrorNotification(ErrorMessage);
		FSlateNotificationManager::Get().AddNotification(ErrorNotification);
	}

	// in the case where we have replaced/reloaded the blueprint 
	// ("TargetBlueprint" is most likely, and should be, forcefully closing in 
	// this scenario... we need to open the result to take its place)
	if (ResultantBlueprint != GetTargetBlueprint())
	{
		FAssetEditorManager::Get().OpenEditorForAsset(ResultantBlueprint);
	}
	
	// should come before CloseMergeTool(), because CloseMergeTool() makes its
	// own call to OnMergeResolved (with an "Unknown" state).
	OnMergeResolved.ExecuteIfBound(Package, EMergeResult::Completed);

	// if we're using the merge command-line, it might close everything once
	// a resolution is found (so the editor may be invalid now)
	if (Data.OwningEditor.IsValid())
	{
		Data.OwningEditor.Pin()->CloseMergeTool();
	}
}

#undef LOCTEXT_NAMESPACE
