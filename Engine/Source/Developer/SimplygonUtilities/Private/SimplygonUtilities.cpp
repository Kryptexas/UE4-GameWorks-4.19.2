// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SimplygonUtilitiesPrivatePCH.h"

#include "SimplygonUtilitiesModule.h"

#include "RawMesh.h"
#include "MeshUtilities.h"
#include "MaterialExportUtils.h"

#define LOCTEXT_NAMESPACE "SimplygonUtilities"

bool FSimplygonUtilities::CurrentlyRendering = false;
TArray<UTextureRenderTarget2D*> FSimplygonUtilities::RTPool;

IMPLEMENT_MODULE( FSimplygonUtilities, SimplygonUtilities )

//#define MULTIMATERIAL_TEST		1


void FSimplygonUtilities::StartupModule()
{
	FCoreUObjectDelegates::PreGarbageCollect.AddRaw(this, &FSimplygonUtilities::OnPreGarbageCollect);
}

void FSimplygonUtilities::ShutdownModule()
{
	FCoreUObjectDelegates::PreGarbageCollect.RemoveAll(this);
	ClearRTPool();
}

void FSimplygonUtilities::OnPreGarbageCollect()
{
	ClearRTPool();
}

FMeshMaterialReductionData::FMeshMaterialReductionData()
	: bReleaseMesh(false)
	, Mesh(nullptr)
	, LODModel(nullptr)
	, StaticMesh(nullptr)
{ }

FMeshMaterialReductionData::~FMeshMaterialReductionData()
{
	if (bReleaseMesh)
	{
		delete Mesh;
	}
}

FMeshMaterialReductionData* FSimplygonUtilities::NewMeshReductionData()
{
	return new FMeshMaterialReductionData();
}

