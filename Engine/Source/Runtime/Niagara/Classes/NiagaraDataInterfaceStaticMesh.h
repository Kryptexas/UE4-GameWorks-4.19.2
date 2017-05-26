// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "NiagaraDataInterface.h"
#include "NiagaraCommon.h"
#include "VectorVM.h"
#include "StaticMeshResources.h"
#include "Engine/StaticMesh.h"
#include "NiagaraDataInterfaceStaticMesh.generated.h"

//A coordinate on a mesh usable in Niagara.
//Do not alter this struct without updating the data interfaces that use it!
USTRUCT()
struct FMeshTriCoordinate
{
	GENERATED_USTRUCT_BODY();
	
	UPROPERTY(EditAnywhere, Category="Coordinate")
	int32 Tri;

	UPROPERTY(EditAnywhere, Category="Coordinate")
	FVector BaryCoord;
};


struct FNDIStaticMeshSectionFilter;

/** Allows uniform random sampling of a number of mesh sections filtered by an FNDIStaticMeshSectionFilter */
struct FStaticMeshFilteredAreaWeightedSectionSampler : FStaticMeshAreaWeightedSectionSampler
{
	FStaticMeshFilteredAreaWeightedSectionSampler();
	void Init(FStaticMeshLODResources* InRes, FNDIStaticMeshSectionFilter* InOwnerFilter);
	virtual float GetWeights(TArray<float>& OutWeights)override;

protected:
	FStaticMeshLODResources* Res;
	FNDIStaticMeshSectionFilter* OwnerFilter;
};

USTRUCT()
struct FNDIStaticMeshSectionFilter
{
	GENERATED_USTRUCT_BODY();

	/** Only allow sections these material slots. */
	UPROPERTY(EditAnywhere, Category="Section Filter")
	TArray<int32> AllowedMaterialSlots;

	//Others?
	//Banned material slots
	
	void Init(class UNiagaraDataInterfaceStaticMesh* Owner, bool bAreaWeighted);
	FORCEINLINE FStaticMeshFilteredAreaWeightedSectionSampler& GetAreaWeigtedSampler() { return Sampler; }
	FORCEINLINE const TArray<int32>& GetValidSections()const { return ValidSections; }

	FORCEINLINE bool CanEverReject()const { return AllowedMaterialSlots.Num() > 0; }
	
private:
	
	/** Cached results of this filter being applied to the owning mesh. */
	TArray<int32> ValidSections;
	/** Area weighted sampler for the valid sections. */
	FStaticMeshFilteredAreaWeightedSectionSampler Sampler;
};

/** Data Interface allowing sampling of static meshes. */
UCLASS(EditInlineNew, Category = "Meshes", meta = (DisplayName = "Static Mesh"))
class NIAGARA_API UNiagaraDataInterfaceStaticMesh : public UNiagaraDataInterface
{
	GENERATED_UCLASS_BODY()

public:
	
	/** Mesh used to sample from when not overridden by a source actor from the scene. Also useful for previewing in the editor. */
	UPROPERTY(EditAnywhere, Category = "Mesh")
	UStaticMesh* DefaultMesh;

	/** The source actor from which to sample. Takes precedence over the direct mesh. */
	UPROPERTY(EditAnywhere, Category = "Mesh")
	AActor* Source;
	
	/** Array of filters the can be used to limit sampling to certain sections of the mesh. */
	UPROPERTY(EditAnywhere, Category = "Mesh")
	FNDIStaticMeshSectionFilter SectionFilter;

	/** Determine whether or not we will support vertex color sorting for random sampling.*/
	UPROPERTY(EditAnywhere, Category = "Mesh")
	bool bEnableVertexColorRangeSorting;

public:

	//~ UObject interface

#if WITH_EDITOR
	virtual void PostInitProperties()override;
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)override;
#endif

public:

	//~ UNiagaraDataInterface interface

	virtual bool PrepareForSimulation(FNiagaraEffectInstance* EffectInstance)override;
	virtual bool PreSimulationTick(FNiagaraEffectInstance* EffectInstance, float DeltaSeconds) override;
	virtual bool HasPreSimulationTick()const override;
	virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)override;
	virtual FVMExternalFunction GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo)override;
	virtual bool CopyTo(UNiagaraDataInterface* Destination) override;
	virtual bool Equals(UNiagaraDataInterface* Other) override;
	virtual bool RequiresPerInstanceData() const override { return true; }
