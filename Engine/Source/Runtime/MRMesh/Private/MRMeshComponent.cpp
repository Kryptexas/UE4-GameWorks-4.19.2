// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MRMeshComponent.h"
#include "PrimitiveSceneProxy.h"
#include "DynamicMeshBuilder.h"
#include "LocalVertexFactory.h"
#include "Containers/ResourceArray.h"
#include "SceneManagement.h"
#include "MaterialShared.h"
#include "Materials/Material.h"
#include "RenderingThread.h"
#include "BaseMeshReconstructorModule.h"



class FMRMeshVertexResourceArray : public FResourceArrayInterface
{
public:
	FMRMeshVertexResourceArray(void* InData, uint32 InSize)
		: Data(InData)
		, Size(InSize)
	{
	}

	virtual const void* GetResourceData() const override { return Data; }
	virtual uint32 GetResourceDataSize() const override { return Size; }
	virtual void Discard() override { }
	virtual bool IsStatic() const override { return false; }
	virtual bool GetAllowCPUAccess() const override { return false; }
	virtual void SetAllowCPUAccess(bool bInNeedsCPUAccess) override { }

private:
	void* Data;
	uint32 Size;
};


class FMRMeshVertexBuffer : public FVertexBuffer
{
public:
	int32 NumVerts = 0;
	void InitRHIWith( TArray<FDynamicMeshVertex>& Vertices )
	{
		NumVerts = Vertices.Num();

		const uint32 SizeInBytes = Vertices.Num() * sizeof(FDynamicMeshVertex);

		FMRMeshVertexResourceArray ResourceArray(Vertices.GetData(), SizeInBytes);
		FRHIResourceCreateInfo CreateInfo(&ResourceArray);
		VertexBufferRHI = RHICreateVertexBuffer(SizeInBytes, BUF_Static, CreateInfo);
	}

};

class FMRMeshIndexBuffer : public FIndexBuffer
{
public:
	int32 NumIndices = 0;
	void InitRHIWith( const TArray<uint32>& Indices )
	{
		NumIndices = Indices.Num();

		FRHIResourceCreateInfo CreateInfo;
		void* Buffer = nullptr;
		IndexBufferRHI = RHICreateAndLockIndexBuffer(sizeof(int32), Indices.Num() * sizeof(int32), BUF_Static, CreateInfo, Buffer);

		// Write the indices to the index buffer.		
		FMemory::Memcpy(Buffer, Indices.GetData(), Indices.Num() * sizeof(int32));
		RHIUnlockIndexBuffer(IndexBufferRHI);
	}
};

class FMRMeshVertexFactory : public FLocalVertexFactory
{
public:

	FMRMeshVertexFactory()
	{}

	/** Init function that should only be called on render thread. */
	void Init_RenderThread(const FMRMeshVertexBuffer* VertexBuffer)
	{
		check(IsInRenderingThread());

		// Initialize the vertex factory's stream components.
		FDataType NewData;
		NewData.PositionComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FDynamicMeshVertex, Position, VET_Float3);
		NewData.TextureCoordinates.Add(
			FVertexStreamComponent(VertexBuffer, STRUCT_OFFSET(FDynamicMeshVertex, TextureCoordinate), sizeof(FDynamicMeshVertex), VET_Float2)
		);
		NewData.TangentBasisComponents[0] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FDynamicMeshVertex, TangentX, VET_PackedNormal);
		NewData.TangentBasisComponents[1] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FDynamicMeshVertex, TangentZ, VET_PackedNormal);
		NewData.ColorComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FDynamicMeshVertex, Color, VET_Color);
		SetData(NewData);
	}

	/** Init function that can be called on any thread, and will do the right thing (enqueue command if called on main thread) */
	void Init(const FMRMeshVertexBuffer* VertexBuffer)
	{
		if (IsInRenderingThread())
		{
			Init_RenderThread(VertexBuffer);
		}
		else
		{
			ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			InitProcMeshVertexFactory,
			FMRMeshVertexFactory*, VertexFactory, this,
			const FMRMeshVertexBuffer*, VertexBuffer, VertexBuffer,
			{
				VertexFactory->Init_RenderThread(VertexBuffer);
			});
		}
	}
};


