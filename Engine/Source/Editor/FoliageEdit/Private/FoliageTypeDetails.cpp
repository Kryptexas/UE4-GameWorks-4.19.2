// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "FoliageEditModule.h"
#include "FoliageTypeDetails.h"

TSharedRef<IDetailCustomization> FFoliageTypeDetails::MakeInstance()
{
	return MakeShareable(new FFoliageTypeDetails());

}


void FFoliageTypeDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	
}