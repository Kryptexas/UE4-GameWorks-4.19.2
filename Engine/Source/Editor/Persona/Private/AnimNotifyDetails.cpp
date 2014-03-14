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
	IDetailCategoryBuilder& AnimNotifyCategory = DetailBuilder.EditCategory( TEXT("AnimNotify"), TEXT(""), ECategoryPriority::TypeSpecific );

	// Get the correct property from the various notifies.
	BoneNameProperty = GetBoneNameProperty(DetailBuilder);

	if( BoneNameProperty.IsValid() && BoneNameProperty->GetProperty() )
	{
		// get all the possible suggestions for the bones and sockets.
		const TArray< TWeakObjectPtr<UObject> >& SelectedObjects = DetailBuilder.GetDetailsView().GetSelectedObjects();
		if( SelectedObjects.Num() == 1 )
		{
			TWeakObjectPtr<UObject> SelectedObject = SelectedObjects[0];
			const UObject* OuterObject = SelectedObject->GetOuter();
			if( const UAnimSequence* AnimSequence = Cast<const UAnimSequence>(OuterObject) )
			{
				if( const USkeleton* Skeleton = AnimSequence->GetSkeleton() )
				{
					AnimNotifyCategory.AddProperty( BoneNameProperty.ToSharedRef() )
					.CustomWidget()
					.NameContent()
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.Padding( FMargin( 2, 1, 0, 1 ) )
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
						.HintText(NSLOCTEXT( "AnimNotifyDetails", "Hint Text", "Bone Name..." ) )
						.OnTextCommitted( this, &FAnimNotifyDetails::OnSearchBoxCommitted )
					];
				}
			}
		}
	}
}

TSharedPtr<IPropertyHandle> FAnimNotifyDetails::GetBoneNameProperty( IDetailLayoutBuilder& DetailBuilder )
{
	const UClass* DetailObjectClass = DetailBuilder.GetDetailsView().GetBaseClass();

	if( DetailObjectClass->GetName().Find( TEXT("AnimNotify_PlayParticleEffect") ) != INDEX_NONE )
	{
		return DetailBuilder.GetProperty( FName(TEXT("SocketName")), DetailObjectClass );
	}
	else if( DetailObjectClass->GetName().Find( TEXT("AnimNotify_PlaySound") ) != INDEX_NONE )
	{
		return DetailBuilder.GetProperty( FName(TEXT("AttachName")), DetailObjectClass );
	}

	return NULL;
}

void FAnimNotifyDetails::OnSearchBoxCommitted(const FText& InSearchText, ETextCommit::Type CommitInfo)
{
	BoneNameProperty->SetValue( InSearchText.ToString() );
}
