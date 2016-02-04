// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "StaticMeshComponentDetails.h"

#define LOCTEXT_NAMESPACE "StaticMeshComponentDetails"


TSharedRef<IDetailCustomization> FStaticMeshComponentDetails::MakeInstance()
{
	return MakeShareable( new FStaticMeshComponentDetails );
}

void FStaticMeshComponentDetails::CustomizeDetails( IDetailLayoutBuilder& DetailBuilder )
{
	// Create a category so this is displayed early in the properties
	DetailBuilder.EditCategory( "StaticMesh", FText::GetEmpty(), ECategoryPriority::Important);

	DetailBuilder.GetObjectsBeingCustomized(ObjectsCustomized);
	
	TSharedPtr<IPropertyHandle> UseDefaultCollisionHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UStaticMeshComponent, bUseDefaultCollision));

	DetailBuilder.EditCategory(TEXT("Collision")).AddCustomRow(UseDefaultCollisionHandle->GetPropertyDisplayName())
	.IsEnabled(TAttribute<bool>(this, &FStaticMeshComponentDetails::IsDefaultCollisionSupported))
	.NameContent()
	[
		UseDefaultCollisionHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	[
		UseDefaultCollisionHandle->CreatePropertyValueWidget()
	];

	UseDefaultCollisionHandle->MarkHiddenByCustomization();
}

bool FStaticMeshComponentDetails::IsDefaultCollisionSupported() const
{
	// Whether to show the default collision property
	bool bShowCollisionDefault = false;
	for (TWeakObjectPtr<UObject> Obj : ObjectsCustomized)
	{
		if (UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Obj.Get()))
		{
			if (SMC->SupportsDefaultCollision())
			{
				bShowCollisionDefault = true;
			}
			else
			{
				bShowCollisionDefault = false;
				break;
			}
		}
	}

	return bShowCollisionDefault;
}

#undef LOCTEXT_NAMESPACE
