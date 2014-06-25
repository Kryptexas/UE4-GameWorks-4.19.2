// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"
#include "Foliage/FoliageType_InstancedStaticMesh.h"

UClass* FAssetTypeActions_InstancedFoliageSettings::GetSupportedClass() const
{
	return UFoliageType_InstancedStaticMesh::StaticClass();
}