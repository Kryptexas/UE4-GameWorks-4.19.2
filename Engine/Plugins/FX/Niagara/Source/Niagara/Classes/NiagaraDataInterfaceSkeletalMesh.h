// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "NiagaraDataInterfaceMeshCommon.h"
#include "WeightedRandomSampler.h"
#include "Engine/SkeletalMesh.h"
#include "SkeletalMeshRenderData.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraDataInterfaceSkeletalMesh.generated.h"

class UNiagaraDataInterfaceSkeletalMesh;
class USkeletalMesh;
struct FSkeletalMeshSkinningData;
struct FNDISkeletalMesh_InstanceData;
class FSkinWeightVertexBuffer;
struct FSkeletalMeshSamplingRegion;
struct FSkeletalMeshSamplingRegionLODBuiltData;
struct FSkeletalMeshAccessorHelper;

//////////////////////////////////////////////////////////////////////////

struct FSkeletalMeshSkinningDataUsage
{
	FSkeletalMeshSkinningDataUsage()
		: LODIndex(INDEX_NONE)
		, bUsesBoneMatrices(false)
		, bUsesPreSkinnedVerts(false)
		, bNeedDataImmediately(false)
	{}

	FSkeletalMeshSkinningDataUsage(int32 InLODIndex, bool bInUsesBoneMatrices, bool bInUsesPreSkinnedVerts, bool bInNeedDataImmediately)
		: LODIndex(InLODIndex)
		, bUsesBoneMatrices(bInUsesBoneMatrices)
		, bUsesPreSkinnedVerts(bInUsesPreSkinnedVerts)
		, bNeedDataImmediately(bInNeedDataImmediately)
	{}

	FORCEINLINE bool NeedBoneMatrices()const { return bUsesBoneMatrices || bUsesPreSkinnedVerts; }
	FORCEINLINE bool NeedPreSkinnedVerts()const { return bUsesPreSkinnedVerts; }
	FORCEINLINE bool NeedsDataImmediately()const { return bNeedDataImmediately; }
	FORCEINLINE int32 GetLODIndex()const { return LODIndex; }
private:
	int32 LODIndex;
	uint32 bUsesBoneMatrices : 1;
	uint32 bUsesPreSkinnedVerts : 1;
	/** Some users need valid data immediately after the register call rather than being able to wait until the next tick. */
	uint32 bNeedDataImmediately : 1;
};

struct FSkeletalMeshSkinningDataHandle
{
	FSkeletalMeshSkinningDataHandle();
	FSkeletalMeshSkinningDataHandle(FSkeletalMeshSkinningDataUsage InUsage, TSharedPtr<struct FSkeletalMeshSkinningData> InSkinningData);

	FSkeletalMeshSkinningDataHandle& operator=(FSkeletalMeshSkinningDataHandle&& Other)
	{
		if (this != &Other)
		{
			Usage = Other.Usage;
			SkinningData = Other.SkinningData;
			Other.SkinningData = nullptr;
		}
		return *this;
	}

	FSkeletalMeshSkinningDataHandle(FSkeletalMeshSkinningDataHandle&& Other)
	{
		Usage = Other.Usage;
		SkinningData = Other.SkinningData;
	}

	~FSkeletalMeshSkinningDataHandle();

	FSkeletalMeshSkinningDataUsage Usage;
	TSharedPtr<FSkeletalMeshSkinningData> SkinningData;

private:

	FSkeletalMeshSkinningDataHandle& operator=(FSkeletalMeshSkinningDataHandle& Other)
	{
		return *this;
	}

	FSkeletalMeshSkinningDataHandle(FSkeletalMeshSkinningDataHandle& Other)
	{
	}
};

struct FSkeletalMeshSkinningData
{
	FSkeletalMeshSkinningData(TWeakObjectPtr<USkeletalMeshComponent> InMeshComp)
		: MeshComp(InMeshComp)
		, DeltaSeconds(.0333f)
		, CurrIndex(0)
		, BoneMatrixUsers(0)
	{}

	void RegisterUser(FSkeletalMeshSkinningDataUsage Usage);
	void UnregisterUser(FSkeletalMeshSkinningDataUsage Usage);
	bool IsUsed()const;

	bool Tick(float InDeltaSeconds);

	FORCEINLINE FVector GetPosition(int32 LODIndex, int32 VertexIndex)const
	{
		return LODData[LODIndex].SkinnedCPUPositions[CurrIndex][VertexIndex];
	}

	FORCEINLINE FVector GetPreviousPosition(int32 LODIndex, int32 VertexIndex)const
	{
		return LODData[LODIndex].SkinnedCPUPositions[CurrIndex ^ 1][VertexIndex];
	}

	FORCEINLINE TArray<FVector>& CurrSkinnedPositions(int32 LODIndex)
	{
		return LODData[LODIndex].SkinnedCPUPositions[CurrIndex];
	}

