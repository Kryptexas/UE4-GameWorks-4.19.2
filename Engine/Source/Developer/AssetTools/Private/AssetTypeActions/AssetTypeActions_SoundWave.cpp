// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"
#include "AssetToolsModule.h"
#include "SoundDefinitions.h"
#include "Editor/UnrealEd/Public/Dialogs/DlgSoundWaveOptions.h"
#include "ContentBrowserModule.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

UClass* FAssetTypeActions_SoundWave::GetSupportedClass() const
{
	return USoundWave::StaticClass();
}

void FAssetTypeActions_SoundWave::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	FAssetTypeActions_SoundBase::GetActions(InObjects, MenuBuilder);

	auto SoundNodes = GetTypedWeakObjectPtrs<USoundWave>(InObjects);

	/*MenuBuilder.AddMenuEntry(
		LOCTEXT("SoundWave_CompressionPreview", "Compression Preview"),
		LOCTEXT("SoundWave_CompressionPreviewTooltip", "Opens the selected sound node waves in the compression preview window."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_SoundWave::ExecuteCompressionPreview, SoundNodes ),
			FCanExecuteAction()
			)
		);*/

	MenuBuilder.AddMenuEntry(
		LOCTEXT("SoundWave_Reimport", "Reimport"),
		LOCTEXT("SoundWave_ReimportTooltip", "Reimports the selected waves from file."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_SoundWave::ExecuteReimport, SoundNodes ),
			FCanExecuteAction()
			)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("SoundWave_FindInExplorer", "Find Source"),
		LOCTEXT("SoundWave_FindInExplorerTooltip", "Opens explorer at the location of this asset."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_SoundWave::ExecuteFindInExplorer, SoundNodes ),
			FCanExecuteAction::CreateSP( this, &FAssetTypeActions_SoundWave::CanExecuteSourceCommands, SoundNodes )
			)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("SoundWave_OpenInExternalEditor", "Open Source"),
		LOCTEXT("SoundWave_OpenInExternalEditorTooltip", "Opens the selected asset in an external editor."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_SoundWave::ExecuteOpenInExternalEditor, SoundNodes ),
			FCanExecuteAction::CreateSP( this, &FAssetTypeActions_SoundWave::CanExecuteSourceCommands, SoundNodes )
			)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("SoundWave_CreateCue", "Create Cue"),
		LOCTEXT("SoundWave_CreateCueTooltip", "Creates a sound cue ."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_SoundWave::ExecuteCreateSoundCue, SoundNodes ),
		FCanExecuteAction()
		)
	);
}

void FAssetTypeActions_SoundWave::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	FSimpleAssetEditor::CreateEditor(EToolkitMode::Standalone, EditWithinLevelEditor, InObjects);

/*	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto SoundWave = Cast<USoundWave>(*ObjIt);
		if (SoundWave != NULL)
		{
			if( SoundWave->ChannelSizes.Num() == 0 && !SSoundWaveCompressionOptions::IsQualityPreviewerActive() && !GIsAutomationTesting )
			{
				TSharedPtr< SWindow > Window;
				TSharedPtr< SSoundWaveCompressionOptions > Widget;

				Window = SNew(SWindow)
					.Title(NSLOCTEXT("SoundWaveOptions", "SoundWaveOptions_Title", "Sound Wave Compression Preview"))
					.SizingRule( ESizingRule::Autosized )
					.SupportsMaximize(false) .SupportsMinimize(false)
					[
						SNew( SBorder )
						.Padding( 4.f )
						.BorderImage( FEditorStyle::GetBrush( "ToolPanel.GroupBorder" ) )
						[
							SAssignNew(Widget, SSoundWaveCompressionOptions)
								.SoundWave(SoundWave)
						]
					];

				Widget->SetWindow(Window);

				FSlateApplication::Get().AddWindow( Window.ToSharedRef() );
			}
			else if( SoundWave->ChannelSizes.Num() > 0 )
			{
				FMessageDialog::Open( EAppMsgType::Ok, LOCTEXT("SoundQualityPreviewerUnavailable", "Sound quality preview is unavailable for multichannel sounds.") );
			}
		}
	}*/
}

