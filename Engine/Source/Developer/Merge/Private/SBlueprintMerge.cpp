// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MergePrivatePCH.h"
#include "BlueprintEditor.h"
#include "SBlueprintMerge.h"
#include "SMergeGraphView.h"
#include "ISourceControlModule.h"

#define LOCTEXT_NAMESPACE "SBlueprintMerge"

/** 
 * Commands used to navigate the lists of differences between the local revision and the common base
 * or the remote revision and the common base.
 */
class FMergeDifferencesListCommands : public TCommands<FMergeDifferencesListCommands>
{
public:
	FMergeDifferencesListCommands()
		: TCommands<FMergeDifferencesListCommands>("MergeList", NSLOCTEXT("Merge", "Blueprint", "Blueprint Merge"), NAME_None, FEditorStyle::GetStyleSetName())
	{
	}

	/** Go to previous or next difference, focusing on either the remote or local diff */
	TSharedPtr<FUICommandInfo> LocalPrevious;
	TSharedPtr<FUICommandInfo> LocalNext;
	TSharedPtr<FUICommandInfo> RemotePrevious;
	TSharedPtr<FUICommandInfo> RemoteNext;

	virtual void RegisterCommands() override
	{
		UI_COMMAND(LocalPrevious, "LocalPrevious", "Go to previous local difference", EUserInterfaceActionType::Button, FInputGesture(EKeys::F7, EModifierKey::Control));
		UI_COMMAND(LocalNext, "LocalNext", "Go to next local difference", EUserInterfaceActionType::Button, FInputGesture(EKeys::F7));
		UI_COMMAND(RemotePrevious, "RemotePrevious", "Go to previous remote difference", EUserInterfaceActionType::Button, FInputGesture(EKeys::F8, EModifierKey::Control));
		UI_COMMAND(RemoteNext, "RemoteNext", "Go to next remote difference", EUserInterfaceActionType::Button, FInputGesture(EKeys::F8));
	}
};

SBlueprintMerge::SBlueprintMerge()
	: Data()
	, GraphView()
	, ComponentsView()
	, DefaultsView()
{
}

void SBlueprintMerge::Construct(const FArguments InArgs, const FBlueprintMergeData& InData)
{
	FMergeDifferencesListCommands::Register();

	check( InData.OwningEditor.Pin().IsValid() );

	Data = InData;

	GraphView = SNew( SMergeGraphView, InData );

	this->ChildSlot
		[
			GraphView.ToSharedRef()
		];

	/*ComponentsView;
	DefaultsView;*/
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

#undef LOCTEXT_NAMESPACE
