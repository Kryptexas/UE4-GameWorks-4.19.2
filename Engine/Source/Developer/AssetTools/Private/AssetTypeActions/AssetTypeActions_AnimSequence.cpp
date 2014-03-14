// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"
#include "ContentBrowserModule.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FAssetTypeActions_AnimSequence::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	FAssetTypeActions_AnimationAsset::GetActions(InObjects, MenuBuilder);

	auto Sequences = GetTypedWeakObjectPtrs<UAnimSequence>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AnimSequence_Reimport", "Reimport"),
		LOCTEXT("AnimSequence_ReimportTooltip", "Reimports the selected sequences."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_AnimSequence::ExecuteReimport, Sequences ),
			FCanExecuteAction()
			)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AnimSequence_FindInExplorer", "Find Source"),
		LOCTEXT("AnimSequence_FindInExplorerTooltip", "Opens explorer at the location of this asset."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_AnimSequence::ExecuteFindInExplorer, Sequences ),
			FCanExecuteAction::CreateSP( this, &FAssetTypeActions_AnimSequence::CanExecuteSourceCommands, Sequences )
			)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AnimSequence_OpenInExternalEditor", "Open Source"),
		LOCTEXT("AnimSequence_OpenInExternalEditorTooltip", "Opens the selected asset in an external editor."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_AnimSequence::ExecuteOpenInExternalEditor, Sequences ),
			FCanExecuteAction::CreateSP( this, &FAssetTypeActions_AnimSequence::CanExecuteSourceCommands, Sequences )
			)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AnimSequence_NewAnimComposite", "Create AnimComposite"),
		LOCTEXT("AnimSequence_NewAnimCompositeTooltip", "Creates an AnimComposite using the selected anim sequence."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_AnimSequence::ExecuteNewAnimComposite, Sequences ),
		FCanExecuteAction()
		)
		);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AnimSequence_NewAnimMontage", "Create AnimMontage"),
		LOCTEXT("AnimSequence_NewAnimMontageTooltip", "Creates an AnimMontage using the selected anim sequence."),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_AnimSequence::ExecuteNewAnimMontage, Sequences ),
		FCanExecuteAction()
		)
		);
}

void FAssetTypeActions_AnimSequence::ExecuteReimport(TArray<TWeakObjectPtr<UAnimSequence>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object )
		{
			FReimportManager::Instance()->Reimport( Object, /*bAskForNewFileIfMissing=*/true );
		}
	}
}

void FAssetTypeActions_AnimSequence::ExecuteFindInExplorer(TArray<TWeakObjectPtr<UAnimSequence>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object && Object->AssetImportData )
		{
			const FString SourceFilePath = FReimportManager::ResolveImportFilename(Object->AssetImportData->SourceFilePath, Object);
			if ( SourceFilePath.Len() && IFileManager::Get().FileSize( *SourceFilePath ) != INDEX_NONE )
			{
				FPlatformProcess::ExploreFolder( *FPaths::GetPath(SourceFilePath) );
			}
		}
	}
}

void FAssetTypeActions_AnimSequence::ExecuteOpenInExternalEditor(TArray<TWeakObjectPtr<UAnimSequence>> Objects)
{
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object && Object->AssetImportData )
		{
			const FString SourceFilePath = FReimportManager::ResolveImportFilename(Object->AssetImportData->SourceFilePath, Object);
			if ( SourceFilePath.Len() && IFileManager::Get().FileSize( *SourceFilePath ) != INDEX_NONE )
			{
				FPlatformProcess::LaunchFileInDefaultExternalApplication( *SourceFilePath );
			}
		}
	}
}

bool FAssetTypeActions_AnimSequence::CanExecuteSourceCommands(TArray<TWeakObjectPtr<UAnimSequence>> Objects) const
{
	bool bHaveSourceAsset = false;
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object && Object->AssetImportData )
		{
			const FString& SourceFilePath = FReimportManager::ResolveImportFilename(Object->AssetImportData->SourceFilePath, Object);

			if ( SourceFilePath.Len() && IFileManager::Get().FileSize(*SourceFilePath) != INDEX_NONE )
			{
				return true;
			}
		}
	}

	return false;
}

