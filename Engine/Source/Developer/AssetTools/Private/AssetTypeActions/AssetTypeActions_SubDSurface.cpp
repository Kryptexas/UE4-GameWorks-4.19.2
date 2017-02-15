// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AssetTypeActions/AssetTypeActions_SubDSurface.h"
#include "Engine/SubDSurface.h"

UClass* FAssetTypeActions_SubDSurface::GetSupportedClass() const
{
	return USubDSurface::StaticClass();
}
