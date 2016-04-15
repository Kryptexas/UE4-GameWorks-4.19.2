// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.


#include "UMGPrivatePCH.h"
#include "WidgetMaterialTrackUtilities.h"


FSlateBrush* WidgetMaterialTrackUtilities::GetBrush( UWidget* Widget, FName BrushPropertyName )
{
	for ( TFieldIterator<UProperty> PropertyIterator( Widget->GetClass() ); PropertyIterator; ++PropertyIterator )
	{
		UProperty* Property = *PropertyIterator;
		if ( Property->GetFName() == BrushPropertyName )
		{
			return Property->ContainerPtrToValuePtr<FSlateBrush>( Widget );
		}
	}
	return nullptr;
}


void WidgetMaterialTrackUtilities::GetMaterialBrushProperties( UWidget* Widget, TArray<UProperty*>& MaterialBrushProperties )
{
	for ( TFieldIterator<UProperty> PropertyIterator( Widget->GetClass() ); PropertyIterator; ++PropertyIterator )
	{
		UProperty* Property = *PropertyIterator;
		if ( Property && !Property->HasAnyPropertyFlags( CPF_Deprecated ) )
		{
			UStructProperty* StructProperty = Cast<UStructProperty>( Property );
			if ( StructProperty != nullptr && StructProperty->Struct->GetName() == "SlateBrush" )
			{
				FSlateBrush* SlateBrush = Property->ContainerPtrToValuePtr<FSlateBrush>( Widget );
				UMaterialInterface* MaterialInterface = Cast<UMaterialInterface>( SlateBrush->GetResourceObject() );
				if ( MaterialInterface != nullptr )
				{
					MaterialBrushProperties.Add( Property );
				}
			}
		}
	}
}