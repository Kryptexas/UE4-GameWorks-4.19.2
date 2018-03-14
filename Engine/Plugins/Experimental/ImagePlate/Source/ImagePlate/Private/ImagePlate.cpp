// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ImagePlate.h"
#include "ImagePlateComponent.h"

AImagePlate::AImagePlate(const FObjectInitializer& Init)
	: Super(Init)
{
	ImagePlate = Init.CreateDefaultSubobject<UImagePlateComponent>(this, "ImagePlateComponent");
	RootComponent = ImagePlate;
}