// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "AssetTypeActions_FlexFluidSurface.h"

#include "FlexFluidSurface.h"

UClass* FAssetTypeActions_FlexFluidSurface::GetSupportedClass() const
{
	return UFlexFluidSurface::StaticClass();
}