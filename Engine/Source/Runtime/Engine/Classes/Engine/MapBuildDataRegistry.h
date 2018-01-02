// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/**
 * Registry for built data from a map build
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Misc/Guid.h"
#include "Engine/EngineTypes.h"
#include "SceneTypes.h"
#include "UObject/UObjectAnnotation.h"
#include "RenderCommandFence.h"
#include "MapBuildDataRegistry.generated.h"

class FPrecomputedLightVolumeData;
class FPrecomputedVolumetricLightmapData;

struct ENGINE_API FPerInstanceLightmapData
{
	FVector2D LightmapUVBias;
	FVector2D ShadowmapUVBias;

	FPerInstanceLightmapData()
		: LightmapUVBias(ForceInit)
		, ShadowmapUVBias(ForceInit)
	{}

	friend FArchive& operator<<(FArchive& Ar, FPerInstanceLightmapData& InstanceData)
	{
		// @warning BulkSerialize: FPerInstanceLightmapData is serialized as memory dump
		// See TArray::BulkSerialize for detailed description of implied limitations.
		Ar << InstanceData.LightmapUVBias << InstanceData.ShadowmapUVBias;
		return Ar;
	}
};

class FMeshMapBuildData
{
public:
	FLightMapRef LightMap;
	FShadowMapRef ShadowMap;
	TArray<FGuid> IrrelevantLights;
	TArray<FPerInstanceLightmapData> PerInstanceLightmapData;

	ENGINE_API FMeshMapBuildData();

	/** Destructor. */
	ENGINE_API ~FMeshMapBuildData();

	/**
	 * Determine if this annotation is the default
	 * @return true is this is a default annotation
	 */
	FORCEINLINE bool IsDefault()
	{
		return LightMap == DefaultAnnotation.LightMap && ShadowMap == DefaultAnnotation.ShadowMap;
	}

	ENGINE_API void AddReferencedObjects(FReferenceCollector& Collector);

	/** Serializer. */
	friend ENGINE_API FArchive& operator<<(FArchive& Ar,FMeshMapBuildData& MeshMapBuildData);

	/** Default state for annotations (no flags changed)*/
	static const FMeshMapBuildData DefaultAnnotation;
};

class FMeshMapBuildLegacyData
{
public:

	TArray<TPair<FGuid, FMeshMapBuildData*>> Data;

	/**
	 * Determine if this annotation is the default
	 * @return true is this is a default annotation
	 */
	FORCEINLINE bool IsDefault()
	{
		return Data.Num() == 0;
	}
};

class FStaticShadowDepthMapData
{
public:
	/** Transform from world space to the coordinate space that DepthSamples are stored in. */
	FMatrix WorldToLight;
	/** Dimensions of the generated shadow map. */
	int32 ShadowMapSizeX;
	int32 ShadowMapSizeY;
	/** Shadowmap depth values */
	TArray<FFloat16> DepthSamples;

	FStaticShadowDepthMapData() :
		WorldToLight(FMatrix::Identity),
		ShadowMapSizeX(0),
		ShadowMapSizeY(0)
	{}

	ENGINE_API void Empty();

	size_t GetAllocatedSize() const
	{
		return DepthSamples.GetAllocatedSize();
	}
	friend FArchive& operator<<(FArchive& Ar, FStaticShadowDepthMapData& ShadowMap);
};

class FLevelLegacyMapBuildData
{
public:

	FGuid Id;
	FPrecomputedLightVolumeData* Data;

	FLevelLegacyMapBuildData() :
		Data(NULL)
	{}

	/**
	 * Determine if this annotation is the default
	 * @return true is this is a default annotation
	 */
	FORCEINLINE bool IsDefault()
	{
		return Id == FGuid();
	}
};

class FLightComponentMapBuildData
{
public:

	FLightComponentMapBuildData() :
		ShadowMapChannel(INDEX_NONE)
	{}

	ENGINE_API ~FLightComponentMapBuildData();

	ENGINE_API void FinalizeLoad();

	/** 
	 * Shadow map channel which is used to match up with the appropriate static shadowing during a deferred shading pass.
	 * This is generated during a lighting build.
	 */
	int32 ShadowMapChannel;

	FStaticShadowDepthMapData DepthMap;

