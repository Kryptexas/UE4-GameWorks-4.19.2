// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "InstancedStaticMeshComponent.generated.h"

USTRUCT()
struct FInstancedStaticMeshInstanceData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category=Instances)
	FMatrix Transform;

	UPROPERTY()
	FVector2D LightmapUVBias;

	UPROPERTY()
	FVector2D ShadowmapUVBias;


	FInstancedStaticMeshInstanceData()
		: Transform(FMatrix::Identity)
		, LightmapUVBias(ForceInit)
		, ShadowmapUVBias(ForceInit)
	{
	}


		friend FArchive& operator<<(FArchive& Ar, FInstancedStaticMeshInstanceData& InstanceData)
		{
			// @warning BulkSerialize: FInstancedStaticMeshInstanceData is serialized as memory dump
			// See TArray::BulkSerialize for detailed description of implied limitations.
			Ar << InstanceData.Transform << InstanceData.LightmapUVBias << InstanceData.ShadowmapUVBias;
			return Ar;
		}
	
};

USTRUCT()
struct FInstancedStaticMeshMappingInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	class UTexture2D* LightmapTexture;


	FInstancedStaticMeshMappingInfo()
		: LightmapTexture(NULL)
	{
	}


	class FInstancedStaticMeshStaticLightingTextureMapping*		Mapping;

	class FInstancedLightMap2D*									Lightmap;
};

/** 
 * A static mesh that can have multiple instances of the same mesh.
 */
UCLASS(HeaderGroup=Component, ClassGroup=Rendering, meta=(BlueprintSpawnableComponent), MinimalAPI)
class UInstancedStaticMeshComponent : public UStaticMeshComponent
{
	GENERATED_UCLASS_BODY()

	/** Array of instances, bulk serialized */
	UPROPERTY(EditAnywhere, Transient, DuplicateTransient, DisplayName="Instances", Category=Instances, meta=(MakeEditWidget=true))
	TArray<struct FInstancedStaticMeshInstanceData> PerInstanceSMData;

	/** Number of pending lightmaps still to be calculated (Apply()'d) */
	UPROPERTY(transient)
	int32 NumPendingLightmaps;

	/**
	 * A key for deciding which components are compatible when joining components together after a lighting build. 
	 * Will default to the staticmesh pointer when SetStaticMesh is called, so this must be set after calling
	 * SetStaticMesh on the component
	 */
	UPROPERTY()
	int32 ComponentJoinKey;

	/** The mappings for all the instances of this component */
	UPROPERTY(transient)
	TArray<struct FInstancedStaticMeshMappingInfo> CachedMappings;

	/** Value used to seed the random number stream that generates random numbers for each of this mesh's instances.
		The random number is stored in a buffer accessible to materials through the PerInstanceRandom expression.  If
		this is set to zero (default), it will be populated automatically by the editor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InstancedStaticMeshComponent)
	int32 InstancingRandomSeed;

	/** Distance from camera at which each instance begins to fade out */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Culling)
	int32 InstanceStartCullDistance;

	/** Distance from camera at which each instance completely fades out */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Culling)
	int32 InstanceEndCullDistance;

	UFUNCTION(BlueprintCallable, Category="Components|InstancedStaticMesh")
	virtual void AddInstance(const FTransform& InstanceTransform);


public:
#if WITH_EDITOR
	/** One bit per instance if the instance is selected. */
	TBitArray<> SelectedInstances;
#endif

#if WITH_PHYSX
	/** Aggregate physx representation of the instances' bodies. */
	class physx::PxAggregate* Aggregate;
#endif	//WITH_PHYSX

	/** Physics representation of the instance bodies */
	TArray<FBodyInstance*> InstanceBodies;

	// Begin UActorComponent interface 
	virtual void GetComponentInstanceData(FComponentInstanceDataCache& Cache) const OVERRIDE;
	virtual void ApplyComponentInstanceData(const FComponentInstanceDataCache& Cache) OVERRIDE;
	// End UActorComponent interface 

	// Begin UPrimitiveComponent Interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;
	virtual void CreatePhysicsState() OVERRIDE;
	virtual void DestroyPhysicsState() OVERRIDE;

	virtual FBoxSphereBounds CalcBounds(const FTransform& BoundTransform) const OVERRIDE;
#if WITH_EDITOR
	virtual void GetStaticLightingInfo(FStaticLightingPrimitiveInfo& OutPrimitiveInfo,const TArray<ULightComponent*>& InRelevantLights,const FLightingBuildOptions& Options) OVERRIDE;
#endif
	virtual void GetLightAndShadowMapMemoryUsage( int32& LightMapMemoryUsage, int32& ShadowMapMemoryUsage ) const OVERRIDE;
	virtual void ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) OVERRIDE;
	// End UPrimitiveComponent Interface

	//Begin UObject Interface
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	virtual SIZE_T GetResourceSize(EResourceSizeMode::Type Mode) OVERRIDE;
#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) OVERRIDE;
#endif
	//End UObject Interface

	/** Check to see if an instance is selected */
	ENGINE_API bool IsInstanceSelected(int32 InInstanceIndex) const;

	/** Select/deselect an instance or group of instances */
	ENGINE_API void SelectInstance(bool bInSelected, int32 InInstanceIndex, int32 InInstanceCount = 1);

private:
	/** Initializes the body instance for the specified instance of the static mesh*/
	void InitInstanceBody(int32 InstanceIdx, FBodyInstance* BodyInstance);

	/** Sets up new instance data to sensible defaults, creates physics counterparts if possible */
	void SetupNewInstanceData(FInstancedStaticMeshInstanceData& InOutNewInstanceData, int32 InInstanceIndex, const FTransform& InInstanceTransform);

};

