// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "FbxTextureImportData.generated.h"

/**
 * Import data and options used when importing any mesh from FBX
 */
UCLASS(AutoExpandCategories=(Texture))
class UFbxTextureImportData : public UFbxAssetImportData
{
	GENERATED_UCLASS_BODY()

	/** If either importing of textures (or materials) is enabled, this option will cause normal map values to be inverted */
	UPROPERTY(EditAnywhere, config, Category=ImportSettings)
	uint32 bInvertNormalMaps:1;
};