// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "AnimationEditorUtils.h"
#include "AssetToolsModule.h"
#include "ContentBrowserModule.h"

#define LOCTEXT_NAMESPACE "AnimationEditorUtils"

namespace AnimationEditorUtils
{
	/** Creates a unique package and asset name taking the form InBasePackageName+InSuffix */
	void CreateUniqueAssetName(const FString& InBasePackageName, const FString& InSuffix, FString& OutPackageName, FString& OutAssetName) 
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
		AssetToolsModule.Get().CreateUniqueAssetName(InBasePackageName, InSuffix, OutPackageName, OutAssetName);
	}

	void CreateAnimationAssets(const TArray<TWeakObjectPtr<USkeleton>>& Skeletons, TSubclassOf<UAnimationAsset> AssetClass, const FString& InPrefix, FAnimAssetCreated AssetCreated )
	{
		TArray<UObject*> ObjectsToSync;
		for(auto SkelIt = Skeletons.CreateConstIterator(); SkelIt; ++SkelIt)
		{
			USkeleton* Skeleton = (*SkelIt).Get();
			if(Skeleton)
			{
				// Determine an appropriate name
				FString Name;
				FString PackageName;
				CreateUniqueAssetName(Skeleton->GetOutermost()->GetName(), InPrefix, PackageName, Name);

				// Create the asset, and assign its skeleton
				FAssetToolsModule& AssetToolsModule = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools");
				UAnimationAsset* NewAsset = Cast<UAnimationAsset>(AssetToolsModule.Get().CreateAsset(Name, FPackageName::GetLongPackagePath(PackageName), AssetClass, NULL));

				if(NewAsset)
				{
					NewAsset->SetSkeleton(Skeleton);
					NewAsset->MarkPackageDirty();

					ObjectsToSync.Add(NewAsset);
				}
			}
		}

		if (AssetCreated.IsBound())
		{
			AssetCreated.Execute(ObjectsToSync);
		}
	}

	template <typename TFactory, typename T>
	void ExecuteNewAnimAsset(TArray<TWeakObjectPtr<USkeleton>> Objects, const FString InSuffix, FAnimAssetCreated AssetCreated, bool bInContentBrowser )
	{
		if(bInContentBrowser && Objects.Num() == 1)
		{
			auto Object = Objects[0].Get();

			if(Object)
			{
				// Determine an appropriate name for inline-rename
				FString Name;
				FString PackageName;
				CreateUniqueAssetName(Object->GetOutermost()->GetName(), InSuffix, PackageName, Name);

				TFactory* Factory = ConstructObject<TFactory>(TFactory::StaticClass());
				Factory->TargetSkeleton = Object;

				FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
				ContentBrowserModule.Get().CreateNewAsset(Name, FPackageName::GetLongPackagePath(PackageName), T::StaticClass(), Factory);

				if(AssetCreated.IsBound())
				{
					// @TODO: this doesn't work
					//FString LongPackagePath = FPackageName::GetLongPackagePath(PackageName);
					UObject * 	Parent = FindPackage(NULL, *PackageName);
					UObject* NewAsset = FindObject<UObject>(Parent, *Name, false);
					if (NewAsset)
					{
						TArray<UObject*> NewAssets;
						NewAssets.Add(NewAsset);
						AssetCreated.Execute(NewAssets);
					}
				}
			}
		}
		else
		{
			CreateAnimationAssets(Objects, T::StaticClass(), InSuffix, AssetCreated);
		}
	}

	/** Handler for the blend space sub menu */
	void FillBlendSpaceMenu(FMenuBuilder& MenuBuilder, TArray<TWeakObjectPtr<USkeleton>> Skeletons, FAnimAssetCreated AssetCreated, bool bInContentBrowser)
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("SkeletalMesh_New1DBlendspace", "Create 1D BlendSpace"),
			LOCTEXT("SkeletalMesh_New1DBlendspaceTooltip", "Creates a 1D blendspace using the skeleton of the selected mesh."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateStatic(&ExecuteNewAnimAsset<UBlendSpaceFactory1D, UBlendSpace1D>, Skeletons, FString("_BlendSpace1D"), AssetCreated, bInContentBrowser),
				FCanExecuteAction()
				)
			);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("SkeletalMesh_New2DBlendspace", "Create 2D BlendSpace"),
			LOCTEXT("SkeletalMesh_New2DBlendspaceTooltip", "Creates a 2D blendspace using the skeleton of the selected mesh."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateStatic(&ExecuteNewAnimAsset<UBlendSpaceFactoryNew, UBlendSpace>, Skeletons, FString("_BlendSpace2D"), AssetCreated, bInContentBrowser),
				FCanExecuteAction()
				)
			);
	}

	/** Handler for the blend space sub menu */
	void FillAimOffsetBlendSpaceMenu(FMenuBuilder& MenuBuilder, TArray<TWeakObjectPtr<USkeleton>> Skeletons, FAnimAssetCreated AssetCreated, bool bInContentBrowser)
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("SkeletalMesh_New1DAimOffset", "Create 1D AimOffset"),
			LOCTEXT("SkeletalMesh_New1DAimOffsetTooltip", "Creates a 1D aimoffset blendspace using the selected skeleton."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateStatic(&ExecuteNewAnimAsset<UAimOffsetBlendSpaceFactory1D, UAimOffsetBlendSpace1D>, Skeletons, FString("_AimOffset1D"), AssetCreated, bInContentBrowser),
				FCanExecuteAction()
				)
			);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("SkeletalMesh_New2DAimOffset", "Create 2D AimOffset"),
			LOCTEXT("SkeletalMesh_New2DAimOffsetTooltip", "Creates a 2D aimoffset blendspace using the selected skeleton."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateStatic(&ExecuteNewAnimAsset<UAimOffsetBlendSpaceFactoryNew, UAimOffsetBlendSpace>, Skeletons, FString("_AimOffset2D"), AssetCreated, bInContentBrowser),
				FCanExecuteAction()
				)
			);
	}

	void FillCreateAssetMenu(FMenuBuilder& MenuBuilder, TArray<TWeakObjectPtr<USkeleton>> Skeletons, FAnimAssetCreated AssetCreated, bool bInContentBrowser) 
	{
		MenuBuilder.AddSubMenu(
			LOCTEXT("SkeletalMesh_NewAimOffset", "Create AimOffset"),
			LOCTEXT("SkeletalMesh_NewAimOffsetTooltip", "Creates an aimoffset blendspace using the selected skeleton."),
			FNewMenuDelegate::CreateStatic(&FillAimOffsetBlendSpaceMenu, Skeletons, AssetCreated, bInContentBrowser));

		MenuBuilder.AddSubMenu(
			LOCTEXT("SkeletalMesh_NewBlendspace", "Create BlendSpace"),
			LOCTEXT("SkeletalMesh_NewBlendspaceTooltip", "Creates a blendspace using the skeleton of the selected mesh."),
			FNewMenuDelegate::CreateStatic(&FillBlendSpaceMenu, Skeletons, AssetCreated, bInContentBrowser));

		MenuBuilder.AddMenuEntry(
			LOCTEXT("Skeleton_NewAnimComposite", "Create AnimComposite"),
			LOCTEXT("Skeleton_NewAnimCompositeTooltip", "Creates an AnimComposite using the selected skeleton."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateStatic(&ExecuteNewAnimAsset<UAnimCompositeFactory, UAnimComposite>, Skeletons, FString("_Composite"), AssetCreated, bInContentBrowser),
				FCanExecuteAction()
				)
			);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("Skeleton_NewAnimMontage", "Create AnimMontage"),
			LOCTEXT("Skeleton_NewAnimMontageTooltip", "Creates an AnimMontage using the selected skeleton."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateStatic(&ExecuteNewAnimAsset<UAnimMontageFactory, UAnimMontage>, Skeletons, FString("_Montage"), AssetCreated, bInContentBrowser),
				FCanExecuteAction()
				)
			);
	}
}

#undef LOCTEXT_NAMESPACE