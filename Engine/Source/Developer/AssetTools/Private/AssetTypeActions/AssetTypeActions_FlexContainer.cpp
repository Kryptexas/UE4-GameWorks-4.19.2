// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"
#include "PhysicsEngine/FlexContainer.h"

UClass* FAssetTypeActions_FlexContainer::GetSupportedClass() const
{
	return UFlexContainer::StaticClass();
}