struct FMRMeshProxySection
{
	/** Which brick this section represents */
	FIntVector BrickId;
	/** Vertex buffer for this section */
	FMRMeshVertexBuffer VertexBuffer;
	/** Index buffer for this section */
	FMRMeshIndexBuffer IndexBuffer;
	/** Vertex factory for this section */
	FMRMeshVertexFactory VertexFactory;

	FMRMeshProxySection(FIntVector InBrickId)
	: BrickId(InBrickId)
	{
	}

	void ReleaseResources()
	{
		VertexBuffer.ReleaseResource();
		IndexBuffer.ReleaseResource();
		VertexFactory.ReleaseResource();
	}

	FMRMeshProxySection(const FMRMeshVertexFactory&) = delete;
	void operator==(const FMRMeshVertexFactory&) = delete;
};

class FMRMeshProxy : public FPrimitiveSceneProxy
{
public:
	FMRMeshProxy(const UMRMeshComponent* InComponent)
	: FPrimitiveSceneProxy(InComponent)
	, MaterialToUse((InComponent->Material!=nullptr) ? InComponent->Material : UMaterial::GetDefaultMaterial(MD_Surface) )
	{
	}

	virtual ~FMRMeshProxy()
	{
		for (FMRMeshProxySection* Section : ProxySections)
		{
			if (Section != nullptr)
			{
				Section->ReleaseResources();
				delete Section;
			}
		}
	}

	void RenderThread_UploadNewSection(IMRMesh::FSendBrickDataArgs Args)
	{
		check(IsInRenderingThread() || IsInRHIThread());

		FMRMeshProxySection* NewSection = new FMRMeshProxySection(Args.BrickCoords);
		ProxySections.Add(NewSection);

		// VERTEX BUFFER
		{
			NewSection->VertexBuffer.InitResource();
			NewSection->VertexBuffer.InitRHIWith(Args.Vertices);
		}

		// INDEX BUFFER
		{
			NewSection->IndexBuffer.InitResource();
			NewSection->IndexBuffer.InitRHIWith(Args.Indices);
		}

		// VERTEX FACTORY
		{
			NewSection->VertexFactory.Init(&NewSection->VertexBuffer);
			NewSection->VertexFactory.InitResource();
		}
	}

	bool RenderThread_RemoveSection(FIntVector BrickCoords)
	{
		check(IsInRenderingThread() || IsInRHIThread());
		for (int32 i = 0; i < ProxySections.Num(); ++i)
		{
			if (ProxySections[i]->BrickId == BrickCoords)
			{
				ProxySections[i]->ReleaseResources();
				delete ProxySections[i];
				ProxySections.RemoveAtSwap(i);
				return true;
			}
		}
		return false;
	}

private:
	//~ FPrimitiveSceneProxy
	virtual uint32 GetMemoryFootprint(void) const override
	{
		return 0;
	}
	
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, class FMeshElementCollector& Collector) const override
	{
		static const FBoxSphereBounds InfiniteBounds(FSphere(FVector::ZeroVector, HALF_WORLD_MAX));

		// Iterate over sections
		for (const FMRMeshProxySection* Section : ProxySections)
		{
			if (Section != nullptr)
			{
				const bool bIsSelected = false;
				FMaterialRenderProxy* MaterialProxy = MaterialToUse->GetRenderProxy(bIsSelected);

				// For each view..
				for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
				{
					if (VisibilityMap & (1 << ViewIndex))
					{
						const FSceneView* View = Views[ViewIndex];
						// Draw the mesh.
						FMeshBatch& Mesh = Collector.AllocateMesh();
						FMeshBatchElement& BatchElement = Mesh.Elements[0];
						BatchElement.IndexBuffer = &Section->IndexBuffer;
						Mesh.bWireframe = false;
						Mesh.VertexFactory = &Section->VertexFactory;
						Mesh.MaterialRenderProxy = MaterialProxy;
						BatchElement.PrimitiveUniformBuffer = CreatePrimitiveUniformBufferImmediate(GetLocalToWorld(), InfiniteBounds, InfiniteBounds, true, UseEditorDepthTest());
						BatchElement.FirstIndex = 0;
						BatchElement.NumPrimitives = Section->IndexBuffer.NumIndices / 3;
						BatchElement.MinVertexIndex = 0;
						BatchElement.MaxVertexIndex = Section->VertexBuffer.NumVerts - 1;
						Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
						Mesh.Type = PT_TriangleList;
						Mesh.DepthPriorityGroup = SDPG_World;
						Mesh.bCanApplyViewModeOverrides = false;
						Collector.AddMesh(ViewIndex, Mesh);
					}
				}
			}
		}
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View);
		Result.bShadowRelevance = IsShadowCast(View);
		Result.bDynamicRelevance = true;
		Result.bRenderInMainPass = ShouldRenderInMainPass();
		Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
		Result.bRenderCustomDepth = ShouldRenderCustomDepth();
		//MaterialRelevance.SetPrimitiveViewRelevance(Result);
		return Result;
	}
	//~ FPrimitiveSceneProxy