public:

	template<typename TAreaWeighted>
	void RandomSection(FVectorVMContext& Context);

	template<typename TAreaWeighted>
	void RandomTriCoord(FVectorVMContext& Context);

	template<typename TAreaWeighted, typename SectionIdxType>
	void RandomTriCoordOnSection(FVectorVMContext& Context);

	template<typename TVertexColorFiltering, typename InputType0, typename InputType1>
	void RandomTriCoordVertexColorFiltered(FVectorVMContext& Context);
	
	template<typename TransformHandlerType, typename TriType, typename BaryXType, typename BaryYType, typename BaryZType>
	void GetTriCoordPosition(FVectorVMContext& Context);

	template<typename TransformHandlerType, typename TriType, typename BaryXType, typename BaryYType, typename BaryZType>
	void GetTriCoordNormal(FVectorVMContext& Context);

	template<typename VertexAccessorType, typename TransformHandlerType, typename TriType, typename BaryXType, typename BaryYType, typename BaryZType>
	void GetTriCoordTangents(FVectorVMContext& Context);

	template<typename TriType, typename BaryXType, typename BaryYType, typename BaryZType>
	void GetTriCoordColor(FVectorVMContext& Context);

	template<typename VertexAccessorType, typename TriType, typename BaryXType, typename BaryYType, typename BaryZType, typename UVSetType>
	void GetTriCoordUV(FVectorVMContext& Context);

	template<typename TriType, typename BaryXType, typename BaryYType, typename BaryZType>
	void GetTriCoordPositionAndVelocity(FVectorVMContext& Context);

	void GetLocalToWorld(FVectorVMContext& Context);
	void GetLocalToWorldInverseTransposed(FVectorVMContext& Context);
	void GetWorldVelocity(FVectorVMContext& Context);

	FORCEINLINE UStaticMesh* GetActualMesh()const { return Mesh; }
	FORCEINLINE bool UsesAreaWeighting()const { return bIsAreaWeightedSampling; }
	FORCEINLINE bool UsesSectionFilter()const { return SectionFilter.CanEverReject(); }
	FORCEINLINE bool UsesVertexColorFiltering()const { return bSupportingVertexColorSampling && bEnableVertexColorRangeSorting; }
private:

	FORCEINLINE bool MeshHasPositions()const { return Mesh && Mesh->RenderData->LODResources[0].PositionVertexBuffer.GetNumVertices() > 0; }
	FORCEINLINE bool MeshHasVerts()const { return Mesh && Mesh->RenderData->LODResources[0].VertexBuffer.GetNumVertices() > 0; }
	FORCEINLINE bool MeshHasColors()const { return Mesh && Mesh->RenderData->LODResources[0].ColorVertexBuffer.GetNumVertices() > 0; }

	template<typename TAreaWeighted, bool bFiltered>
	FORCEINLINE int32 RandomSection(FStaticMeshLODResources& Res);

	template<typename TAreaWeighted, bool bFiltered>
	FORCEINLINE int32 RandomTriIndex(FStaticMeshLODResources& Res);

	template<typename TAreaWeighted>
	FORCEINLINE int32 RandomTriIndexOnSection(FStaticMeshLODResources& Res, int32 SectionIdx);

	void WriteTransform(const FMatrix& ToWrite, FVectorVMContext& Context);

	bool ResetRequired();
	void ResetFilter();

	FRandomStream RandStream;//Might remove this when we rework randoms. I don't like stateful randoms as they can't work on GPU and likely not threaded VM either!
	
	//Cached ptr to component we sample from. 
	TWeakObjectPtr<USceneComponent> Component;

	//Cached ptr to actual mesh we sample from. 
	UStaticMesh* Mesh;
	
	//Cached GetComponentTransform().
	FMatrix Transform;
	//InverseTranspose of above for transforming normals/tangents.
	FMatrix TransformInverseTransposed;

	//Cached GetComponentTransform() from previous tick.
	FMatrix PrevTransform;
	//InverseTranspose of above for transforming normals/tangents.
	FMatrix PrevTransformInverseTransposed;

	/** Time separating Transform and PrevTransform. */
	float DeltaSeconds;

	/** True if the mesh we're using allows area weighted sampling. */
	uint32 bIsAreaWeightedSampling : 1;
	/** True if the mesh we're using allows vertex color sampling and we have the .bEnableVertexColorRangeSorting property set to true. */
	uint32 bSupportingVertexColorSampling : 1;

	/** Container for the vertex colored triangles broken out by red channel values*/
	TArray<uint32> TrianglesSortedByVertexColor;
	/** Mapping from vertex color red value to starting entry in TrianglesSortedByVertexColor*/
	TArray<uint32> VertexColorToTriangleStart;

	bool bFilterInitialized;
};