	FORCEINLINE TArray<FVector>& PrevSkinnedPositions(int32 LODIndex)
	{
		return LODData[LODIndex].SkinnedCPUPositions[CurrIndex ^ 1];
	}

	FORCEINLINE TArray<FMatrix>& CurrBoneRefToLocals()
	{
		return BoneRefToLocals[CurrIndex];
	}

	FORCEINLINE TArray<FMatrix>& PrevBoneRefToLocals()
	{
		return BoneRefToLocals[CurrIndex ^ 1];
	}

private:

	FCriticalSection CriticalSection; 
	
	TWeakObjectPtr<USkeletalMeshComponent> MeshComp;

	/** Delta seconds between calculations of the previous and current skinned positions. */
	float DeltaSeconds;

	/** Index of the current frames skinned positions and bone matrices. */
	int32 CurrIndex;

	/** Number of users for cached bone matrices. */
	volatile int32 BoneMatrixUsers;

	/** Cached bone matrices. */
	TArray<FMatrix> BoneRefToLocals[2];

	struct FLODData
	{
		FLODData() : PreSkinnedVertsUsers(0) { }

		/** Number of users for pre skinned verts. */
		volatile int32 PreSkinnedVertsUsers;

		/** CPU Skinned vertex positions. Double buffered to allow accurate velocity calculation. */
		TArray<FVector> SkinnedCPUPositions[2];
	};
	TArray<FLODData> LODData;
};

class FNDI_SkeletalMesh_GeneratedData
{
	TMap<TWeakObjectPtr<USkeletalMeshComponent>, TSharedPtr<FSkeletalMeshSkinningData>> CachedSkinningData;
public:

	FSkeletalMeshSkinningDataHandle GetCachedSkinningData(TWeakObjectPtr<USkeletalMeshComponent>& InComponent, FSkeletalMeshSkinningDataUsage Usage);

	void TickGeneratedData(float DeltaSeconds);

	FCriticalSection CriticalSection;
};

//////////////////////////////////////////////////////////////////////////

UENUM()
enum class ENDISkeletalMesh_SkinningMode : uint8
{
	/** No skinning. */
	None,
	/** Skin vertex locations as you need them. Use if you have a high poly mesh or you are sampling the interface a small number of times. */
	SkinOnTheFly,
	/**
	Pre skins the whole mesh. Makes access to location data on the mesh much faster but incurs a significant initial cost in cpu time and memory to skin the mesh.
	Cost is proportional to vertex count in the mesh.
	Use if you are sampling skinned data from the mesh many times and are able to provide a low poly LOD to sample from.
	*/
	PreSkin,
};

enum class ENDISkeletalMesh_FilterMode : uint8
{
	/** No filtering, use all triangles. */
	None,
	/** Filtered to a single region. */
	SingleRegion,
	/** Filtered to multiple regions. */
	MultiRegion,
};

enum class ENDISkelMesh_AreaWeightingMode : uint8
{
	None,
	AreaWeighted,
};

/** Allows perfect area weighted sampling between different skeletal mesh Sampling regions. */
struct FSkeletalMeshSamplingRegionAreaWeightedSampler : FWeightedRandomSampler
{
	FSkeletalMeshSamplingRegionAreaWeightedSampler();
	void Init(FNDISkeletalMesh_InstanceData* InOwner);
	virtual float GetWeights(TArray<float>& OutWeights)override;

	FORCEINLINE bool IsValid() { return TotalWeight > 0.0f; }
protected:
	FNDISkeletalMesh_InstanceData* Owner;
};

struct FNDISkeletalMesh_InstanceData
{
	FRandomStream RandStream;//Might remove this when we rework randoms. I don't like stateful randoms as they can't work on GPU and likely not threaded VM either!

	//Cached ptr to component we sample from. 
	TWeakObjectPtr<USceneComponent> Component;

	USkeletalMesh* Mesh;

	/** Handle to our skinning data. */
	FSkeletalMeshSkinningDataHandle SkinningData;
	
	/** Indices of all valid Sampling regions on the mesh to sample from. */
	TArray<int32> SamplingRegionIndices;
			
	/** Additional sampler for if we need to do area weighting sampling across multiple area weighted regions. */
	FSkeletalMeshSamplingRegionAreaWeightedSampler SamplingRegionAreaWeightedSampler;

	//Cached ComponentToWorld.
	FMatrix Transform;
	//InverseTranspose of above for transforming normals/tangents.
	FMatrix TransformInverseTransposed;

	//Cached ComponentToWorld from previous tick.
	FMatrix PrevTransform;
	//InverseTranspose of above for transforming normals/tangents.
	FMatrix PrevTransformInverseTransposed;

	/** Time separating Transform and PrevTransform. */
	float DeltaSeconds;

	FORCEINLINE_DEBUGGABLE bool ResetRequired(UNiagaraDataInterfaceSkeletalMesh* Interface)const;

