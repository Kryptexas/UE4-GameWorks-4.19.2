// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"

UClass* FAssetTypeActions_InstancedFoliageSettings::GetSupportedClass() const
{
	// UInstancedFoliageSettings is defined in EngineFoliageClasses.h
	return UInstancedFoliageSettings::StaticClass();
}