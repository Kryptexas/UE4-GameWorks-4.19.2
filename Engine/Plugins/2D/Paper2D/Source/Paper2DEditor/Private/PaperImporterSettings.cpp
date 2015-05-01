// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "PaperImporterSettings.h"
#include "AlphaBitmap.h"

//////////////////////////////////////////////////////////////////////////
// UPaperImporterSettings

UPaperImporterSettings::UPaperImporterSettings()
	: bPickBestMaterialWhenCreatingSprite(true)
	, DefaultPixelsPerUnrealUnit(1.0f)
	, DefaultSpriteTextureGroup(TEXTUREGROUP_Pixels2D)
	, bOverrideTextureCompression(true)
	, DefaultSpriteTextureCompression(TC_EditorIcon)
{
	NormalMapTextureSuffixes.Add(TEXT("_N"));
	NormalMapTextureSuffixes.Add(TEXT("_Normal"));

	BaseMapTextureSuffixes.Add(TEXT("_D"));
	BaseMapTextureSuffixes.Add(TEXT("_Diffuse"));

	if (!IsRunningCommandlet())
	{
		UnlitDefaultMaskedMaterialName = FStringAssetReference("/Paper2D/MaskedUnlitSpriteMaterial.MaskedUnlitSpriteMaterial");
		UnlitDefaultTranslucentMaterialName = FStringAssetReference("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial");
		UnlitDefaultOpaqueMaterialName = FStringAssetReference("/Paper2D/OpaqueUnlitSpriteMaterial.OpaqueUnlitSpriteMaterial");

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

void UPaperImporterSettings::ApplySettingsForSpriteInit(FSpriteAssetInitParameters& InitParams, ESpriteInitMaterialLightingMode LightingMode, ESpriteInitMaterialType MaterialTypeMode) const
{
	InitParams.SetPixelsPerUnrealUnit(DefaultPixelsPerUnrealUnit);

	ESpriteInitMaterialType DesiredMaterialType = MaterialTypeMode;
	if (DesiredMaterialType == ESpriteInitMaterialType::Automatic)
	{
		// Analyze the texture if desired (to see if it's got greyscale alpha or just binary alpha, picking either a translucent or masked material)
		if (bPickBestMaterialWhenCreatingSprite && (InitParams.Texture != nullptr))
		{
			FAlphaBitmap AlphaBitmap(InitParams.Texture);
			bool bHasIntermediateValues;
			bool bHasZeros;
			AlphaBitmap.AnalyzeImage((int32)InitParams.Offset.X, (int32)InitParams.Offset.Y, (int32)InitParams.Dimension.X, (int32)InitParams.Dimension.Y, /*out*/ bHasZeros, /*out*/ bHasIntermediateValues);

			if (bAnalysisCanUseOpaque && !bHasIntermediateValues && !bHasZeros)
			{
				DesiredMaterialType = ESpriteInitMaterialType::Opaque;
			}
			else
			{
				DesiredMaterialType = bHasIntermediateValues ? ESpriteInitMaterialType::Translucent : ESpriteInitMaterialType::Masked;
			}
		}
	}

	if (DesiredMaterialType == ESpriteInitMaterialType::Automatic)
	{
		// Fall back to masked if we wanted automatic and couldn't analyze things
		DesiredMaterialType = ESpriteInitMaterialType::Masked;
	}

	if (DesiredMaterialType != ESpriteInitMaterialType::LeaveAsIs)
	{
		// Determine whether to use lit or unlit materials
		const bool bUseLitMaterial = LightingMode == ESpriteInitMaterialLightingMode::ForceLit;

		// Apply the materials
		switch (DesiredMaterialType)
		{
		default:
		case ESpriteInitMaterialType::LeaveAsIs:
		case ESpriteInitMaterialType::Automatic:
			check(false);
		case ESpriteInitMaterialType::Masked:
			InitParams.DefaultMaterialOverride = GetDefaultMaskedMaterial(bUseLitMaterial);
			break;
		case ESpriteInitMaterialType::Translucent:
			InitParams.DefaultMaterialOverride = GetDefaultTranslucentMaterial(bUseLitMaterial);
			break;
		case ESpriteInitMaterialType::Opaque:
			InitParams.DefaultMaterialOverride = GetDefaultOpaqueMaterial(bUseLitMaterial);
			break;
		}

		InitParams.AlternateMaterialOverride = GetDefaultOpaqueMaterial(bUseLitMaterial);
	}
}

UMaterialInterface* UPaperImporterSettings::GetDefaultTranslucentMaterial(bool bLit) const
{
	return Cast<UMaterialInterface>((bLit ? LitDefaultTranslucentMaterialName : UnlitDefaultTranslucentMaterialName).TryLoad());
}

UMaterialInterface* UPaperImporterSettings::GetDefaultOpaqueMaterial(bool bLit) const
{
	return Cast<UMaterialInterface>((bLit ? LitDefaultOpaqueMaterialName : UnlitDefaultOpaqueMaterialName).TryLoad());
}

UMaterialInterface* UPaperImporterSettings::GetDefaultMaskedMaterial(bool bLit) const
{
	return Cast<UMaterialInterface>((bLit ? LitDefaultMaskedMaterialName : UnlitDefaultMaskedMaterialName).TryLoad());
}
