// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MeshPaintSkeletalMeshAdapter.h"
#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "MeshPaintHelpers.h"
#include "MeshPaintTypes.h"

#include "ComponentReregisterContext.h"

//////////////////////////////////////////////////////////////////////////
// FMeshPaintGeometryAdapterForSkeletalMeshes

FMeshPaintGeometryAdapterForSkeletalMeshes::FMeshToComponentMap FMeshPaintGeometryAdapterForSkeletalMeshes::MeshToComponentMap;

bool FMeshPaintGeometryAdapterForSkeletalMeshes::Construct(UMeshComponent* InComponent, int32 InMeshLODIndex)
{
	SkeletalMeshComponent = Cast<USkeletalMeshComponent>(InComponent);
	if (SkeletalMeshComponent != nullptr)
	{
		SkeletalMeshChangedHandle = SkeletalMeshComponent->RegisterOnSkeletalMeshPropertyChanged(USkeletalMeshComponent::FOnSkeletalMeshPropertyChanged::CreateRaw(this, &FMeshPaintGeometryAdapterForSkeletalMeshes::OnSkeletalMeshChanged));

		if (SkeletalMeshComponent->SkeletalMesh != nullptr)
		{
			ReferencedSkeletalMesh = SkeletalMeshComponent->SkeletalMesh;
			MeshLODIndex = InMeshLODIndex;
			const bool bSuccess = Initialize();
			return bSuccess;
		}
	}

	return false;
}

FMeshPaintGeometryAdapterForSkeletalMeshes::~FMeshPaintGeometryAdapterForSkeletalMeshes()
{
	if (SkeletalMeshComponent != nullptr)
	{
		SkeletalMeshComponent->UnregisterOnSkeletalMeshPropertyChanged(SkeletalMeshChangedHandle);
	}
}

void FMeshPaintGeometryAdapterForSkeletalMeshes::OnSkeletalMeshChanged()
{
	OnRemoved();

	if (SkeletalMeshComponent->SkeletalMesh != nullptr)
	{
		ReferencedSkeletalMesh = SkeletalMeshComponent->SkeletalMesh;
		Initialize();
		OnAdded();
	}	
}

bool FMeshPaintGeometryAdapterForSkeletalMeshes::Initialize()
{
	check(ReferencedSkeletalMesh == SkeletalMeshComponent->SkeletalMesh);

	bool bInitializationResult = false;

	MeshResource = ReferencedSkeletalMesh->GetImportedResource();
	if (MeshResource != nullptr)
	{
		LODModel = &MeshResource->LODModels[MeshLODIndex];
		bInitializationResult = FBaseMeshPaintGeometryAdapter::Initialize();
	}

	
	return bInitializationResult;
}

bool FMeshPaintGeometryAdapterForSkeletalMeshes::InitializeVertexData()
{
	// Retrieve mesh vertex and index data 
	const int32 NumVertices = LODModel->NumVertices;
	MeshVertices.Reset();
	MeshVertices.AddDefaulted(NumVertices);
	for (int32 Index = 0; Index < NumVertices; Index++)
	{
		const FVector& Position = LODModel->VertexBufferGPUSkin.GetVertexPositionSlow(Index);
		MeshVertices[Index] = Position;
	}

	MeshIndices.Reserve(LODModel->MultiSizeIndexContainer.GetIndexBuffer()->Num());
	LODModel->MultiSizeIndexContainer.GetIndexBuffer(MeshIndices);

	return (MeshVertices.Num() >= 0 && MeshIndices.Num() > 0);
}

void FMeshPaintGeometryAdapterForSkeletalMeshes::InitializeAdapterGlobals()
{
	static bool bInitialized = false;
	if (!bInitialized)
	{
		bInitialized = true;
		MeshToComponentMap.Empty();
	}
}