	FORCEINLINE_DEBUGGABLE bool Init(UNiagaraDataInterfaceSkeletalMesh* Interface, FNiagaraSystemInstance* SystemInstance);
	FORCEINLINE_DEBUGGABLE bool Cleanup(UNiagaraDataInterfaceSkeletalMesh* Interface, FNiagaraSystemInstance* SystemInstance);
	FORCEINLINE_DEBUGGABLE bool Tick(UNiagaraDataInterfaceSkeletalMesh* Interface, FNiagaraSystemInstance* SystemInstance, float InDeltaSeconds);
	FORCEINLINE int32 GetLODIndex()const { return SkinningData.Usage.GetLODIndex(); }

	FORCEINLINE_DEBUGGABLE FSkeletalMeshLODRenderData& GetLODRenderDataAndSkinWeights(FSkinWeightVertexBuffer*& OutSkinWeightBuffer)
	{
		FSkeletalMeshLODRenderData& Ret = Mesh->GetResourceForRendering()->LODRenderData[GetLODIndex()];
		if (USkeletalMeshComponent* SkelComp = Cast<USkeletalMeshComponent>(Component.Get()))
		{
			OutSkinWeightBuffer = SkelComp->GetSkinWeightBuffer(GetLODIndex());
		}
		else
		{
			OutSkinWeightBuffer = &Ret.SkinWeightVertexBuffer;
		}
		return Ret;
	}
};

/** Data Interface allowing sampling of static meshes. */
UCLASS(EditInlineNew, Category = "Meshes", meta = (DisplayName = "Skeletal Mesh"))
class NIAGARA_API UNiagaraDataInterfaceSkeletalMesh : public UNiagaraDataInterface
{
	GENERATED_UCLASS_BODY()

public:
	
	/** Mesh used to sample from when not overridden by a source actor from the scene. Also useful for previewing in the editor. */
	UPROPERTY(EditAnywhere, Category = "Mesh")
	USkeletalMesh* DefaultMesh;

	/** The source actor from which to sample. Takes precedence over the direct mesh. */
	UPROPERTY(EditAnywhere, Category = "Mesh")
	AActor* Source;
	
	UPROPERTY(EditAnywhere, Category="Mesh")
	ENDISkeletalMesh_SkinningMode SkinningMode;

	/** Sampling regions on the mesh from which to sample. Leave this empty to sample from the whole mesh. */
	UPROPERTY(EditAnywhere, Category="Mesh")
	TArray<FName> SamplingRegions;

	/** If no regions are specified, we'll sample the whole mesh at this LODIndex. -1 indicates to use the last LOD.*/
	UPROPERTY(EditAnywhere, Category="Mesh")
	int32 WholeMeshLOD;

public:

	//~ UObject interface
#if WITH_EDITOR
	virtual void PostInitProperties()override;
#endif
	//~ UObject interface END

public:

	//~ UNiagaraDataInterface interface
	virtual bool InitPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance)override;
	virtual void DestroyPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance)override;
	virtual bool PerInstanceTick(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance, float DeltaSeconds) override;
	virtual int32 PerInstanceDataSize()const override { return sizeof(FNDISkeletalMesh_InstanceData); }

	virtual void GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)override;
	virtual FVMExternalFunction GetVMExternalFunction(const FVMExternalFunctionBindingInfo& BindingInfo, void* InstanceData)override;
	virtual bool Equals(const UNiagaraDataInterface* Other) const override;
	virtual bool CanExecuteOnTarget(ENiagaraSimTarget Target)const override { return Target == ENiagaraSimTarget::CPUSim; }
	//~ UNiagaraDataInterface interface END
public:

	template<typename FilterMode, typename AreaWeightingMode>
	void RandomTriCoord(FVectorVMContext& Context);

	template<typename SkinningHandlerType, typename TransformHandlerType, typename TriType, typename BaryXType, typename BaryYType, typename BaryZType>
	void GetTriCoordPosition(FVectorVMContext& Context);

	template<typename SkinningHandlerType, typename TransformHandlerType, typename TriType, typename BaryXType, typename BaryYType, typename BaryZType>
	void GetTriCoordPositionVelocityAndNormal(FVectorVMContext& Context);
	
	template<typename TriType, typename BaryXType, typename BaryYType, typename BaryZType>
	void GetTriCoordColor(FVectorVMContext& Context);

	template<typename VertexAccessorType, typename TriType, typename BaryXType, typename BaryYType, typename BaryZType, typename UVSetType>
	void GetTriCoordUV(FVectorVMContext& Context);
 
protected:
	virtual bool CopyToInternal(UNiagaraDataInterface* Destination) const override;

private:

	template<typename FilterMode, typename AreaWeightingMode>
	FORCEINLINE_DEBUGGABLE int32 RandomTriIndex(FSkeletalMeshAccessorHelper& Accessor, FNDISkeletalMesh_InstanceData* InstData);
};
