// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
TextureStreamingBuild.cpp : Contains definitions to build texture streaming data.
=============================================================================*/

#include "EnginePrivate.h"
#include "DebugViewModeMaterialProxy.h"
#include "ShaderCompiler.h"

/**
 * Build Shaders to compute tex coord scale per texture.
 *
 * @param InWorld			The world to build streaming for.
 * @param QualityLevel		The quality level for the shaders.
 * @param FeatureLevel		The feature level for the shaders.
 * @param TexCoordScales	The map to store material interfaces used by the current primitive world.
 */
ENGINE_API void BuildTextureStreamingShaders(UWorld* InWorld, EMaterialQualityLevel::Type QualityLevel, ERHIFeatureLevel::Type FeatureLevel, OUT FTexCoordScaleMap& TexCoordScales)
{
#if WITH_EDITORONLY_DATA

	if (!InWorld) return;

	const bool bUseNewMetrics = CVarStreamingUseNewMetrics.GetValueOnGameThread() != 0;
	const double StartTime = FPlatformTime::Seconds();

	FlushRenderingCommands();

	FDebugViewModeMaterialProxy::ClearAllShaders();

	// Without the new metrics, the shaders will are not compiled, making the texcoord scale viewmode non functional.
	if (bUseNewMetrics && AllowDebugViewPS(DVSM_MaterialTexCoordScalesAnalysis, GetFeatureLevelShaderPlatform(FeatureLevel)))
	{
		for (int32 LevelIndex = 0; LevelIndex < InWorld->GetNumLevels(); LevelIndex++)
		{
			ULevel* Level = InWorld->GetLevel(LevelIndex);

			TArray<UObject*> ObjectsInOuter;
			GetObjectsWithOuter(Level, ObjectsInOuter);

			for (int32 Index = 0; Index < ObjectsInOuter.Num(); Index++)
			{
				UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(ObjectsInOuter[Index]);
				if (!Primitive) continue;

				TArray<UMaterialInterface*> Materials;
				Primitive->GetUsedMaterials(Materials);

				for (int32 MaterialIndex = 0; MaterialIndex < Materials.Num(); ++MaterialIndex)
				{
					UMaterialInterface* MaterialInterface = Materials[MaterialIndex];
					if (!MaterialInterface) continue;

					if (!TexCoordScales.Contains(MaterialInterface))
					{
						TexCoordScales.Add(MaterialInterface);
					}

					FDebugViewModeMaterialProxy::AddShader(MaterialInterface, QualityLevel, FeatureLevel, EMaterialShaderMapUsage::DebugViewModeTexCoordScale);
				}
			}
		}

		// This could be very slow if there were already other shaders compiling
		GShaderCompilingManager->FinishAllCompilation();
	}


	UE_LOG(LogLevel, Display, TEXT("Build Texture Streaming Shaders took %.3f seconds."), FPlatformTime::Seconds() - StartTime);
#else
	UE_LOG(LogLevel, Fatal,TEXT("Build Texture Streaming Shaders should not be called on a console"));
#endif
}

ENGINE_API void BuildTextureStreamingData(UWorld* InWorld, const FTexCoordScaleMap& InTexCoordScales, EMaterialQualityLevel::Type QualityLevel, ERHIFeatureLevel::Type FeatureLevel)
{
#if WITH_EDITORONLY_DATA
	if (!InWorld) return;
	const double StartTime = FPlatformTime::Seconds();

	// ====================================================
	// Build per primitive data
	// ====================================================

	for (int32 LevelIndex = 0; LevelIndex < InWorld->GetNumLevels(); LevelIndex++)
	{
		ULevel* Level = InWorld->GetLevel(LevelIndex);
		if (!Level) continue;

		TArray<UObject*> ObjectsInOuter;
		GetObjectsWithOuter(Level, ObjectsInOuter);

		TArray<UTexture2D*> LevelTextures;
		for (int32 Index = 0; Index < ObjectsInOuter.Num(); Index++)
		{
			UStaticMeshComponent* Primitive = Cast<UStaticMeshComponent>(ObjectsInOuter[Index]);
			if (!Primitive) continue;

			Primitive->UpdateStreamingTextureData(LevelTextures, InTexCoordScales, QualityLevel, FeatureLevel);
		}

		Level->bTextureStreamingRotationChanged = false;
		Level->StreamingTextureGuids.Empty(LevelTextures.Num());

		// Reset LevelIndex to default for next use and build the level Guid array.
		for (int32 TextureIndex = 0; TextureIndex < LevelTextures.Num(); ++TextureIndex)
		{
			UTexture2D* Texture2D = LevelTextures[TextureIndex];
			Level->StreamingTextureGuids.Push(Texture2D->GetLightingGuid());
			Texture2D->LevelIndex = INDEX_NONE;
		}
	}

	// ====================================================
	// Update texture streaming data
	// ====================================================

	ULevel::BuildStreamingData(InWorld);

	// ====================================================
	// Reregister everything for debug view modes to reflect changes
	// ====================================================

	for (int32 LevelIndex = 0; LevelIndex < InWorld->GetNumLevels(); LevelIndex++)
	{
		ULevel* Level = InWorld->GetLevel(LevelIndex);
		if (!Level) continue;
		
		for (AActor* Actor : Level->Actors)
		{
			if (!Actor) continue;

			Actor->MarkComponentsRenderStateDirty();
		}
	}
	UE_LOG(LogLevel, Display, TEXT("Build Texture Streaming took %.3f seconds."), FPlatformTime::Seconds() - StartTime);
#else
	UE_LOG(LogLevel, Fatal,TEXT("Build Texture Streaming should not be called on a console"));
#endif
}


