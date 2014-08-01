// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MediaAssetEditorPrivatePCH.h"


/* UMediaAssetFactoryNew structors
 *****************************************************************************/

UMediaAssetFactoryNew::UMediaAssetFactoryNew( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{
	SupportedClass = UMediaAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}


/* UFactory overrides
 *****************************************************************************/

UObject* UMediaAssetFactoryNew::FactoryCreateNew( UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn )
{
	return CastChecked<UMediaAsset>(StaticConstructObject(InClass, InParent, InName, Flags));
}


bool UMediaAssetFactoryNew::ShouldShowInNewMenu( ) const
{
	return true;
}
