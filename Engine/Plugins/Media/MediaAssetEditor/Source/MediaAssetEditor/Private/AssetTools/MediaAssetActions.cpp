// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MediaAssetEditorPrivatePCH.h"
#include "ContentBrowserModule.h"


#define LOCTEXT_NAMESPACE "AssetTypeActions"


/* FMediaAssetActions structors
 *****************************************************************************/

FMediaAssetActions::FMediaAssetActions( const TSharedRef<ISlateStyle>& InStyle )
	: Style(InStyle)
{ }


/* FAssetTypeActions_Base overrides
 *****************************************************************************/

bool FMediaAssetActions::CanFilter( )
{
	return true;
}


void FMediaAssetActions::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	FAssetTypeActions_Base::GetActions(InObjects, MenuBuilder);

	auto MediaAssets = GetTypedWeakObjectPtrs<UMediaAsset>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("MediaAsset_PlayMovie", "Play Movie"),
		LOCTEXT("MediaAsset_PlayMovieToolTip", "Starts playback of the media."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &FMediaAssetActions::HandlePlayMovieActionExecute, MediaAssets),
			FCanExecuteAction()
		)
	);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("MediaAsset_PauseMovie", "Pause Movie"),
		LOCTEXT("MediaAsset_PauseMovieToolTip", "Pauses playback of the media."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &FMediaAssetActions::HandlePauseMovieActionExecute, MediaAssets),
			FCanExecuteAction()
		)
	);

	MenuBuilder.AddMenuSeparator();

	MenuBuilder.AddMenuEntry(
		LOCTEXT("MediaAsset_CreateMediaSoundWave", "Create Media Sound Wave"),
		LOCTEXT("MediaAsset_CreateMediaSoundWaveTooltip", "Creates a new media sound wave using this media asset."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &FMediaAssetActions::ExecuteCreateMediaSoundWave, MediaAssets),
			FCanExecuteAction()
		)
	);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("MediaAsset_CreateMediaTexture", "Create Media Texture"),
		LOCTEXT("MediaAsset_CreateMediaTextureTooltip", "Creates a new media texture using this media asset."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP(this, &FMediaAssetActions::ExecuteCreateMediaTexture, MediaAssets),
			FCanExecuteAction()
		)
	);

	FAssetTypeActions_Base::GetActions(InObjects, MenuBuilder);
}


uint32 FMediaAssetActions::GetCategories( )
{
	return EAssetTypeCategories::Misc;
}


FText FMediaAssetActions::GetName( ) const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_MediaAsset", "Media Asset");
}


UClass* FMediaAssetActions::GetSupportedClass( ) const
{
	return UMediaAsset::StaticClass();
}


FColor FMediaAssetActions::GetTypeColor( ) const
{
	return FColor(255, 0, 0);
}


bool FMediaAssetActions::HasActions( const TArray<UObject*>& InObjects ) const
{
	return true;
}


void FMediaAssetActions::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor )
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid()
		? EToolkitMode::WorldCentric
		: EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto MediaAsset = Cast<UMediaAsset>(*ObjIt);

		if (MediaAsset != nullptr)
		{
			TSharedRef<FMediaAssetEditorToolkit> EditorToolkit = MakeShareable(new FMediaAssetEditorToolkit(Style));
			EditorToolkit->Initialize(MediaAsset, Mode, EditWithinLevelEditor);
		}
	}
}


/* FAssetTypeActions_MediaAsset callbacks
 *****************************************************************************/

