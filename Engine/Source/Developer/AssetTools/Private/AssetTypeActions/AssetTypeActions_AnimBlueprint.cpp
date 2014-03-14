// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"
#include "PersonaModule.h"
#include "EditorAnimUtils.h"
#include "SSkeletonWidget.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"

void FAssetTypeActions_AnimBlueprint::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	FAssetTypeActions_Blueprint::GetActions(InObjects, MenuBuilder);

	auto AnimBlueprints = GetTypedWeakObjectPtrs<UAnimBlueprint>(InObjects);

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AnimBlueprint_FindSkeleton", "Find Skeleton"),
		LOCTEXT("AnimBlueprint_FindSkeletonTooltip", "Finds the skeleton used by the selected Anim Blueprints in the content browser."),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_AnimBlueprint::ExecuteFindSkeleton, AnimBlueprints ),
			FCanExecuteAction()
			)
		);

	MenuBuilder.AddSubMenu( 
		LOCTEXT("RetargetBlueprintSubmenu", "Retarget Anim Blueprints"),
		LOCTEXT("RetargetBlueprintSubmenu_ToolTip", "Opens the retarget blueprints menu"),
		FNewMenuDelegate::CreateSP( this, &FAssetTypeActions_AnimBlueprint::FillRetargetMenu, InObjects ) );

}

void FAssetTypeActions_AnimBlueprint::FillRetargetMenu( FMenuBuilder& MenuBuilder, const TArray<UObject*> InObjects )
{
	bool bAllSkeletonsNull = true;

	for(auto Iter = InObjects.CreateConstIterator(); Iter; ++Iter)
	{
		if(UAnimBlueprint* AnimBlueprint = Cast<UAnimBlueprint>(*Iter))
		{
			if(AnimBlueprint->TargetSkeleton)
			{
				bAllSkeletonsNull = false;
				break;
			}
		}
	}

	if(bAllSkeletonsNull)
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("AnimBlueprint_RetargetSkeletonInPlace", "Retarget skeleton on existing Anim Blueprints"),
			LOCTEXT("AnimBlueprint_RetargetSkeletonInPlaceTooltip", "Retargets the selected Anim Blueprints to a new skeleton (and optionally all referenced animations too)"),
			FSlateIcon(),
			FUIAction(
			FExecuteAction::CreateSP( this, &FAssetTypeActions_AnimBlueprint::RetargetAssets, InObjects, false ), // false = do not duplicate assets first
			FCanExecuteAction()
			)
			);
	}

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AnimBlueprint_DuplicateAndRetargetSkeleton", "Duplicate Anim Blueprints and Retarget"),
		LOCTEXT("AnimBlueprint_DuplicateAndRetargetSkeletonTooltip", "Duplicates and then retargets the selected Anim Blueprints to a new skeleton (and optionally all referenced animations too)"),
		FSlateIcon(),
		FUIAction(
		FExecuteAction::CreateSP( this, &FAssetTypeActions_AnimBlueprint::RetargetAssets, InObjects, true ), // true = duplicate assets and retarget them
		FCanExecuteAction()
		)
		);
}

void FAssetTypeActions_AnimBlueprint::OpenAssetEditor( const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor )
{
	EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (auto ObjIt = InObjects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto AnimBlueprint = Cast<UAnimBlueprint>(*ObjIt);
		if (AnimBlueprint != NULL && AnimBlueprint->SkeletonGeneratedClass && AnimBlueprint->GeneratedClass)
		{
			if(!AnimBlueprint->TargetSkeleton)
			{
				FText ShouldRetargetMessage = LOCTEXT("ShouldRetarget_Message", "Could not find the skeleton for Anim Blueprint '{BlueprintName}' Would you like to choose a new one?");
				
				FFormatNamedArguments Arguments;
				Arguments.Add( TEXT("BlueprintName"), FText::FromString(AnimBlueprint->GetName()));

				if ( FMessageDialog::Open(EAppMsgType::YesNo, FText::Format(ShouldRetargetMessage, Arguments)) == EAppReturnType::Yes )
				{
					bool bDuplicateAssets = false;
					TArray<UObject*> AnimBlueprints;
					AnimBlueprints.Add(AnimBlueprint);
					RetargetAssets(AnimBlueprints, bDuplicateAssets);
				}
			}

			if(AnimBlueprint->TargetSkeleton)
			{
				FPersonaModule& PersonaModule = FModuleManager::LoadModuleChecked<FPersonaModule>( "Persona" );
				PersonaModule.CreatePersona( Mode, EditWithinLevelEditor, NULL, AnimBlueprint, NULL, NULL);
			}
			else
			{
				FMessageDialog::Open( EAppMsgType::Ok, LOCTEXT("FailedToLoadSkeletonlessAnimBlueprint", "The Anim Blueprint could not be loaded because it's skeleton is missing."));
			}
		}
		else
		{
			FMessageDialog::Open( EAppMsgType::Ok, LOCTEXT("FailedToLoadCorruptAnimBlueprint", "The Anim Blueprint could not be loaded because it is corrupt."));
		}
	}
}

bool FAssetTypeActions_AnimBlueprint::CanCreateNewDerivedBlueprint() const
{
	// Animation blueprints cannot create derived blueprints.
	return false;
}

void FAssetTypeActions_AnimBlueprint::ExecuteFindSkeleton(TArray<TWeakObjectPtr<UAnimBlueprint>> Objects)
{
	TArray<UObject*> ObjectsToSync;
	for (auto ObjIt = Objects.CreateConstIterator(); ObjIt; ++ObjIt)
	{
		auto Object = (*ObjIt).Get();
		if ( Object )
		{
			USkeleton* Skeleton = Object->TargetSkeleton;
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

void FAssetTypeActions_AnimBlueprint::RetargetAssets(const TArray<UObject*> InAnimBlueprints, bool bDuplicateAssets)
{
	bool bRemapReferencedAssets = false;
	USkeleton * NewSkeleton = NULL;

	const FText Message = LOCTEXT("RemapSkeleton_Warning", "Select the skeleton to remap this asset to.");

	if (SAnimationRemapSkeleton::ShowModal(NULL, NewSkeleton, Message, &bRemapReferencedAssets))
	{
		EditorAnimUtils::RetargetAnimations(NewSkeleton, InAnimBlueprints, bRemapReferencedAssets, bDuplicateAssets);
	}
}

#undef LOCTEXT_NAMESPACE