void FAssetTypeActions_SoundWave::ExecuteCompressionPreview(TArray<TWeakObjectPtr<USoundWave>> Objects)
{
/*	TArray<UObject*> ObjectsToEdit;
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object )
		{
			ObjectsToEdit.Add(Object);
		}
	}

	OpenAssetEditor(ObjectsToEdit);*/
}

void FAssetTypeActions_SoundWave::ExecuteReimport(TArray<TWeakObjectPtr<USoundWave>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		USoundWave* SoundWave = (*ObjIt).Get();
		if ( SoundWave )
		{
			FReimportManager::Instance()->Reimport(SoundWave, /*bAskForNewFileIfMissing=*/true);
		}
	}
}

void FAssetTypeActions_SoundWave::ExecuteFindInExplorer(TArray<TWeakObjectPtr<USoundWave>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object )
		{
			const FString SourceFilePath = FReimportManager::ResolveImportFilename(Object->SourceFilePath, Object);
			if ( SourceFilePath.Len() && IFileManager::Get().FileSize( *SourceFilePath ) != INDEX_NONE )
			{
				FPlatformProcess::ExploreFolder( *FPaths::GetPath(SourceFilePath) );
			}
		}
	}
}

void FAssetTypeActions_SoundWave::ExecuteOpenInExternalEditor(TArray<TWeakObjectPtr<USoundWave>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object )
		{
			const FString SourceFilePath = FReimportManager::ResolveImportFilename(Object->SourceFilePath, Object);
			if ( SourceFilePath.Len() && IFileManager::Get().FileSize( *SourceFilePath ) != INDEX_NONE )
			{
				FPlatformProcess::LaunchFileInDefaultExternalApplication( *SourceFilePath );
			}
		}
	}
}

bool FAssetTypeActions_SoundWave::CanExecuteSourceCommands(TArray<TWeakObjectPtr<USoundWave>> Objects) const
{
	bool bHaveSourceAsset = false;
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object )
		{
			const FString& SourceFilePath = FReimportManager::ResolveImportFilename(Object->SourceFilePath, Object);

			if ( SourceFilePath.Len() && IFileManager::Get().FileSize(*SourceFilePath) != INDEX_NONE )
			{
				return true;
			}
		}
	}

	return false;
}

void FAssetTypeActions_SoundWave::ExecuteCreateSoundCue(TArray<TWeakObjectPtr<USoundWave>> Objects)
{
	const FString DefaultSuffix = TEXT("_Cue");

	if ( Objects.Num() == 1 )
	{
		auto Object = Objects[0].Get();

		if ( Object )
		{
			// Determine an appropriate name
			FString Name;
			FString PackagePath;
			CreateUniqueAssetName(Object->GetOutermost()->GetName(), DefaultSuffix, PackagePath, Name);

			// Create the factory used to generate the asset
			USoundCueFactoryNew* Factory = ConstructObject<USoundCueFactoryNew>(USoundCueFactoryNew::StaticClass());
			Factory->InitialSoundWave = Object;

			FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			ContentBrowserModule.Get().CreateNewAsset(Name, FPackageName::GetLongPackagePath(PackagePath), USoundCue::StaticClass(), Factory);
		}
	}
	else
	{
		TArray<UObject*> ObjectsToSync;

		for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
		{
			auto Object = (*ObjIt).Get();
			if ( Object )
			{
				FString Name;
				FString PackageName;
				CreateUniqueAssetName(Object->GetOutermost()->GetName(), DefaultSuffix, PackageName, Name);

				// Create the factory used to generate the asset
				USoundCueFactoryNew* Factory = ConstructObject<USoundCueFactoryNew>(USoundCueFactoryNew::StaticClass());
				Factory->InitialSoundWave = Object;

				FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
				UObject* NewAsset = AssetToolsModule.Get().CreateAsset(Name, FPackageName::GetLongPackagePath(PackageName), USoundCue::StaticClass(), Factory);

				if ( NewAsset )
				{
					ObjectsToSync.Add(NewAsset);
				}
			}
		}

		if ( ObjectsToSync.Num() > 0 )
		{
			FAssetTools::Get().SyncBrowserToAssets(ObjectsToSync);
		}
	}
}

#undef LOCTEXT_NAMESPACE
