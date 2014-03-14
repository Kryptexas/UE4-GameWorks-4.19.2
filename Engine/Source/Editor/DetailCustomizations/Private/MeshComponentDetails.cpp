// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "MeshComponentDetails.h"
#include "AssetSelection.h"

TSharedRef<IDetailCustomization> FMeshComponentDetails::MakeInstance()
{
	return MakeShareable( new FMeshComponentDetails );
}

void FMeshComponentDetails::CustomizeDetails( IDetailLayoutBuilder& DetailLayout )
{
	RenderingCategory = &DetailLayout.EditCategory("Rendering");
	TSharedRef<IPropertyHandle> MaterialProperty = DetailLayout.GetProperty( "Materials" );

	if( MaterialProperty->IsValidHandle() )
	{
		// Only show this in the advanced section of the category if we have selected actors (which will show a separate material section)
		bool bIsAdvanced = DetailLayout.GetDetailsView().GetSelectedActorInfo().NumSelected > 0;

		RenderingCategory->AddProperty( MaterialProperty, bIsAdvanced ? EPropertyLocation::Advanced : EPropertyLocation::Default );
	}

}