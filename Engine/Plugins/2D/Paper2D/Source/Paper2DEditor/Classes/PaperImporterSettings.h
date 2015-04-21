// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperImporterSettings.generated.h"

/**
 * Implements the settings for imported Paper2D assets, such as sprite sheet textures.
 */
UCLASS(config=Editor, defaultconfig)
class PAPER2DEDITOR_API UPaperImporterSettings : public UObject
{
	GENERATED_BODY()

public:
	// A list of default suffixes to use when looking for associated normal maps while importing sprites or creating sprites from textures
	UPROPERTY(config, EditAnywhere, Category=ImportSettings)
	TArray<FString> NormalMapTextureSuffixes;

	// The default suffix to remove (if present) from a texture name before looking for an associated normal map using NormalMapTextureSuffixes
	UPROPERTY(config, EditAnywhere, Category=ImportSettings)
	TArray<FString> BaseMapTextureSuffixes;

	// The default masked material for newly created sprites (masked means binary opacity: things are either opaque or see-thru, with nothing in between)
	UPROPERTY(config, EditAnywhere, Category=Settings, meta=(AllowedClasses="MaterialInterface", DisplayName="Lit Default Masked Material"))
	FStringAssetReference LitDefaultMaskedMaterialName;

	// The default translucent material for newly created sprites (translucent means smooth opacity which can vary continuously from 0..1, but translucent rendering is more expensive that opaque or masked rendering and has different sorting rules)
	UPROPERTY(config, EditAnywhere, Category=Settings, meta=(AllowedClasses="MaterialInterface", DisplayName="Lit Default Translucent Material"))
	FStringAssetReference LitDefaultTranslucentMaterialName;

	// The default opaque material for newly created sprites
	UPROPERTY(config, EditAnywhere, Category=Settings, meta=(AllowedClasses="MaterialInterface", DisplayName="Lit Default Opaque Material"))
	FStringAssetReference LitDefaultOpaqueMaterialName;

public:
	UPaperImporterSettings();

	// Removes the suffix from the specified name if it matches something in BaseMapTextureSuffixes
	FString RemoveSuffixFromBaseMapName(const FString& InName) const;

	// Generates names to test for a normal map using NormalMapTextureSuffixes
	void GenerateNormalMapNamesToTest(const FString& InRoot, TArray<FString>& InOutNames) const;

	// Pushes the lit materials
	void PopulateMaterialsIntoInitParams(FSpriteAssetInitParameters& InitParams) const;
};