void FMeshMaterialReductionData::BuildOutputMaterialMap(const FSimplygonMaterialLODSettings& MaterialLODSettings, bool bAllowMultiMaterial)
{
	if (NonFlattenMaterials.Num() == 0)
	{
		UE_LOG(LogSimplygonUtilities, Error, TEXT("(Simplygon) BuildOutputMaterialMap failed - no input materials provided"));
		return;
	}

#if MULTIMATERIAL_TEST
	if (bAllowMultiMaterial)
	{
		// For testing: generate 1:1 output material map
		for (int32 SourceMaterialIndex = 0; SourceMaterialIndex < NonFlattenMaterials.Num(); SourceMaterialIndex++)
		{
			const UMaterialInterface* Material = NonFlattenMaterials[SourceMaterialIndex];
			if (Material)
			{
				OutputMaterialMap.Add(SourceMaterialIndex);
				OutputBlendMode.Add(Material->GetBlendMode());
				OutputTwoSided.Add(Material->IsTwoSided());
			}
			else
			{
				OutputMaterialMap.Add(0);
				OutputBlendMode.Add(BLEND_Opaque);
				OutputTwoSided.Add(false);
			}
		}
		return;
	}
#endif // MULTIMATERIAL_TEST

	// Check is opacity channel enabled in settings
	bool bHasOpacityEnabled = false;
	for (int32 ChannelIndex = 0; ChannelIndex < MaterialLODSettings.ChannelsToCast.Num(); ChannelIndex++)
	{
		const FSimplygonChannelCastingSettings& Channel = MaterialLODSettings.ChannelsToCast[ChannelIndex];
		if (Channel.bActive && Channel.MaterialChannel == ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_OPACITY)
		{
			bHasOpacityEnabled = true;
			break;
		}
	}

	// Find which blend mode groups are used
	bool bHasOpaqueBlend = false;
	bool bHasTranslucentBlend = false;
	EBlendMode MaxOpaqueBlend = BLEND_Opaque;
	EBlendMode MinTranslucentBlend = BLEND_MAX;
	EBlendMode MaxTranslucentBlend = BLEND_Opaque;

	for (int32 SourceMaterialIndex = 0; SourceMaterialIndex < NonFlattenMaterials.Num(); SourceMaterialIndex++)
	{
		const UMaterialInterface* Material = NonFlattenMaterials[SourceMaterialIndex];
		if (Material)
		{
			EBlendMode BlendMode = Material->GetBlendMode();
			if (IsTranslucentBlendMode(BlendMode))
			{
				bHasTranslucentBlend = true;
				if (BlendMode < MinTranslucentBlend)
				{
					MinTranslucentBlend = BlendMode;
				}
				if (BlendMode > MaxTranslucentBlend)
				{
					MaxTranslucentBlend = BlendMode;
				}
			}
			else
			{
				// either BLEND_Opaque or BLEND_Masked
				bHasOpaqueBlend = true;
				if (BlendMode > MaxOpaqueBlend)
				{
					MaxOpaqueBlend = BlendMode;
				}
			}
		}
	}

	OutputBlendMode.Empty();
	OutputTwoSided.Empty();

	if (!bHasOpacityEnabled)
	{
		// There's no point to bake translucent materials when opacity channel is not baked
		bHasTranslucentBlend = false;
		MaxOpaqueBlend = BLEND_Opaque;
	}

	if (!bAllowMultiMaterial ||												// not allowed by Simplygon
		// QQQ !MaterialLODSettings.bAllowMultiMaterial ||							// disabled by user
		!bHasTranslucentBlend ||											// no translucency
		(!bHasOpaqueBlend && (MinTranslucentBlend == MaxTranslucentBlend)))	// no opacity, and single translucent mode
	{
		// Only single output material will be used.

		// Fill OutputMaterialMap with zeros
		OutputMaterialMap.Init(0, NonFlattenMaterials.Num());
		// Choose output material blend mode
		EBlendMode BlendMode = BLEND_Opaque;
		if (bHasOpaqueBlend && bHasTranslucentBlend)
		{
			// Both blend mode kinds are used, select BLEND_Masked
			BlendMode = BLEND_Masked;
		}
		else if (bHasTranslucentBlend)
		{
			// No opaque blends, can safely use translucency
			BlendMode = MinTranslucentBlend;
		}
		else
		{
			// Only opaque blend modes are used
			BlendMode = MaxOpaqueBlend;
		}
		OutputBlendMode.Add(BlendMode);
		// Choose TwoSided value
		bool bHasTwoSided = false;
		bool bHasSingleSided = false;
		for (int32 MaterialIndex = 0; MaterialIndex < NonFlattenMaterials.Num(); MaterialIndex++)
		{
			if (NonFlattenMaterials[MaterialIndex]->IsTwoSided())
			{
				bHasTwoSided = true;
			}
			else
			{
				bHasSingleSided = true;
			}
		}
		bool bTwoSided = bHasTwoSided && (!bHasSingleSided);// QQ || MaterialLODSettings.bPreferTwoSideMaterials);
		OutputTwoSided.Add(bTwoSided);
		return;
	}

	// Multiple output materials could be used.
	// Analyze source material blend modes and decide if we want to merge them into a single material
	for (int32 SourceMaterialIndex = 0; SourceMaterialIndex < NonFlattenMaterials.Num(); SourceMaterialIndex++)
	{
		const UMaterialInterface* Material = NonFlattenMaterials[SourceMaterialIndex];
		if (Material)
		{
			EBlendMode BlendMode = Material->GetBlendMode();
			int32 OutputMaterialIndex;
			bool TwoSided = false;// QQ MaterialLODSettings.bPreferTwoSideMaterials ? Material->IsTwoSided() : false;
			for (OutputMaterialIndex = 0; OutputMaterialIndex < OutputBlendMode.Num(); OutputMaterialIndex++)
			{
				EBlendMode OtherBlendMode = OutputBlendMode[OutputMaterialIndex];
				if (OtherBlendMode == BlendMode)
				{
					// Found exact match
					break;
				}
				// BLEND_Masked and BLEND_Opaque are compatible
				if (OtherBlendMode == BLEND_Masked && BlendMode == BLEND_Opaque)
				{
					break;
				}
				if (OtherBlendMode == BLEND_Opaque && BlendMode == BLEND_Masked)
				{
					// replace existing blend mode with BLEND_Masked
					OutputBlendMode[OutputMaterialIndex] = BLEND_Masked;
					break;
				}
			}
			if (OutputMaterialIndex >= OutputBlendMode.Num())
			{
				// This blend mode was not found (for example if OutBlendModes is currently empty), add it.
				OutputBlendMode.Add(BlendMode);
				check(OutputBlendMode.Num() == OutputMaterialIndex+1);
			}
			OutputMaterialMap.Add(OutputMaterialIndex);
		}
		else
		{
			OutputMaterialMap.Add(0);
		}
	}

	// If no materials were processed, still need valid structures
	if (OutputMaterialMap.Num() && !OutputBlendMode.Num())
	{
		OutputBlendMode.Add(BLEND_Opaque);
		OutputTwoSided.Add(false);
		return;
	}

	// Select TwoSided property value for each of output materials
	for (int32 OutMaterialIndex = 0; OutMaterialIndex < OutputBlendMode.Num(); OutMaterialIndex++)
	{
		bool bHasTwoSided = false;
		bool bHasSingleSided = false;
		// Check all NonFlattenMaterials which will be mapped to current output material
		for (int32 InMaterialIndex = 0; InMaterialIndex < NonFlattenMaterials.Num(); InMaterialIndex++)
		{
			if (OutputMaterialMap[InMaterialIndex] == OutMaterialIndex)
			{
				if (NonFlattenMaterials[InMaterialIndex]->IsTwoSided())
				{
					bHasTwoSided = true;
				}
				else
				{
					bHasSingleSided = true;
				}
			}
		}
		bool bTwoSided = bHasTwoSided && (!bHasSingleSided); // QQ || MaterialLODSettings.bPreferTwoSideMaterials);
		OutputTwoSided.Add(bTwoSided);
	}
}

