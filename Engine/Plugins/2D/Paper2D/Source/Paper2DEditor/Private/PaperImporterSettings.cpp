// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "PaperImporterSettings.h"

//////////////////////////////////////////////////////////////////////////
// UPaperImporterSettings

UPaperImporterSettings::UPaperImporterSettings()
: DefaultSpriteTextureGroup(TEXTUREGROUP_Pixels2D)
	, bOverrideTextureCompression(true)
	, DefaultSpriteTextureCompression(TC_EditorIcon)
{
	NormalMapTextureSuffixes.Add(TEXT("_N"));
	NormalMapTextureSuffixes.Add(TEXT("_Normal"));

	BaseMapTextureSuffixes.Add(TEXT("_D"));
	BaseMapTextureSuffixes.Add(TEXT("_Diffuse"));

	if (!IsRunningCommandlet())
	{
		LitDefaultMaskedMaterialName = FStringAssetReference("/Paper2D/MaskedLitSpriteMaterial.MaskedLitSpriteMaterial");
		LitDefaultTranslucentMaterialName = FStringAssetReference("/Paper2D/TranslucentLitSpriteMaterial.TranslucentLitSpriteMaterial");
		LitDefaultOpaqueMaterialName = FStringAssetReference("/Paper2D/OpaqueLitSpriteMaterial.OpaqueLitSpriteMaterial");
	}
}

FString UPaperImporterSettings::RemoveSuffixFromBaseMapName(const FString& InName) const
{
	FString Result = InName;

	for (const FString& PossibleSuffix : BaseMapTextureSuffixes)
	{
		if (Result.RemoveFromEnd(PossibleSuffix))
		{
			return Result;
		}
	}

	return Result;
}

void UPaperImporterSettings::GenerateNormalMapNamesToTest(const FString& InRoot, TArray<FString>& InOutNames) const
{
	for (const FString& PossibleSuffix : NormalMapTextureSuffixes)
	{
		InOutNames.Add(InRoot + PossibleSuffix);
	}
}

void UPaperImporterSettings::PopulateMaterialsIntoInitParams(FSpriteAssetInitParameters& InitParams) const
{
	InitParams.MaskedMaterialOverride = Cast<UMaterialInterface>(LitDefaultMaskedMaterialName.TryLoad());
	InitParams.OpaqueMaterialOverride = Cast<UMaterialInterface>(LitDefaultOpaqueMaterialName.TryLoad());
	InitParams.TranslucentMaterialOverride = Cast<UMaterialInterface>(LitDefaultTranslucentMaterialName.TryLoad());
}

void UPaperImporterSettings::ApplyTextureSettings(UTexture2D* Texture) const
{
	if (Texture->IsNormalMap())
	{
		// Leave normal maps alone
	}
	else
	{
		Texture->Modify();

		Texture->LODGroup = DefaultSpriteTextureGroup;

		if (bOverrideTextureCompression)
		{
			Texture->CompressionSettings = DefaultSpriteTextureCompression;
		}

		Texture->PostEditChange();
	}
}