static uint32 PackRelativeBox(const FVector& RefOrigin, const FVector& RefExtent, const FVector& Origin, const FVector& Extent)
{
	const FVector RefMin = RefOrigin - RefExtent;
	// 15.5 and 31.5 have the / 2 scale included 
	const FVector PackScale = FVector(15.5f, 15.5f, 31.5f) / RefExtent.ComponentMax(FVector(KINDA_SMALL_NUMBER, KINDA_SMALL_NUMBER, KINDA_SMALL_NUMBER));

	const FVector Min = Origin - Extent;
	const FVector Max = Origin + Extent;

	const FVector RelMin = (Min - RefMin) * PackScale;
	const FVector RelMax = (Max - RefMin) * PackScale;

	const uint32 PackedMinX = (uint32)FMath::Clamp<int32>(FMath::FloorToInt(RelMin.X), 0, 31);
	const uint32 PackedMinY = (uint32)FMath::Clamp<int32>(FMath::FloorToInt(RelMin.Y), 0, 31);
	const uint32 PackedMinZ = (uint32)FMath::Clamp<int32>(FMath::FloorToInt(RelMin.Z), 0, 63);

	const uint32 PackedMaxX = (uint32)FMath::Clamp<int32>(FMath::CeilToInt(RelMax.X), 0, 31);
	const uint32 PackedMaxY = (uint32)FMath::Clamp<int32>(FMath::CeilToInt(RelMax.Y), 0, 31);
	const uint32 PackedMaxZ = (uint32)FMath::Clamp<int32>(FMath::CeilToInt(RelMax.Z), 0, 63);

	return PackedMinX | (PackedMinY << 5) | (PackedMinZ << 10) | (PackedMaxX << 16) | (PackedMaxY << 21) | (PackedMaxZ << 26);
}

static void UnpackRelativeBox(const FVector& InRefOrigin, const FVector& InRefExtent, uint32 InPackedRelBox, FVector& OutOrigin, FVector& OutExtent)
{
	const uint32 PackedMinX = InPackedRelBox & 31;
	const uint32 PackedMinY = (InPackedRelBox >> 5) & 31;
	const uint32 PackedMinZ = (InPackedRelBox >> 10) & 63;

	const uint32 PackedMaxX = (InPackedRelBox >> 16) & 31;
	const uint32 PackedMaxY = (InPackedRelBox >> 21) & 31;
	const uint32 PackedMaxZ = (InPackedRelBox >> 26) & 63;

	const FVector RefMin = InRefOrigin - InRefExtent;
	// 15.5 and 31.5 have the / 2 scale included 
	const FVector UnpackScale = InRefExtent.ComponentMax(FVector(KINDA_SMALL_NUMBER, KINDA_SMALL_NUMBER, KINDA_SMALL_NUMBER)) / FVector(15.5f, 15.5f, 31.5f);

	const FVector Min = FVector((float)PackedMinX, (float)PackedMinY, (float)PackedMinZ) * UnpackScale + RefMin;
	const FVector Max = FVector((float)PackedMaxX, (float)PackedMaxY, (float)PackedMaxZ) * UnpackScale + RefMin;

	OutOrigin = .5 * (Min + Max);
	OutExtent = .5 * (Max - Min);
}

void FStreamingTexturePrimitiveInfo::UnPackFrom(UTexture2D* InTexture, const FBoxSphereBounds& RefBounds, const FStreamingTextureBuildInfo& Info, bool bUseRelativeBox)
{
	check(InTexture->LevelIndex == Info.TextureLevelIndex);
	Texture = InTexture;

	if (bUseRelativeBox)
	{
		UnpackRelativeBox(RefBounds.Origin, RefBounds.BoxExtent, Info.PackedRelativeBox, Bounds.Origin, Bounds.BoxExtent);
		Bounds.SphereRadius = Bounds.BoxExtent.Size();
	}
	else
	{
		Bounds = RefBounds;
	}

	TexelFactor = Info.TexelFactor;
}

