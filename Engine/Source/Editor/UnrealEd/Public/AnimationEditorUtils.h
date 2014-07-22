// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#ifndef __AnimationEditorUtils_h__
#define __AnimationEditorUtils_h__

/** Defines FCanExecuteAction delegate interface.  Returns true when an action is able to execute. */
DECLARE_DELEGATE_OneParam(FAnimAssetCreated, TArray<class UObject*>);

//Animation editor utility functions
namespace AnimationEditorUtils
{
	UNREALED_API void CreateAnimationAssets(const TArray<TWeakObjectPtr<USkeleton>>& Skeletons, TSubclassOf<UAnimationAsset> AssetClass, const FString& InPrefix, FAnimAssetCreated AssetCreated );
	UNREALED_API void FillCreateAssetMenu(FMenuBuilder& MenuBuilder, TArray<TWeakObjectPtr<USkeleton>> Skeletons, FAnimAssetCreated AssetCreated, bool bInContentBrowser=true);
} // namespace AnimationEditorUtils

#endif //__AnimationEditorUtils_h__
