// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"
#include "ContentBrowserModule.h"
#include "Animation/AnimMontage.h"
#include "AnimationEditorUtils.h"

#define LOCTEXT_NAMESPACE "AssetTypeActions"


void FAssetTypeActions_AnimMontage::GetActions( const TArray<UObject*>& InObjects, FMenuBuilder& MenuBuilder )
{
	auto Montages = GetTypedWeakObjectPtrs<UAnimMontage>(InObjects);

	// only show child montage if inobjects are not child montage already
	bool bContainsChildMontage = false;
	for (UObject* Object : InObjects)
	{
		bContainsChildMontage |= CastChecked<UAnimationAsset>(Object)->HasParentAsset();
		if (bContainsChildMontage)
		{
			break;
		}
	}

	// if no child montage is found
	if (!bContainsChildMontage)
	{
		// create mew child anim montage
		MenuBuilder.AddMenuEntry(
			LOCTEXT("AnimMontage_CreateChildMontage", "Create Child Montage"),
			LOCTEXT("AnimMontage_CreateChildMontageTooltip", "Create Child Animation Montage and remap to another animation assets."),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "ClassIcon.AnimMontage"),
			FUIAction(FExecuteAction::CreateSP(this, &FAssetTypeActions_AnimMontage::CreateChildAnimMontage, Montages))
		);
	}

	FAssetTypeActions_AnimationAsset::GetActions(InObjects, MenuBuilder);
}

void FAssetTypeActions_AnimMontage::CreateChildAnimMontage(TArray<TWeakObjectPtr<UAnimMontage>> AnimMontages) 
{
	if (AnimMontages.Num() > 0)
	{
		const FString DefaultSuffix = TEXT("_Montage");
		UAnimMontageFactory* Factory = NewObject<UAnimMontageFactory>();

		TArray<UObject*> ObjectsToSync;
		// need to know source and target
		for (int32 MontageIndex = 0; MontageIndex < AnimMontages.Num(); ++MontageIndex)
		{
			UAnimMontage* ParentMontage = AnimMontages[MontageIndex].Get();
			check(ParentMontage);

			UAnimMontage* NewAsset = AnimationEditorUtils::CreateAnimationAsset<UAnimMontage>(ParentMontage->GetSkeleton(), ParentMontage->GetOutermost()->GetName(), TEXT("_Child"));
			if (NewAsset)
			{
				NewAsset->SetParentAsset(ParentMontage);
				ObjectsToSync.Add(NewAsset);
			}
		}

		FAssetTools::Get().SyncBrowserToAssets(ObjectsToSync);
	}
}

#undef LOCTEXT_NAMESPACE