void FMeshPaintGeometryAdapterForSkeletalMeshes::OnAdded()
{
	checkf(SkeletalMeshComponent, TEXT("Invalid SkeletalMesh Component"));
	checkf(ReferencedSkeletalMesh, TEXT("Invalid reference to Skeletal Mesh"));
	checkf(ReferencedSkeletalMesh == SkeletalMeshComponent->SkeletalMesh, TEXT("Referenced Skeletal Mesh does not match one in Component"));

	FSkeletalMeshReferencers& SkeletalMeshReferencers = MeshToComponentMap.FindOrAdd(ReferencedSkeletalMesh);

	checkf(!SkeletalMeshReferencers.Referencers.ContainsByPredicate(
		[=](const FSkeletalMeshReferencers::FReferencersInfo& Info)
		{
			return Info.SkeletalMeshComponent == this->SkeletalMeshComponent;
		}), TEXT("This Skeletal Mesh Component has already been Added"));

	// If this is the first attempt to add a temporary body setup to the mesh, do it
	if (SkeletalMeshReferencers.Referencers.Num() == 0)
	{
		// Remember the old body setup (this will be added as a GC reference so that it doesn't get destroyed)
		SkeletalMeshReferencers.RestoreBodySetup = ReferencedSkeletalMesh->BodySetup;

		if (SkeletalMeshReferencers.RestoreBodySetup)
		{
			// Create a new body setup from the mesh's main body setup. This has to have the skeletal mesh as its outer,
			// otherwise the body instance will not be created correctly.
			UBodySetup* TempBodySetupRaw = DuplicateObject<UBodySetup>(ReferencedSkeletalMesh->BodySetup, ReferencedSkeletalMesh);
			TempBodySetupRaw->ClearFlags(RF_Transactional);

			// Set collide all flag so that the body creates physics meshes using ALL elements from the mesh not just the collision mesh.
			TempBodySetupRaw->bMeshCollideAll = true;

			// This forces it to recreate the physics mesh.
			TempBodySetupRaw->InvalidatePhysicsData();

			// Force it to use high detail tri-mesh for collisions.
			TempBodySetupRaw->CollisionTraceFlag = CTF_UseComplexAsSimple;
			TempBodySetupRaw->AggGeom.ConvexElems.Empty();

			// Set as new body setup
			ReferencedSkeletalMesh->BodySetup = TempBodySetupRaw;
		}
	}

	SkeletalMeshComponent->bUseRefPoseOnInitAnim = true;
	SkeletalMeshComponent->InitAnim(true);
	ECollisionEnabled::Type CachedCollisionType = SkeletalMeshComponent->BodyInstance.GetCollisionEnabled();
	SkeletalMeshReferencers.Referencers.Emplace(SkeletalMeshComponent, CachedCollisionType);

	// Force the collision type to not be 'NoCollision' without it the line trace will always fail. 
	if (CachedCollisionType == ECollisionEnabled::NoCollision)
	{
		SkeletalMeshComponent->BodyInstance.SetCollisionEnabled(ECollisionEnabled::QueryOnly, false);
	}

	// Set new physics state for the component
	SkeletalMeshComponent->RecreatePhysicsState();
}

void FMeshPaintGeometryAdapterForSkeletalMeshes::OnRemoved()
{
	checkf(SkeletalMeshComponent, TEXT("Invalid SkeletalMesh Component"));
	
	// If the referenced skeletal mesh has been destroyed (and nulled by GC), don't try to do anything more.
	// It should be in the process of removing all global geometry adapters if it gets here in this situation.
	if (!ReferencedSkeletalMesh)
	{
		return;
	}

	// Remove a reference from the skeletal mesh map
	FSkeletalMeshReferencers* SkeletalMeshReferencers = MeshToComponentMap.Find(ReferencedSkeletalMesh);
	checkf(SkeletalMeshReferencers, TEXT("Could not find Reference to Skeletal Mesh"));
	checkf(SkeletalMeshReferencers->Referencers.Num() > 0, TEXT("Skeletal Mesh does not have any referencers"));

	const int32 Index = SkeletalMeshReferencers->Referencers.IndexOfByPredicate(
		[=](const FSkeletalMeshReferencers::FReferencersInfo& Info)
		{
			return Info.SkeletalMeshComponent == this->SkeletalMeshComponent;
		}
	);
	check(Index != INDEX_NONE);

	SkeletalMeshComponent->bUseRefPoseOnInitAnim = false;
	SkeletalMeshComponent->InitAnim(true);
	SkeletalMeshComponent->BodyInstance.SetCollisionEnabled(SkeletalMeshReferencers->Referencers[Index].CachedCollisionType, false);
	SkeletalMeshComponent->RecreatePhysicsState();

	SkeletalMeshReferencers->Referencers.RemoveAtSwap(Index);

	// If the last reference was removed, restore the body setup for the static mesh
	if (SkeletalMeshReferencers->Referencers.Num() == 0)
	{
		if (SkeletalMeshReferencers->RestoreBodySetup != nullptr)
		{
			ReferencedSkeletalMesh->BodySetup = SkeletalMeshReferencers->RestoreBodySetup;
		}
		
		verify(MeshToComponentMap.Remove(ReferencedSkeletalMesh) == 1);
	}
}

bool FMeshPaintGeometryAdapterForSkeletalMeshes::LineTraceComponent(struct FHitResult& OutHit, const FVector Start, const FVector End, const struct FCollisionQueryParams& Params) const
{
	return SkeletalMeshComponent->LineTraceComponent(OutHit, Start, End, Params);
}

void FMeshPaintGeometryAdapterForSkeletalMeshes::QueryPaintableTextures(int32 MaterialIndex, int32& OutDefaultIndex, TArray<struct FPaintableTexture>& InOutTextureList)
{
	DefaultQueryPaintableTextures(MaterialIndex, SkeletalMeshComponent, OutDefaultIndex, InOutTextureList);
}

