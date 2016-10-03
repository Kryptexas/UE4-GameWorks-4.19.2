// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimationEditorPreviewScene.h"

class FPreviewSceneDescriptionCustomization : public IDetailCustomization
{
public:
	FPreviewSceneDescriptionCustomization(const FString& InSkeletonName, const TSharedRef<class IPersonaPreviewScene>& InPreviewScene, const TSharedRef<class IEditableSkeleton>& InEditableSkeleton)
		: SkeletonName(InSkeletonName)
		, PreviewScene(StaticCastSharedRef<FAnimationEditorPreviewScene>(InPreviewScene))
		, EditableSkeleton(InEditableSkeleton)
	{}

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	bool HandleShouldFilterAsset(const FAssetData& InAssetData, bool bCanUseDifferentSkeleton);

	void HandleAnimationModeChanged();

	void HandleAnimationChanged(const FAssetData& InAssetData);

	void HandleMeshChanged(const FAssetData& InAssetData);

	void HandleAdditionalMeshesChanged(const FAssetData& InAssetData, IDetailLayoutBuilder* DetailLayoutBuilder);

private:
	/** Cached skeleton name to check for asset registry tags */
	FString SkeletonName;

	/** Preview scene we will be editing */
	TWeakPtr<class FAnimationEditorPreviewScene> PreviewScene;

	/** Editable Skeleton scene we will be editing */
	TWeakPtr<class IEditableSkeleton> EditableSkeleton;
};

class FPreviewMeshCollectionEntryCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance()
	{
		return MakeShareable(new FPreviewMeshCollectionEntryCustomization(nullptr));
	}

	FPreviewMeshCollectionEntryCustomization(const TSharedPtr<IPersonaPreviewScene>& InPreviewScene)
		: PreviewScene(InPreviewScene)
	{}

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override {}

private:
	bool HandleShouldFilterAsset(const FAssetData& InAssetData, FString SkeletonName);

	void HandleMeshChanged(const FAssetData& InAssetData);

	void HandleMeshesArrayChanged(TSharedPtr<IPropertyUtilities> PropertyUtilities);

private:
	/** Preview scene we will be editing */
	TWeakPtr<class IPersonaPreviewScene> PreviewScene;
};