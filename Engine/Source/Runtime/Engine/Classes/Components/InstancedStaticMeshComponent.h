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

/** A component that efficiently renders multiple instances of the same StaticMesh. */
UCLASS(ClassGroup=Rendering, meta=(BlueprintSpawnableComponent))
class ENGINE_API UInstancedStaticMeshComponent : public UStaticMeshComponent
{
	GENERATED_UCLASS_BODY()

	/** Array of instances, bulk serialized */
	UPROPERTY(EditAnywhere, Transient, DuplicateTransient, DisplayName="Instances", Category=Instances, meta=(MakeEditWidget=true))
	TArray<struct FInstancedStaticMeshInstanceData> PerInstanceSMData;

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

	/** Add an instance to this component. Transform is given in local space of this component.  */
	UFUNCTION(BlueprintCallable, Category="Components|InstancedStaticMesh")
	void AddInstance(const FTransform& InstanceTransform);

	/** Add an instance to this component. Transform is given in world space. */
	UFUNCTION(BlueprintCallable, Category = "Components|InstancedStaticMesh")
	 void AddInstanceWorldSpace(const FTransform& WorldTransform);

	virtual bool ShouldCreatePhysicsState() const;

	/** Clear all instances being rendered by this component */
	UFUNCTION(BlueprintCallable, Category="Components|InstancedStaticMesh")
	void ClearInstances();

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
	virtual TSharedPtr<FComponentInstanceDataBase> GetComponentInstanceData() const override;
	virtual FName GetComponentInstanceDataType() const override;
	virtual void ApplyComponentInstanceData(TSharedPtr<FComponentInstanceDataBase> ComponentInstanceData) override;
	// End UActorComponent interface 

	// Begin UPrimitiveComponent Interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual void CreatePhysicsState() override;
	virtual void DestroyPhysicsState() override;

	virtual FBoxSphereBounds CalcBounds(const FTransform& BoundTransform) const override;
#if WITH_EDITOR
	virtual void GetStaticLightingInfo(FStaticLightingPrimitiveInfo& OutPrimitiveInfo,const TArray<ULightComponent*>& InRelevantLights,const FLightingBuildOptions& Options) override;
#endif
	virtual void GetLightAndShadowMapMemoryUsage( int32& LightMapMemoryUsage, int32& ShadowMapMemoryUsage ) const override;
	
	virtual bool DoCustomNavigableGeometryExport(struct FNavigableGeometryExport* GeomExport) const override;
	// End UPrimitiveComponent Interface

	//Begin UObject Interface
	virtual void Serialize(FArchive& Ar) override;
	virtual SIZE_T GetResourceSize(EResourceSizeMode::Type Mode) override;
#if WITH_EDITOR
	virtual void PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent) override;
#endif
	//End UObject Interface

	/** Check to see if an instance is selected */
	bool IsInstanceSelected(int32 InInstanceIndex) const;

	/** Select/deselect an instance or group of instances */
	void SelectInstance(bool bInSelected, int32 InInstanceIndex, int32 InInstanceCount = 1);

private:
	/** Initializes the body instance for the specified instance of the static mesh*/
	void InitInstanceBody(int32 InstanceIdx, FBodyInstance* BodyInstance);

	/** Terminate all body instances owned by this component */
	void ClearAllInstanceBodies();

	/** Sets up new instance data to sensible defaults, creates physics counterparts if possible */
	void SetupNewInstanceData(FInstancedStaticMeshInstanceData& InOutNewInstanceData, int32 InInstanceIndex, const FTransform& InInstanceTransform);

};