private:
	TArray<FMRMeshProxySection*> ProxySections;
	UMaterialInterface* MaterialToUse;
};


UMRMeshComponent::UMRMeshComponent(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
, MeshReconstructor(nullptr)
{

}

FPrimitiveSceneProxy* UMRMeshComponent::CreateSceneProxy()
{
	// The render thread owns the memory, so if this function is
	// being called, it's safe to just re-allocate.
	
	if ( FBaseMeshReconstructorModule::IsAvailable() )
	{
		MeshReconstructor = &FBaseMeshReconstructorModule::Get();
		MeshReconstructor->PairWithComponent(*this);
	}

	return new FMRMeshProxy(this);
}

void UMRMeshComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials /*= false*/) const
{
	if (Material != nullptr)
	{
		OutMaterials.Add(Material);
	}
}

FBoxSphereBounds UMRMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return FBoxSphereBounds(FSphere(FVector::ZeroVector, HALF_WORLD_MAX));
}

void UMRMeshComponent::SendBrickData(IMRMesh::FSendBrickDataArgs Args, const FOnProcessingComplete& OnProcessingComplete /*= FOnProcessingComplete()*/)
{
	auto BrickDataTask = FSimpleDelegateGraphTask::FDelegate::CreateUObject(this, &UMRMeshComponent::SendBrickData_Internal, Args, OnProcessingComplete);

	DECLARE_CYCLE_STAT(TEXT("UMRMeshComponent.SendBrickData"),
		STAT_UMRMeshComponent_SendBrickData,
		STATGROUP_MRMESH);

	// The render thread might not be around, in which case
	// queue the task into the game thread for later processing.
	FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(BrickDataTask, GET_STATID(STAT_UMRMeshComponent_SendBrickData), nullptr, ENamedThreads::GameThread);

}

void UMRMeshComponent::SendBrickData_Internal(IMRMesh::FSendBrickDataArgs Args, FOnProcessingComplete OnProcessingComplete)
{
	check(IsInGameThread());
	if (!IsPendingKill() && SceneProxy != nullptr)
	{
		check(GRenderingThread != nullptr);
		check(SceneProxy != nullptr);

		ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
			FSendBrickDataLambda,
			UMRMeshComponent*, This, this,
			IMRMesh::FSendBrickDataArgs, Args, Args,
			FOnProcessingComplete, OnProcessingComplete, OnProcessingComplete,
			{
				FMRMeshProxy* MRMeshProxy = static_cast<FMRMeshProxy*>(This->SceneProxy);
				if (MRMeshProxy)
				{
					MRMeshProxy->RenderThread_RemoveSection(Args.BrickCoords);
					MRMeshProxy->RenderThread_UploadNewSection(Args);

					if (OnProcessingComplete.IsBound())
					{
						OnProcessingComplete.Execute();
					}
				}
			});
	}
}

void UMRMeshComponent::BeginDestroy()
{
	if (MeshReconstructor != nullptr)
	{
		MeshReconstructor->Stop();
		MeshReconstructor = nullptr;
	}

	Super::BeginDestroy();
	
}

