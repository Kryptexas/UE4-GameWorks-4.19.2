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
		AddBoneNameProperty(DetailBuilder, DetailObjectClass, TEXT("FirstSocketName"), TEXT("Trail"));
		AddBoneNameProperty(DetailBuilder, DetailObjectClass, TEXT("SecondSocketName"), TEXT("Trail"));
	}
}

void FAnimNotifyDetails::AddBoneNameProperty(IDetailLayoutBuilder& DetailBuilder, const UClass* PropertyClass, const TCHAR* PropertyName, const TCHAR* CategoryName)
{
	IDetailCategoryBuilder& AnimNotifyCategory = DetailBuilder.EditCategory(CategoryName, TEXT(""), ECategoryPriority::TypeSpecific);
	int32 PropIndex = BoneNameProperties.Num();
	TSharedPtr<IPropertyHandle> BoneNameProperty = DetailBuilder.GetProperty(FName(PropertyName), PropertyClass);

	if (BoneNameProperty.IsValid() && BoneNameProperty->GetProperty())
	{
		BoneNameProperties.Add(BoneNameProperty);
		// get all the possible suggestions for the bones and sockets.
		const TArray< TWeakObjectPtr<UObject> >& SelectedObjects = DetailBuilder.GetDetailsView().GetSelectedObjects();
		if (SelectedObjects.Num() == 1)
		{
			TWeakObjectPtr<UObject> SelectedObject = SelectedObjects[0];
			const UObject* OuterObject = SelectedObject->GetOuter();
			if (const UAnimSequence* AnimSequence = Cast<const UAnimSequence>(OuterObject))
			{
				if (const USkeleton* Skeleton = AnimSequence->GetSkeleton())
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

void FAnimNotifyDetails::OnSearchBoxCommitted(const FText& InSearchText, ETextCommit::Type CommitInfo, int32 PropertyIndex )
{
	BoneNameProperties[PropertyIndex]->SetValue( InSearchText.ToString() );
}