void FMediaAssetActions::ExecuteCreateMediaSoundWave( TArray<TWeakObjectPtr<UMediaAsset>> Objects )
{
	IContentBrowserSingleton& ContentBrowserSingleton = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser").Get();
	const FString DefaultSuffix = TEXT("_Wav");

	if (Objects.Num() == 1)
	{
		auto Object = Objects[0].Get();

		if (Object != nullptr)
		{
			// Determine an appropriate name
			FString Name;
			FString PackagePath;
			CreateUniqueAssetName(Object->GetOutermost()->GetName(), DefaultSuffix, PackagePath, Name);

			// Create the factory used to generate the asset
			UMediaSoundWaveFactoryNew* Factory = ConstructObject<UMediaSoundWaveFactoryNew>(UMediaSoundWaveFactoryNew::StaticClass());
			Factory->InitialMediaAsset = Object;

			ContentBrowserSingleton.CreateNewAsset(Name, FPackageName::GetLongPackagePath(PackagePath), UMediaSoundWave::StaticClass(), Factory);
		}
	}
	else
	{
		TArray<UObject*> ObjectsToSync;

		for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
		{
			auto Object = (*ObjIt).Get();

			if (Object != nullptr)
			{
				FString Name;
				FString PackageName;
				CreateUniqueAssetName(Object->GetOutermost()->GetName(), DefaultSuffix, PackageName, Name);

				// Create the factory used to generate the asset
				UMediaSoundWaveFactoryNew* Factory = ConstructObject<UMediaSoundWaveFactoryNew>(UMediaSoundWaveFactoryNew::StaticClass());
				Factory->InitialMediaAsset = Object;

				FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
				UObject* NewAsset = AssetToolsModule.Get().CreateAsset(Name, FPackageName::GetLongPackagePath(PackageName), UMediaSoundWave::StaticClass(), Factory);

				if (NewAsset != nullptr)
				{
					ObjectsToSync.Add(NewAsset);
				}
			}
		}

		if (ObjectsToSync.Num() > 0)
		{
			ContentBrowserSingleton.SyncBrowserToAssets(ObjectsToSync);
		}
	}
}


void FMediaAssetActions::ExecuteCreateMediaTexture( TArray<TWeakObjectPtr<UMediaAsset>> Objects )
{
	IContentBrowserSingleton& ContentBrowserSingleton = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser").Get();
	const FString DefaultSuffix = TEXT("_Tex");

	if (Objects.Num() == 1)
	{
		auto Object = Objects[0].Get();

		if (Object != nullptr)
		{
			// Determine an appropriate name
			FString Name;
			FString PackagePath;
			CreateUniqueAssetName(Object->GetOutermost()->GetName(), DefaultSuffix, PackagePath, Name);

			// Create the factory used to generate the asset
			UMediaTextureFactoryNew* Factory = ConstructObject<UMediaTextureFactoryNew>(UMediaTextureFactoryNew::StaticClass());
			Factory->InitialMediaAsset = Object;

			ContentBrowserSingleton.CreateNewAsset(Name, FPackageName::GetLongPackagePath(PackagePath), UMediaTexture::StaticClass(), Factory);
		}
	}
	else
	{
		TArray<UObject*> ObjectsToSync;

		for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
		{
			auto Object = (*ObjIt).Get();

			if (Object != nullptr)
			{
				FString Name;
				FString PackageName;
				CreateUniqueAssetName(Object->GetOutermost()->GetName(), DefaultSuffix, PackageName, Name);

				// Create the factory used to generate the asset
				UMediaTextureFactoryNew* Factory = ConstructObject<UMediaTextureFactoryNew>(UMediaTextureFactoryNew::StaticClass());
				Factory->InitialMediaAsset = Object;

				FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
				UObject* NewAsset = AssetToolsModule.Get().CreateAsset(Name, FPackageName::GetLongPackagePath(PackageName), UMediaTexture::StaticClass(), Factory);

				if (NewAsset != nullptr)
				{
					ObjectsToSync.Add(NewAsset);
				}
			}
		}

		if (ObjectsToSync.Num() > 0)
		{
			ContentBrowserSingleton.SyncBrowserToAssets(ObjectsToSync);
		}
	}
}


void FMediaAssetActions::HandlePauseMovieActionExecute( TArray<TWeakObjectPtr<UMediaAsset>> Objects )
{
	for (auto MediaAsset : Objects)
	{
		if (MediaAsset.IsValid())
		{
			MediaAsset->Pause();
		}
	}
}


void FMediaAssetActions::HandlePlayMovieActionExecute( TArray<TWeakObjectPtr<UMediaAsset>> Objects )
{
	for (auto MediaAsset : Objects)
	{
		if (MediaAsset.IsValid())
		{
			MediaAsset->Play();
		}
	}
}


#undef LOCTEXT_NAMESPACE
