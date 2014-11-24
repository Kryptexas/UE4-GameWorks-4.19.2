// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

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

static void WriteBackup(UPackage& Package, const FString& Directory, const FString& Filename)
{
	if (GEditor)
	{
		const FString DestinationFilename = Directory + FString("/") + Filename;
		FString OriginalFilename;
		if (FPackageName::DoesPackageExist(Package.GetName(), nullptr, &OriginalFilename))
		{
			IFileManager::Get().Copy(*DestinationFilename, *OriginalFilename);
		}
	}
}

void SBlueprintMerge::Construct(const FArguments InArgs, const FBlueprintMergeData& InData)
{
	check( InData.OwningEditor.Pin().IsValid() );

	Data = InData;
	OnMergeResolved = InArgs._OnMergeResolved;

	BackupSubDir = FPaths::GameSavedDir() / TEXT("Backup") / TEXT("Resolve_Backup[") + FDateTime::Now().ToString(TEXT("%Y-%m-%d-%H-%M-%S")) + TEXT("]");

	// Because merge operations are so destructive and can be confusing, lets write backups of the files involved:
	WriteBackup(*Data.BlueprintRemote->GetOutermost(), BackupSubDir, TEXT("RemoteAsset") + FPackageName::GetAssetPackageExtension());
	WriteBackup(*Data.BlueprintBase->GetOutermost(), BackupSubDir, TEXT("CommonBaseAsset") + FPackageName::GetAssetPackageExtension());
	WriteBackup(*Data.BlueprintLocal->GetOutermost(), BackupSubDir, TEXT("LocalAsset") + FPackageName::GetAssetPackageExtension());

	auto GraphView = SNew(SMergeGraphView, InData);
	GraphControl.Widget = GraphView;
	GraphControl.DiffControl = &GraphView.Get();
	GraphControl.MergeControl = &GraphView.Get();

	auto TreeView = SNew(SMergeTreeView, InData);
	TreeControl.Widget = TreeView;
	TreeControl.DiffControl = &TreeView.Get();
	TreeControl.MergeControl = &TreeView.Get();

	auto DetailsView = SNew(SMergeDetailsView, InData);
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

	InData.OwningEditor.Pin()->OnModeSet().AddSP( this, &SBlueprintMerge::OnModeChanged );
	OnModeChanged( InData.OwningEditor.Pin()->GetCurrentMode() );
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
	UPackage* Package = GetTargetBlueprint()->GetOutermost();
	TArray<UPackage*> PackagesToSave;
	PackagesToSave.Add(Package);

	// Perform the resolve with the SCC plugin, we do this first so that the editor doesn't warn about writing to a file that is unresolved:
	ISourceControlModule::Get().GetProvider().Execute(ISourceControlOperation::Create<FResolve>(), SourceControlHelpers::PackageFilenames(PackagesToSave), EConcurrency::Synchronous);

	const auto Result = FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, /*bCheckDirty=*/ false, /*bPromptToSave=*/ false);
	if (Result != FEditorFileUtils::PR_Success)
	{
		static const float MessageTime = 5.0f;
		const FText ErrorMessage = FText::Format( LOCTEXT("MergeWriteFailedError", "Failed to write merged files, please look for backups in {0}"), FText::FromString(BackupSubDir) );

		FNotificationInfo ErrorNotification(ErrorMessage);
		FSlateNotificationManager::Get().AddNotification( ErrorNotification );
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

#undef LOCTEXT_NAMESPACE
