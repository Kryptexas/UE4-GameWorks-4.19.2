// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

#include "Factories/SlateBrushAssetFactory.h"

#define LOCTEXT_NAMESPACE "SlateBrushAssetFactory"

USlateBrushAssetFactory::USlateBrushAssetFactory( const class FPostConstructInitializeProperties& PCIP )
 : Super(PCIP)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = USlateBrushAsset::StaticClass();
}


FText USlateBrushAssetFactory::GetDisplayName() const
{
	return LOCTEXT("SlateBrushAssetFactoryDescription", "Slate Brush");
}


bool USlateBrushAssetFactory::ConfigureProperties()
{
	return true;
}


UObject* USlateBrushAssetFactory::FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn)
{
	USlateBrushAsset* NewSlateBrushAsset = CastChecked<USlateBrushAsset>(StaticConstructObject(USlateBrushAsset::StaticClass(), InParent, Name, Flags));
	NewSlateBrushAsset->Brush = InitialTexture != NULL ? FSlateDynamicImageBrush( InitialTexture, FVector2D( InitialTexture->GetImportedSize() ), NAME_None ) : FSlateBrush();
	return NewSlateBrushAsset;
}

#undef LOCTEXT_NAMESPACE