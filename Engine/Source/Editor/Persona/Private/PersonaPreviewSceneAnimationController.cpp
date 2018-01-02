// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PersonaPreviewSceneAnimationController.h"
#include "AnimationEditorPreviewScene.h"
#include "PersonaPreviewSceneDescription.h"
#include "IPersonaToolkit.h"
#include "IEditableSkeleton.h"
#include "DetailWidgetRow.h"
#include "PropertyCustomizationHelpers.h"

void UPersonaPreviewSceneAnimationController::InitializeView(UPersonaPreviewSceneDescription* SceneDescription, IPersonaPreviewScene* PreviewScene) const
{
	PreviewScene->SetPreviewAnimationAsset(Animation.LoadSynchronous());
}

void UPersonaPreviewSceneAnimationController::UninitializeView(UPersonaPreviewSceneDescription* SceneDescription, IPersonaPreviewScene* PreviewScene) const
{

}

IDetailPropertyRow* UPersonaPreviewSceneAnimationController::AddPreviewControllerPropertyToDetails(const TSharedRef<IPersonaToolkit>& PersonaToolkit, IDetailLayoutBuilder& DetailBuilder, IDetailCategoryBuilder& Category, const UProperty* Property, const EPropertyLocation::Type PropertyLocation)
{
	TArray<UObject*> ListOfPreviewController{ this };

	FString SkeletonName = FAssetData(&PersonaToolkit->GetEditableSkeleton()->GetSkeleton()).GetExportTextName();

	IDetailPropertyRow* NewRow = Category.AddExternalObjectProperty(ListOfPreviewController, Property->GetFName(), PropertyLocation);
	
	NewRow->CustomWidget()
	.NameContent()
	[
		NewRow->GetPropertyHandle()->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MaxDesiredWidth(250.0f)
	.MinDesiredWidth(250.0f)
	[
		SNew(SObjectPropertyEntryBox)
		.AllowedClass(UAnimationAsset::StaticClass())
		.PropertyHandle(NewRow->GetPropertyHandle())
		.OnShouldFilterAsset(FOnShouldFilterAsset::CreateUObject(this, &UPersonaPreviewSceneAnimationController::HandleShouldFilterAsset, SkeletonName))
		.ThumbnailPool(DetailBuilder.GetThumbnailPool())
	];
	return NewRow;
}

bool UPersonaPreviewSceneAnimationController::HandleShouldFilterAsset(const FAssetData& InAssetData, const FString SkeletonName) const
{
	FString SkeletonTag = InAssetData.GetTagValueRef<FString>("Skeleton");
	if (SkeletonTag == SkeletonName)
	{
		return false;
	}

	return true;
}