	friend FArchive& operator<<(FArchive& Ar, FLightComponentMapBuildData& ShadowMap);
};

class FLightComponentLegacyMapBuildData
{
public:

	FGuid Id;
	FLightComponentMapBuildData* Data;

	FLightComponentLegacyMapBuildData() :
		Data(NULL)
	{}

	/**
	 * Determine if this annotation is the default
	 * @return true is this is a default annotation
	 */
	FORCEINLINE bool IsDefault()
	{
		return Id == FGuid();
	}
};

class FReflectionCaptureData
{
public:
	int32 CubemapSize;
	float AverageBrightness;
	/** Needed for EncodedHDRCapturedData cooking. */
	float Brightness;

	TArray<uint8> FullHDRCapturedData;
	TArray<uint8> EncodedHDRCapturedData;

	FReflectionCaptureData() :
		CubemapSize(0),
		AverageBrightness(0.0f),
		Brightness(0.0f),
		bUploadedFinal(false)
	{}

	bool HasBeenUploadedFinal() const
	{
		return bUploadedFinal;
	}

	void OnDataUploadedToGPUFinal()
	{
		check(!bUploadedFinal);

		// In the editor we need this data for serialization
		if (!GIsEditor)
		{
			FullHDRCapturedData.Empty();
			EncodedHDRCapturedData.Empty();
			CubemapSize = 0;
			bUploadedFinal = true;
		}
	}

private:
	bool bUploadedFinal;
};

class FReflectionCaptureMapBuildData : public FReflectionCaptureData
{
public:

	// Stored redundantly to support stats after discarding data
	size_t AllocatedSize;

	FReflectionCaptureMapBuildData() :
		AllocatedSize(0)
	{}

	ENGINE_API ~FReflectionCaptureMapBuildData();

	size_t GetAllocatedSize() const
	{
		return AllocatedSize;
	}

	ENGINE_API void FinalizeLoad();

	/**
	 * Determine if this annotation is the default
	 * @return true is this is a default annotation
	 */
	FORCEINLINE bool IsDefault()
	{
		return CubemapSize == DefaultAnnotation.CubemapSize && FullHDRCapturedData.Num() == DefaultAnnotation.FullHDRCapturedData.Num();
	}

	/** Serializer. */
	friend ENGINE_API FArchive& operator<<(FArchive& Ar,FReflectionCaptureMapBuildData& ReflectionCaptureMapBuildData);

	/** Default state for annotations (no flags changed)*/
	static const FReflectionCaptureMapBuildData DefaultAnnotation;
};

class FReflectionCaptureMapBuildLegacyData
{
public:

	FGuid Id;
	FReflectionCaptureMapBuildData* MapBuildData;

	/**
	 * Determine if this annotation is the default
	 * @return true is this is a default annotation
	 */
	FORCEINLINE bool IsDefault()
	{
		return Id == FGuid();
	}
};

UCLASS(MinimalAPI)
class UMapBuildDataRegistry : public UObject
{
	GENERATED_UCLASS_BODY()
public:

	/** The lighting quality the level was last built with */
	UPROPERTY(Category=Lighting, VisibleAnywhere)
	TEnumAsByte<enum ELightingBuildQuality> LevelLightingQuality;

	//~ Begin UObject Interface
	ENGINE_API virtual void Serialize(FArchive& Ar) override;
	ENGINE_API virtual void PostLoad() override;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	ENGINE_API virtual void BeginDestroy() override;
	ENGINE_API virtual bool IsReadyForFinishDestroy() override;
	ENGINE_API virtual void FinishDestroy() override;
	//~ End UObject Interface

	/** 
	 * Allocates a new FMeshMapBuildData from the registry.
	 * Warning: Further allocations will invalidate the returned reference.
	 */
	ENGINE_API FMeshMapBuildData& AllocateMeshBuildData(const FGuid& MeshId, bool bMarkDirty);
	ENGINE_API const FMeshMapBuildData* GetMeshBuildData(FGuid MeshId) const;
	ENGINE_API FMeshMapBuildData* GetMeshBuildData(FGuid MeshId);

