// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "IAssetTools.h"
#include "AssetTypeActions_MovieScene.h"
#include "Toolkits/IToolkit.h"
#include "ISequencerModule.h"
#include "MovieScene.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"


UClass* FAssetTypeActions_MovieScene::GetSupportedClass() const
{
	return UMovieScene::StaticClass(); 
}


void FAssetTypeActions_MovieScene::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	auto MovieScenes = GetTypedWeakObjectPtrs<UMovieScene>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("MovieScene_Edit", "Edit"),
		LOCTEXT("MovieScene_EditTooltip", "Opens the selected movie scenes in the sequencer for editing."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_MovieScene::ExecuteEdit, MovieScenes ),
			FCanExecuteAction()
			)
		);
}


void FAssetTypeActions_MovieScene::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto MovieScene = Cast<UMovieScene>(*ObjIt);
		if (MovieScene != NULL)
		{
			// @todo sequencer: Only allow users to create new MovieScenes if that feature is turned on globally.
			if( FParse::Param( FCommandLine::Get(), TEXT( "Sequencer" ) ) )
			{
				auto NewSequencer = FModuleManager::LoadModuleChecked< ISequencerModule >( "Sequencer" ).CreateSequencer( Mode, EditWithinLevelEditor, MovieScene );
			}
		}
	}
}


void FAssetTypeActions_MovieScene::ExecuteEdit(TArray<TWeakObjectPtr<UMovieScene>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object )
		{
			FAssetEditorManager::Get().OpenEditorForAsset(Object);
		}
	}
}




#undef LOCTEXT_NAMESPACE