void FMeshPaintGeometryAdapterForSkeletalMeshes::ApplyOrRemoveTextureOverride(UTexture* SourceTexture, UTexture* OverrideTexture) const
{
	DefaultApplyOrRemoveTextureOverride(SkeletalMeshComponent, SourceTexture, OverrideTexture);
}

void FMeshPaintGeometryAdapterForSkeletalMeshes::AddReferencedObjects(FReferenceCollector& Collector)
{
	if (!ReferencedSkeletalMesh)
	{
		return;
	}

	FSkeletalMeshReferencers* SkeletalMeshReferencers = MeshToComponentMap.Find(ReferencedSkeletalMesh);
	checkf(SkeletalMeshReferencers, TEXT("No references found for Skeletal Mesh"));
	if (SkeletalMeshReferencers->RestoreBodySetup != nullptr)
	{
		Collector.AddReferencedObject(SkeletalMeshReferencers->RestoreBodySetup);
	}
	

	for (auto& Info : SkeletalMeshReferencers->Referencers)
	{
		Collector.AddReferencedObject(Info.SkeletalMeshComponent);
	}
}

void FMeshPaintGeometryAdapterForSkeletalMeshes::GetTextureCoordinate(int32 VertexIndex, int32 ChannelIndex, FVector2D& OutTextureCoordinate) const
{
	OutTextureCoordinate = LODModel->VertexBufferGPUSkin.GetVertexUVFast(VertexIndex, ChannelIndex);
}

void FMeshPaintGeometryAdapterForSkeletalMeshes::PreEdit()
{
	FlushRenderingCommands();

	SkeletalMeshComponent->Modify();

	ReferencedSkeletalMesh->SetFlags(RF_Transactional);
	ReferencedSkeletalMesh->Modify();

	ReferencedSkeletalMesh->bHasVertexColors = true;

	// Release the static mesh's resources.
	ReferencedSkeletalMesh->ReleaseResources();

	// Flush the resource release commands to the rendering thread to ensure that the build doesn't occur while a resource is still
	// allocated, and potentially accessing the UStaticMesh.
	ReferencedSkeletalMesh->ReleaseResourcesFence.Wait();

	if (LODModel->ColorVertexBuffer.GetNumVertices() == 0)
	{
		// Mesh doesn't have a color vertex buffer yet!  We'll create one now.
		LODModel->ColorVertexBuffer.InitFromSingleColor(FColor(255, 255, 255, 255), LODModel->NumVertices);
		ReferencedSkeletalMesh->bHasVertexColors = true;
		BeginInitResource(&LODModel->ColorVertexBuffer);
	}
}

void FMeshPaintGeometryAdapterForSkeletalMeshes::PostEdit()
{
	TUniquePtr< FSkeletalMeshComponentRecreateRenderStateContext > RecreateRenderStateContext = MakeUnique<FSkeletalMeshComponentRecreateRenderStateContext>(ReferencedSkeletalMesh);
	ReferencedSkeletalMesh->InitResources();
}

void FMeshPaintGeometryAdapterForSkeletalMeshes::GetVertexColor(int32 VertexIndex, FColor& OutColor, bool bInstance /*= true*/) const
{
	if (LODModel->ColorVertexBuffer.GetNumVertices() > 0)
	{
		check((int32)LODModel->ColorVertexBuffer.GetNumVertices() > VertexIndex);
		OutColor = LODModel->ColorVertexBuffer.VertexColor(VertexIndex);
	}
}

void FMeshPaintGeometryAdapterForSkeletalMeshes::SetVertexColor(int32 VertexIndex, FColor Color, bool bInstance /*= true*/)
{
	if (LODModel->ColorVertexBuffer.GetNumVertices() > 0)
	{
		LODModel->ColorVertexBuffer.VertexColor(VertexIndex) = Color;

		if (ReferencedSkeletalMesh->LODInfo[MeshLODIndex].bHasPerLODVertexColors)
		{
			ReferencedSkeletalMesh->LODInfo[MeshLODIndex].bHasPerLODVertexColors = true;
		}
	}
}

FMatrix FMeshPaintGeometryAdapterForSkeletalMeshes::GetComponentToWorldMatrix() const
{
	return SkeletalMeshComponent->GetComponentToWorld().ToMatrixWithScale();
}

//////////////////////////////////////////////////////////////////////////
// FMeshPaintGeometryAdapterForSkeletalMeshesFactory

TSharedPtr<IMeshPaintGeometryAdapter> FMeshPaintGeometryAdapterForSkeletalMeshesFactory::Construct(class UMeshComponent* InComponent, int32 InMeshLODIndex) const
{
	if (USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(InComponent))
	{
		if (SkeletalMeshComponent->SkeletalMesh != nullptr)
		{
			TSharedRef<FMeshPaintGeometryAdapterForSkeletalMeshes> Result = MakeShareable(new FMeshPaintGeometryAdapterForSkeletalMeshes());
			if (Result->Construct(InComponent, InMeshLODIndex))
			{
				return Result;
			}
		}
	}

	return nullptr;
}