void FStreamingTextureBuildInfo::PackFrom(TArray<UTexture2D*>& LevelTextures, const FBoxSphereBounds& RefBounds, const FStreamingTexturePrimitiveInfo& Info)
{
	PackedRelativeBox = PackRelativeBox(RefBounds.Origin, RefBounds.BoxExtent, Info.Bounds.Origin, Info.Bounds.BoxExtent);

	UTexture2D* Texture2D = CastChecked<UTexture2D>(Info.Texture);
	if (Texture2D->LevelIndex == INDEX_NONE)
	{
		// If this is the first time this texture gets processed in the packing process, encode it.
		Texture2D->LevelIndex = LevelTextures.Add(Texture2D);
	}
	TextureLevelIndex = (uint16)Texture2D->LevelIndex;

	TexelFactor = Info.TexelFactor;
}


FStreamingTextureLevelContext::FStreamingTextureLevelContext(const ULevel* InLevel) :
	bUseRelativeBoxes(InLevel ? !InLevel->bTextureStreamingRotationChanged : false),
	ComponentTimestamp(0),
	ComponentBounds(ForceInitToZero),
	ComponentMeshScale(1.f),
	ComponentStreamingScale(1.f)
{
	if (InLevel)
	{
		for (int32 TextureIndex = 0; TextureIndex < InLevel->StreamingTextureGuids.Num(); ++TextureIndex)
		{
			TextureGuidToLevelIndex.Add(InLevel->StreamingTextureGuids[TextureIndex], TextureIndex);
		}
		BoundStates.AddZeroed(InLevel->StreamingTextureGuids.Num());
	}
}

FStreamingTextureLevelContext::~FStreamingTextureLevelContext()
{
	// Reset the level indices for the next use.
	for (UTexture2D* Texture : ProcessedTextures)
	{
		Texture->LevelIndex = INDEX_NONE;
	}
}

void FStreamingTextureLevelContext::BindComponent(const TArray<FStreamingTextureBuildInfo>* BuildData, const FBoxSphereBounds& Bounds, float MeshScale, float StreamingScale)
{
	// First processed component must use a timestamp > 0  so that default value of 0 is invalid.
	++ComponentTimestamp;

	ComponentBounds = Bounds;
	ComponentMeshScale = MeshScale;
	ComponentStreamingScale = StreamingScale;
	ComponentBuildData = BuildData;

	if (BuildData)
	{
		for (int32 Index = 0; Index < BuildData->Num(); ++Index)
		{
			int32 TextureLevelIndex = (*BuildData)[Index].TextureLevelIndex;
			if (BoundStates.IsValidIndex(TextureLevelIndex))
			{
				FTextureBoundState& BoundState = BoundStates[TextureLevelIndex];
				BoundState.BuildDataIndex = Index;
				BoundState.BuildDataTimestamp = ComponentTimestamp;
			}
		}
	}
}

void FStreamingTextureLevelContext::Process(const TArray<UTexture*>& InTextures, TArray<FStreamingTexturePrimitiveInfo>& OutInfos)
{
	for (int32 TextureIndex = 0; TextureIndex < InTextures.Num(); ++TextureIndex)
	{
		UTexture2D* Texture2D = Cast<UTexture2D>(InTextures[TextureIndex]);
		if (!Texture2D) continue; // Only interested in 2d texture for streaming

		if (Texture2D->LevelIndex == INDEX_NONE)
		{
			int32* LevelIndex = TextureGuidToLevelIndex.Find(Texture2D->GetLightingGuid());
			if (LevelIndex)
			{
				Texture2D->LevelIndex = *LevelIndex;
			}
			else // If it is not in build data, add an entry so that we can process it in a uniform way.
			{
				Texture2D->LevelIndex = BoundStates.AddZeroed();
			}
			ProcessedTextures.Push(Texture2D);
		}

		FTextureBoundState& BoundState = BoundStates[Texture2D->LevelIndex];

		// Ignore this texture if it has already been handled for the current component.
		if (BoundState.Timestamp != ComponentTimestamp)
		{
			FStreamingTexturePrimitiveInfo& StreamingTexture = *new(OutInfos) FStreamingTexturePrimitiveInfo;

			// Check if there is any precomputed data
			if (BoundState.BuildDataTimestamp == ComponentTimestamp && ComponentBuildData)
			{
				StreamingTexture.UnPackFrom(Texture2D, ComponentBounds, (*ComponentBuildData)[BoundState.BuildDataIndex], bUseRelativeBoxes);
			}
			else // Otherwise add default entry.
			{
				StreamingTexture.Texture = Texture2D;
				StreamingTexture.Bounds = ComponentBounds;
				StreamingTexture.TexelFactor = ComponentMeshScale;
			}

			// Component scale is not included in the build data, so that changing it does not invalidate the data.
			StreamingTexture.TexelFactor *= ComponentStreamingScale;

			// Mark this texture as processed for this component.
			BoundState.Timestamp = ComponentTimestamp;
		}
	}
}