void FAssetTypeActions_AnimSequence::ExecuteNewAnimComposite(TArray<TWeakObjectPtr<UAnimSequence>> Objects) const
{
	const FString DefaultSuffix = TEXT("_Composite");
	UAnimCompositeFactory* Factory = ConstructObject<UAnimCompositeFactory>(UAnimCompositeFactory::StaticClass());

	CreateAnimationAssets(Objects, UAnimComposite::StaticClass(), Factory, DefaultSuffix, FOnConfigureFactory::CreateSP(this, &FAssetTypeActions_AnimSequence::ConfigureFactoryForAnimComposite));
}

void FAssetTypeActions_AnimSequence::ExecuteNewAnimMontage(TArray<TWeakObjectPtr<UAnimSequence>> Objects) const
{
	const FString DefaultSuffix = TEXT("_Montage");
	UAnimMontageFactory* Factory = ConstructObject<UAnimMontageFactory>(UAnimMontageFactory::StaticClass());

	CreateAnimationAssets(Objects, UAnimMontage::StaticClass(), Factory, DefaultSuffix, FOnConfigureFactory::CreateSP(this, &FAssetTypeActions_AnimSequence::ConfigureFactoryForAnimMontage));
}

void FAssetTypeActions_AnimSequence::ConfigureFactoryForAnimComposite(UFactory* AssetFactory, UAnimSequence* SourceAnimation) const
{
	UAnimCompositeFactory* CompositeFactory = CastChecked<UAnimCompositeFactory>(AssetFactory);
	CompositeFactory->SourceAnimation = SourceAnimation;
}

void FAssetTypeActions_AnimSequence::ConfigureFactoryForAnimMontage(UFactory* AssetFactory, UAnimSequence* SourceAnimation) const
{
	UAnimMontageFactory* MontageFactory = CastChecked<UAnimMontageFactory>(AssetFactory);
	MontageFactory->SourceAnimation = SourceAnimation;
}

void FAssetTypeActions_AnimSequence::CreateAnimationAssets(const TArray<TWeakObjectPtr<UAnimSequence>>& AnimSequences, TSubclassOf<UAnimationAsset> AssetClass, UFactory* AssetFactory, const FString& InSuffix, FOnConfigureFactory OnConfigureFactory) const
{
	if ( AnimSequences.Num() == 1 )
	{
		auto AnimSequence = AnimSequences[0].Get();

		if ( AnimSequence )
		{
			// Determine an appropriate name for inline-rename
			FString Name;
			FString PackageName;
			CreateUniqueAssetName(AnimSequence->GetOutermost()->GetName(), InSuffix, PackageName, Name);

			OnConfigureFactory.ExecuteIfBound(AssetFactory, AnimSequence);

			FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
			ContentBrowserModule.Get().CreateNewAsset(Name, FPackageName::GetLongPackagePath(PackageName), AssetClass, AssetFactory);
		}
	}
	else
	{
		TArray<UObject*> ObjectsToSync;
		for (auto SeqIt = AnimSequences.CreateConstIterator(); SeqIt; ++SeqIt)
		{
			UAnimSequence* AnimSequence = (*SeqIt).Get();
			if ( AnimSequence )
			{
				// Determine an appropriate name
				FString Name;
				FString PackageName;
				CreateUniqueAssetName(AnimSequence->GetOutermost()->GetName(), InSuffix, PackageName, Name);

				OnConfigureFactory.ExecuteIfBound(AssetFactory, AnimSequence);

				// Create the asset, and assign it's skeleton
				FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
				UAnimationAsset* NewAsset = Cast<UAnimationAsset>(AssetToolsModule.Get().CreateAsset(Name, FPackageName::GetLongPackagePath(PackageName), AssetClass, AssetFactory));

				if ( NewAsset )
				{
					NewAsset->MarkPackageDirty();
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