void FMeshMaterialReductionData::BuildFlattenMaterials(const FSimplygonMaterialLODSettings& MaterialLODSettings, int32 TextureWidth, int32 TextureHeight)
{
	UMaterialInterface* DefaultMaterial = Cast<UMaterialInterface>(UMaterial::GetDefaultMaterial(MD_Surface));

	for (int32 MaterialIndex = 0; MaterialIndex < NonFlattenMaterials.Num(); MaterialIndex++)
	{
		UMaterialInterface* Material = NonFlattenMaterials[MaterialIndex];
		// Correctly handle NULL materials - they should take their slot in material table
		if (!Material)
		{
			UE_LOG(LogSimplygonUtilities, Warning, TEXT("(Simplygon) Material #%d does not exist. Adding default material instead."), MaterialIndex);
			Material = DefaultMaterial;
		}
		FMaterialResource* Resource = Material->GetMaterialResource(GMaxRHIFeatureLevel);
		if (!Resource)
		{
			UE_LOG(LogSimplygonUtilities, Warning, TEXT("(Simplygon) %s is missing material resource. Adding default material instead."), *Material->GetName());
			Material = DefaultMaterial;
		}

		FFlattenMaterial* FlattenMaterial = FSimplygonUtilities::CreateFlattenMaterial(MaterialLODSettings, TextureWidth, TextureHeight);
		FlattenMaterials.Add(FlattenMaterial);

		// Render FFlattenMaterial
		if (Mesh)
		{
			// StaticMesh version
			FSimplygonUtilities::ExportMaterial(
				Material,
				Mesh,
				MaterialIndex,
				TexcoordBounds[MaterialIndex],
				NewUVs,
				*FlattenMaterial);
		}
	}

	AdjustEmissiveChannels();
}

void FMeshMaterialReductionData::AdjustEmissiveChannels()
{
	// Iterate over all output materials.
	for (int32 OutputMaterialIndex = 0; OutputMaterialIndex < GetOutputMaterialCount(); OutputMaterialIndex++)
	{
		// Collect all input materials and determine maximal emissive scale
		TArray<FFlattenMaterial*> InputMaterials;
		float MaxEmissiveScale = 0.0f;
		for (int32 InputMaterialIndex = 0; InputMaterialIndex < GetInputMaterialCount(); InputMaterialIndex++)
		{
			if (OutputMaterialMap[InputMaterialIndex] == OutputMaterialIndex)
			{
				FFlattenMaterial& Material = FlattenMaterials[InputMaterialIndex];
				if (Material.EmissiveSamples.Num())
				{
					if (Material.EmissiveScale > MaxEmissiveScale)
					{
						MaxEmissiveScale = Material.EmissiveScale;
					}
					InputMaterials.Add(&Material);
				}
			}
		}

		OutputEmissiveScale.Add(MaxEmissiveScale);

		if (InputMaterials.Num() <= 1 || MaxEmissiveScale <= 0.001f)
		{
			// Nothing to do here.
			continue;
		}
		// Rescale all materials.
		for (int32 MaterialIndex = 0; MaterialIndex < InputMaterials.Num(); MaterialIndex++)
		{
			FFlattenMaterial* Material = InputMaterials[MaterialIndex];
			float Scale = Material->EmissiveScale / MaxEmissiveScale;
			if (FMath::Abs(Scale - 1.0f) < 0.01f)
			{
				// Difference is not noticeable for this material, or this material has maximal emissive level.
				continue;
			}
			// Rescale emissive data.
			for (int32 PixelIndex = 0; PixelIndex < Material->EmissiveSamples.Num(); PixelIndex++)
			{
				FColor& C = Material->EmissiveSamples[PixelIndex];
				C.R = FMath::RoundToInt(C.R * Scale);
				C.G = FMath::RoundToInt(C.G * Scale);
				C.B = FMath::RoundToInt(C.B * Scale);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
