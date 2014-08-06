// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MergePrivatePCH.h"

#include "BlueprintEditor.h"
#include "BlueprintEditorModes.h"
#include "ISourceControlModule.h"
#include "SBlueprintEditorToolbar.h"
#include "SBlueprintMerge.h"
#include "SMergeGraphView.h"

#define LOCTEXT_NAMESPACE "SBlueprintMerge"

SBlueprintMerge::SBlueprintMerge()
	: Data()
	, GraphView()
	, ComponentsView()
	, DefaultsView()
{
}

void SBlueprintMerge::Construct(const FArguments InArgs, const FBlueprintMergeData& InData)
{
	check( InData.OwningEditor.Pin().IsValid() );

	Data = InData;

	GraphView = SNew( SMergeGraphView, InData );
	ComponentsView = SNew(SBorder);
	DefaultsView = SNew(SBorder);

	InData.OwningEditor.Pin()->OnModeSet().AddSP( this, &SBlueprintMerge::OnModeChanged );
	OnModeChanged( InData.OwningEditor.Pin()->GetCurrentMode() );
}

template< typename T >
static void CopyToShared( const TArray<T>& Source, TArray< TSharedPtr<T> >& Dest )
{
	check( Dest.Num() == 0 );
	for( auto i : Source )
	{
		Dest.Add( MakeShareable(new T( i )) );
	}
}

static void WriteBackup( UPackage& Package, const FString& Directory, const FString& Filename )
{
	if( GEditor )
	{
		const FString DestinationFilename = Directory + FString("/") + Filename;
		FString OriginalFilename;
		if( FPackageName::DoesPackageExist(Package.GetName(), NULL, &OriginalFilename) )
		{
			IFileManager::Get().Copy(*DestinationFilename, *OriginalFilename);
		}
	}
}

UBlueprint* SBlueprintMerge::GetTargetBlueprint()
{
	return Data.OwningEditor.Pin()->GetBlueprintObj();
}

FReply SBlueprintMerge::OnAcceptResultClicked()
{
	// Because merge operations are so destructive and can be confusing, lets write backups of the files involved:
	const FString BackupSubDir = FPaths::GameSavedDir() / TEXT("Backup") / TEXT("Resolve_Backup[") + FDateTime::Now().ToString(TEXT("%Y-%m-%d-%H-%M-%S")) + TEXT("]");
	WriteBackup(*Data.BlueprintNew->GetOutermost(), BackupSubDir, TEXT("RemoteAsset") + FPackageName::GetAssetPackageExtension() );
	WriteBackup(*Data.BlueprintBase->GetOutermost(), BackupSubDir, TEXT("CommonBaseAsset") + FPackageName::GetAssetPackageExtension() );
	WriteBackup(*Data.BlueprintLocal->GetOutermost(), BackupSubDir, TEXT("LocalAsset") + FPackageName::GetAssetPackageExtension());

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

	Data.OwningEditor.Pin()->CloseMergeTool();

	return FReply::Handled();
}

FReply SBlueprintMerge::OnCancelClicked()
{
	Data.OwningEditor.Pin()->CloseMergeTool();
	return FReply::Handled();
}

void SBlueprintMerge::OnModeChanged(FName NewMode)
{
	if (NewMode == FBlueprintEditorApplicationModes::StandardBlueprintEditorMode ||
		NewMode == FBlueprintEditorApplicationModes::BlueprintMacroMode)
	{
		this->ChildSlot
			[
				GraphView.ToSharedRef()
			];
	}
	else if (NewMode == FBlueprintEditorApplicationModes::BlueprintDefaultsMode)
	{
		this->ChildSlot
			[
				DefaultsView.ToSharedRef()
			];
	}
	else if (NewMode == FBlueprintEditorApplicationModes::BlueprintComponentsMode ||
			 NewMode == FBlueprintEditorApplicationModes::BlueprintInterfaceMode)
	{
		this->ChildSlot
			[
				ComponentsView.ToSharedRef()
			];
	}
	else
	{
		ensureMsgf(false, TEXT("Diff panel does not support mode %s"), *NewMode.ToString());
	}
}

#undef LOCTEXT_NAMESPACE
