// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

UClass* FAssetTypeActions_PhysicalMaterial::GetSupportedClass() const
{
	return UPhysicalMaterial::StaticClass();
}