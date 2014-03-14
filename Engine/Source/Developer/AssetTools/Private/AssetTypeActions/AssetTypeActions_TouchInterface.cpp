// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"
#include "EngineUserInterfaceClasses.h"

UClass* FAssetTypeActions_TouchInterface::GetSupportedClass() const
{
	return UTouchInterface::StaticClass();
}
