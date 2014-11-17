// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PersonaPrivatePCH.h"
#include "AnimNotifyDetails.h"
#include "SAssetSearchBox.h"
#include "PropertyHandle.h"
#include "IDetailPropertyRow.h"
#include "DetailWidgetRow.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "IDetailsView.h"
#include "AssetSearchBoxUtilPersona.h"

TSharedRef<IDetailCustomization> FAnimNotifyDetails::MakeInstance()
{
	return MakeShareable( new FAnimNotifyDetails );
}

void FAnimNotifyDetails::CustomizeDetails( IDetailLayoutBuilder& DetailBuilder )
{
	const UClass* DetailObjectClass = DetailBuilder.GetDetailsView().GetBaseClass();
	if (DetailObjectClass)
	{
		if (DetailObjectClass->GetName().Find(TEXT("AnimNotify_PlayParticleEffect")) != INDEX_NONE)
		{
			AddBoneNameProperty(DetailBuilder, DetailObjectClass, TEXT("SocketName"), TEXT("AnimNotify"));
		}
		else if (DetailObjectClass->GetName().Find(TEXT("AnimNotify_PlaySound")) != INDEX_NONE)
		{
			AddBoneNameProperty(DetailBuilder, DetailObjectClass, TEXT("AttachName"), TEXT("AnimNotify"));
		}
		else if (DetailObjectClass->GetName().Find(TEXT("AnimNotifyState_Trail")) != INDEX_NONE)
		{
			AddPropertyDefault(DetailBuilder, DetailObjectClass, TEXT("PSTemplate"), TEXT("Trail"));	
			AddBoneNameProperty(DetailBuilder, DetailObjectClass, TEXT("FirstSocketName"), TEXT("Trail"));
			AddBoneNameProperty(DetailBuilder, DetailObjectClass, TEXT("SecondSocketName"), TEXT("Trail"));
			AddPropertyDefault(DetailBuilder, DetailObjectClass, TEXT("WidthScaleMode"), TEXT("Trail"));
			AddCurveNameProperty(DetailBuilder, DetailObjectClass, TEXT("WidthScaleCurve"), TEXT("Trail"));
		}
		else if (DetailObjectClass->GetName().Find(TEXT("AnimNotifyState_TimedParticleEffect")) != INDEX_NONE)
		{
			AddBoneNameProperty(DetailBuilder, DetailObjectClass, TEXT("SocketName"), TEXT("ParticleSystem"));
		}
	}
}

void FAnimNotifyDetails::AddPropertyDefault(IDetailLayoutBuilder& DetailBuilder, const UClass* PropertyClass, const TCHAR* PropertyName, const TCHAR* CategoryName)
{
	IDetailCategoryBuilder& AnimNotifyCategory = DetailBuilder.EditCategory(CategoryName, TEXT(""), ECategoryPriority::TypeSpecific);
	TSharedPtr<IPropertyHandle> Property = DetailBuilder.GetProperty(FName(PropertyName), PropertyClass);
	if (Property.IsValid())
	{
		AnimNotifyCategory.AddProperty(Property);
	}
}

void FAnimNotifyDetails::AddBoneNameProperty(IDetailLayoutBuilder& DetailBuilder, const UClass* PropertyClass, const TCHAR* PropertyName, const TCHAR* CategoryName)
{
	IDetailCategoryBuilder& AnimNotifyCategory = DetailBuilder.EditCategory(CategoryName, TEXT(""), ECategoryPriority::TypeSpecific);
	int32 PropIndex = NameProperties.Num();
	TSharedPtr<IPropertyHandle> BoneNameProperty = DetailBuilder.GetProperty(FName(PropertyName), PropertyClass);

	if (BoneNameProperty.IsValid() && BoneNameProperty->GetProperty())
	{
		NameProperties.Add(BoneNameProperty);
		// get all the possible suggestions for the bones and sockets.
		const TArray< TWeakObjectPtr<UObject> >& SelectedObjects = DetailBuilder.GetDetailsView().GetSelectedObjects();
		if (SelectedObjects.Num() == 1)
		{
			TWeakObjectPtr<UObject> SelectedObject = SelectedObjects[0];
			const UObject* OuterObject = SelectedObject->GetOuter();
			if (const UAnimationAsset* AnimAsset = Cast<const UAnimationAsset>(OuterObject))
			{
				if (const USkeleton* Skeleton = AnimAsset->GetSkeleton())
				{
					AnimNotifyCategory.AddProperty(BoneNameProperty.ToSharedRef())
						.CustomWidget()
						.NameContent()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.Padding(FMargin(2, 1, 0, 1))
							[
								SNew(STextBlock)
								.Text(BoneNameProperty->GetPropertyDisplayName())
								.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
							]
						]
					.ValueContent()
						[
							SNew(SAssetSearchBoxForBones, Skeleton, BoneNameProperty)
							.IncludeSocketsForSuggestions(true)
							.MustMatchPossibleSuggestions(true)
							.HintText(NSLOCTEXT("AnimNotifyDetails", "Hint Text", "Bone Name..."))
							.OnTextCommitted(this, &FAnimNotifyDetails::OnSearchBoxCommitted, PropIndex)
						];
				}
			}
		}
	}
}

void FAnimNotifyDetails::AddCurveNameProperty(IDetailLayoutBuilder& DetailBuilder, const UClass* PropertyClass, const TCHAR* PropertyName, const TCHAR* CategoryName)
{
	IDetailCategoryBuilder& AnimNotifyCategory = DetailBuilder.EditCategory(CategoryName, TEXT(""), ECategoryPriority::TypeSpecific);
	int32 PropIndex = NameProperties.Num();
	TSharedPtr<IPropertyHandle> CurveNameProperty = DetailBuilder.GetProperty(FName(PropertyName), PropertyClass);

	if (CurveNameProperty.IsValid() && CurveNameProperty->GetProperty())
	{
		NameProperties.Add(CurveNameProperty);
		// get all the possible suggestions for the curves.
		const TArray< TWeakObjectPtr<UObject> >& SelectedObjects = DetailBuilder.GetDetailsView().GetSelectedObjects();
		if (SelectedObjects.Num() == 1)
		{
			TWeakObjectPtr<UObject> SelectedObject = SelectedObjects[0];
			const UObject* OuterObject = SelectedObject->GetOuter();
			if (const UAnimationAsset* AnimAsset = Cast<const UAnimationAsset>(OuterObject))
			{
				if (const USkeleton* Skeleton = AnimAsset->GetSkeleton())
				{
					AnimNotifyCategory.AddProperty(CurveNameProperty.ToSharedRef())
						.CustomWidget()
						.NameContent()
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot()
							.Padding(FMargin(2, 1, 0, 1))
							[
								SNew(STextBlock)
								.Text(CurveNameProperty->GetPropertyDisplayName())
								.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
							]
						]
					.ValueContent()
						[
							SNew(SAssetSearchBoxForCurves, Skeleton, CurveNameProperty)
							.IncludeSocketsForSuggestions(true)
							.MustMatchPossibleSuggestions(true)
							.HintText(NSLOCTEXT("AnimNotifyDetails", "Curve Name Hint Text", "Curve Name..."))
							.OnTextCommitted(this, &FAnimNotifyDetails::OnSearchBoxCommitted, PropIndex)
						];
				}
			}
		}
	}
}

void FAnimNotifyDetails::OnSearchBoxCommitted(const FText& InSearchText, ETextCommit::Type CommitInfo, int32 PropertyIndex )
{
	NameProperties[PropertyIndex]->SetValue( InSearchText.ToString() );
}
