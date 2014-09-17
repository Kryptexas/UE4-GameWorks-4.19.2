// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"
#include "Toolkits/IToolkitHost.h"
#include "PersonaModule.h"
#include "EditorAnimUtils.h"
#include "SSkeletonWidget.h"
#include "ObjectEditorUtils.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FAssetTypeActions_AnimationAsset::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	auto AnimAssets = GetTypedWeakObjectPtrs<UAnimationAsset>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AnimSequenceBase_FindSkeleton", "Find Skeleton"),
		LOCTEXT("AnimSequenceBase_FindSkeletonTooltip", "Finds the skeleton for the selected assets in the content browser."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_AnimationAsset::ExecuteFindSkeleton, AnimAssets ),
			FCanExecuteAction()
			)
		);

	MenuBuilder.AddSubMenu( 
		LOCTEXT("RetargetAnimSubmenu", "Retarget Anim Assets"),
		LOCTEXT("RetargetAnimSubmenu_ToolTip", "Opens the retarget anim assets menu"),
		FNewMenuDelegate::CreateSP( this, &FAssetTypeActions_AnimationAsset::FillRetargetMenu, InObjects ) );
}

void FAssetTypeActions_AnimationAsset::FillRetargetMenu( FMenuBuilder& MenuBuilder, const TArray<UObject*> InObjects )
{
	bool bAllSkeletonsNull = true;

	for(auto Iter = InObjects.CreateConstIterator(); Iter; ++Iter)
	{
		if(UAnimationAsset* AnimAsset = Cast<UAnimationAsset>(*Iter))
		{
			if(AnimAsset->GetSkeleton())
			{
				bAllSkeletonsNull = false;
				break;
			}
		}
	}

	if(bAllSkeletonsNull)
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("AnimAsset_RetargetSkeletonInPlace", "Retarget skeleton on existing Anim Assets"),
			LOCTEXT("AnimAsset_RetargetSkeletonInPlaceTooltip", "Retargets the selected Anim Assets to a new skeleton (and optionally all referenced animations too)"),
			FSlateIcon(),
			FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_AnimationAsset::RetargetAssets, InObjects, false ), // false = do not duplicate assets first
			FCanExecuteAction()
			)
			);
	}

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AnimAsset_DuplicateAndRetargetSkeleton", "Duplicate Anim Assets and Retarget"),
		LOCTEXT("AnimAsset_DuplicateAndRetargetSkeletonTooltip", "Duplicates and then retargets the selected Anim Assets to a new skeleton (and optionally all referenced animations too)"),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_AnimationAsset::RetargetAssets, InObjects, true ), // true = duplicate assets and retarget them
		FCanExecuteAction()
		)
		);
}

UThumbnailInfo* FAssetTypeActions_AnimationAsset::GetThumbnailInfo(UObject* Asset) const
{
	UAnimationAsset* Anim = CastChecked<UAnimationAsset>(Asset);
	UThumbnailInfo* ThumbnailInfo = Anim->ThumbnailInfo;
	if (ThumbnailInfo == NULL)
	{
		ThumbnailInfo = ConstructObject<USceneThumbnailInfo>(USceneThumbnailInfo::StaticClass(), Anim);
		Anim->ThumbnailInfo = ThumbnailInfo;
	}

	return ThumbnailInfo;
}

void FAssetTypeActions_AnimationAsset::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto AnimAsset = Cast<UAnimationAsset>(*ObjIt);
		if (AnimAsset != NULL)
		{
			USkeleton * AnimSkeleton = AnimAsset->GetSkeleton();
			if (!AnimSkeleton)
			{
				FText ShouldRetargetMessage = LOCTEXT("ShouldRetargetAnimAsset_Message", "Could not find the skeleton for Anim '{AnimName}' Would you like to choose a new one?");

				FFormatNamedArguments Arguments;
				Arguments.Add( TEXT("AnimName"), FText::FromString(AnimAsset->GetName()));

				if ( FMessageDialog::Open(EAppMsgType::YesNo, FText::Format(ShouldRetargetMessage, Arguments)) == EAppReturnType::Yes )
				{
					bool bDuplicateAssets = false;
					TArray<UObject*> AnimAssets;
					AnimAssets.Add(AnimAsset);
					RetargetAssets(AnimAssets, bDuplicateAssets);
				}
			}

			AnimSkeleton = AnimAsset->GetSkeleton();
			if (AnimSkeleton)
			{
				const bool bBringToFrontIfOpen = false;
				if (IAssetEditorInstance* EditorInstance = FAssetEditorManager::Get().FindEditorForAsset(AnimSkeleton, bBringToFrontIfOpen))
				{
					// The skeleton is already open in an editor.
					// Tell persona that an animation asset was requested
					EditorInstance->FocusWindow(AnimAsset);
				}
				else
				{
					FPersonaModule& PersonaModule = FModuleManager::LoadModuleChecked<FPersonaModule>( "Persona" );
					PersonaModule.CreatePersona(Mode, EditWithinLevelEditor, AnimSkeleton, NULL, AnimAsset, NULL);
				}
			}
			else
			{
				FMessageDialog::Open( EAppMsgType::Ok, LOCTEXT("FailedToLoadSkeletonlessAnimAsset", "The Anim Asset could not be loaded because it's skeleton is missing."));
			}
		}
	}
}

void FAssetTypeActions_AnimationAsset::ExecuteFindSkeleton(TArray<TWeakObjectPtr<UAnimationAsset>> Objects)
{
	TArray<UObject*> ObjectsToSync;
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object )
		{
			USkeleton* Skeleton = Object->GetSkeleton();
			if (Skeleton)
			{
				ObjectsToSync.AddUnique(Skeleton);
			}
		}
	}

	if ( ObjectsToSync.Num() > 0 )
	{
		FAssetTools::Get().SyncBrowserToAssets(ObjectsToSync);
	}
}

void FAssetTypeActions_AnimationAsset::RetargetAnimationHandler(USkeleton* OldSkeleton, USkeleton* NewSkeleton, bool bRemapReferencedAssets, bool bConvertSpaces, bool bDuplicateAssets,  TArray<TWeakObjectPtr<UObject>> InAnimAssets)
{
	EditorAnimUtils::RetargetAnimations(OldSkeleton, NewSkeleton, InAnimAssets, bRemapReferencedAssets, bDuplicateAssets, bConvertSpaces);
}

void FAssetTypeActions_AnimationAsset::RetargetAssets(TArray<UObject*> InAnimAssets, bool bDuplicateAssets)
{
	bool bRemapReferencedAssets = false;
	USkeleton * NewSkeleton = NULL;
	USkeleton * OldSkeleton = NULL;
	if(InAnimAssets.Num() > 0)
	{
		UAnimationAsset * AnimAsset = CastChecked<UAnimationAsset>(InAnimAssets[0]);
		OldSkeleton = AnimAsset->GetSkeleton();
	}

	const FText Message = LOCTEXT("SelectSkeletonToRemap", "Select the skeleton to remap this asset to.");

	auto AnimAssets = GetTypedWeakObjectPtrs<UObject>(InAnimAssets);
	SAnimationRemapSkeleton::ShowWindow(OldSkeleton, Message, FOnRetargetAnimation::CreateSP(this, &FAssetTypeActions_AnimationAsset::RetargetAnimationHandler, bDuplicateAssets, AnimAssets) );
}


#undef LOCTEXT_NAMESPACE