	/** 
	 * Allocates a new FPrecomputedLightVolumeData from the registry.
	 * Warning: Further allocations will invalidate the returned reference.
	 */
	ENGINE_API FPrecomputedLightVolumeData& AllocateLevelPrecomputedLightVolumeBuildData(const FGuid& LevelId);
	ENGINE_API void AddLevelPrecomputedLightVolumeBuildData(const FGuid& LevelId, FPrecomputedLightVolumeData* InData);
	ENGINE_API const FPrecomputedLightVolumeData* GetLevelPrecomputedLightVolumeBuildData(FGuid LevelId) const;
	ENGINE_API FPrecomputedLightVolumeData* GetLevelPrecomputedLightVolumeBuildData(FGuid LevelId);

	/** 
	 * Allocates a new FPrecomputedVolumetricLightmapData from the registry.
	 * Warning: Further allocations will invalidate the returned reference.
	 */
	ENGINE_API FPrecomputedVolumetricLightmapData& AllocateLevelPrecomputedVolumetricLightmapBuildData(const FGuid& LevelId);
	ENGINE_API void AddLevelPrecomputedVolumetricLightmapBuildData(const FGuid& LevelId, FPrecomputedVolumetricLightmapData* InData);
	ENGINE_API const FPrecomputedVolumetricLightmapData* GetLevelPrecomputedVolumetricLightmapBuildData(FGuid LevelId) const;
	ENGINE_API FPrecomputedVolumetricLightmapData* GetLevelPrecomputedVolumetricLightmapBuildData(FGuid LevelId);

	/** 
	 * Allocates a new FLightComponentMapBuildData from the registry.
	 * Warning: Further allocations will invalidate the returned reference.
	 */
	ENGINE_API FLightComponentMapBuildData& FindOrAllocateLightBuildData(FGuid LightId, bool bMarkDirty);
	ENGINE_API const FLightComponentMapBuildData* GetLightBuildData(FGuid LightId) const;
	ENGINE_API FLightComponentMapBuildData* GetLightBuildData(FGuid LightId);

	/** 
	 * Allocates a new FMeshMapBuildData from the registry.
	 * Warning: Further allocations will invalidate the returned reference.
	 */
	ENGINE_API FReflectionCaptureMapBuildData& AllocateReflectionCaptureBuildData(const FGuid& CaptureId, bool bMarkDirty);
	ENGINE_API const FReflectionCaptureMapBuildData* GetReflectionCaptureBuildData(FGuid CaptureId) const;
	ENGINE_API FReflectionCaptureMapBuildData* GetReflectionCaptureBuildData(FGuid CaptureId);

	ENGINE_API void InvalidateStaticLighting(UWorld* World, const TSet<FGuid>* ResourcesToKeep = nullptr);
	ENGINE_API void InvalidateReflectionCaptures(const TSet<FGuid>* ResourcesToKeep = nullptr);

	ENGINE_API bool IsLegacyBuildData() const;

private:

	ENGINE_API void ReleaseResources(const TSet<FGuid>* ResourcesToKeep = nullptr);
	ENGINE_API void EmptyLevelData(const TSet<FGuid>* ResourcesToKeep = nullptr);

	TMap<FGuid, FMeshMapBuildData> MeshBuildData;
	TMap<FGuid, FPrecomputedLightVolumeData*> LevelPrecomputedLightVolumeBuildData;
	TMap<FGuid, FPrecomputedVolumetricLightmapData*> LevelPrecomputedVolumetricLightmapBuildData;
	TMap<FGuid, FLightComponentMapBuildData> LightBuildData;
	TMap<FGuid, FReflectionCaptureMapBuildData> ReflectionCaptureBuildData;

	FRenderCommandFence DestroyFence;
};

extern ENGINE_API FUObjectAnnotationSparse<FMeshMapBuildLegacyData, true> GComponentsWithLegacyLightmaps;
extern ENGINE_API FUObjectAnnotationSparse<FLevelLegacyMapBuildData, true> GLevelsWithLegacyBuildData;
extern ENGINE_API FUObjectAnnotationSparse<FLightComponentLegacyMapBuildData, true> GLightComponentsWithLegacyBuildData;
extern ENGINE_API FUObjectAnnotationSparse<FReflectionCaptureMapBuildLegacyData, true> GReflectionCapturesWithLegacyBuildData;