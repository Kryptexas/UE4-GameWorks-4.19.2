// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	InstancedFoliage.cpp: Instanced foliage implementation.
=============================================================================*/

#include "EnginePrivate.h"
#include "StaticMeshResources.h"
#include "MessageLog.h"
#include "UObjectToken.h"
#include "MapErrors.h"
#include "Foliage/InstancedFoliageActor.h"
#include "Foliage/FoliageType_InstancedStaticMesh.h"
#include "Components/ModelComponent.h"

#define LOCTEXT_NAMESPACE "InstancedFoliage"

#define DO_FOLIAGE_CHECK			0			// whether to validate foliage data during editing.
#define FOLIAGE_CHECK_TRANSFORM		0			// whether to compare transforms between render and painting data.


DEFINE_LOG_CATEGORY_STATIC(LogInstancedFoliage, Log, All);

IMPLEMENT_HIT_PROXY(HInstancedStaticMeshInstance, HHitProxy);

//
// Serializers for struct data
//

FArchive& operator<<(FArchive& Ar, FFoliageInstance& Instance)
{
	Ar << Instance.Base;
	Ar << Instance.Location;
	Ar << Instance.Rotation;
	Ar << Instance.DrawScale3D;
	Ar << Instance.ClusterIndex;
	Ar << Instance.PreAlignRotation;
	Ar << Instance.Flags;
	Ar << Instance.ZOffset;
	return Ar;
}

FArchive& operator<<(FArchive& Ar, FFoliageInstanceCluster& Cluster)
{
	Ar << Cluster.Bounds;
	Ar << Cluster.ClusterComponent;

#if WITH_EDITORONLY_DATA
	if (!Ar.ArIsFilterEditorOnly ||
		Ar.UE4Ver() < VER_UE4_FOLIAGE_SETTINGS_TYPE)
	{
		Ar << Cluster.InstanceIndices;
	}
#endif

	return Ar;
}

FArchive& operator<<(FArchive& Ar, FFoliageMeshInfo& MeshInfo)
{
	Ar << MeshInfo.InstanceClusters;

#if WITH_EDITORONLY_DATA
	if (!Ar.ArIsFilterEditorOnly ||
		Ar.UE4Ver() < VER_UE4_FOLIAGE_SETTINGS_TYPE)
	{
		Ar << MeshInfo.Instances;
	}

	// Serialize the transient data for undo.
	if (Ar.IsTransacting())
	{
		Ar << *MeshInfo.InstanceHash;
		Ar << MeshInfo.ComponentHash;
		Ar << MeshInfo.FreeInstanceIndices;
		Ar << MeshInfo.SelectedIndices;
	}
#endif

	return Ar;
}

FArchive& operator<<(FArchive& Ar, FFoliageComponentHashInfo& ComponentHashInfo)
{
	return Ar << ComponentHashInfo.CachedLocation << ComponentHashInfo.CachedRotation << ComponentHashInfo.CachedDrawScale << ComponentHashInfo.Instances;
}

//
// UFoliageType
//

UFoliageType::UFoliageType(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Density = 100.0f;
	Radius = 0.0f;
	AlignToNormal = true;
	RandomYaw = true;
	UniformScale = true;
	ScaleMinX = 1.0f;
	ScaleMinY = 1.0f;
	ScaleMinZ = 1.0f;
	ScaleMaxX = 1.0f;
	ScaleMaxY = 1.0f;
	ScaleMaxZ = 1.0f;
	AlignMaxAngle = 0.0f;
	RandomPitchAngle = 0.0f;
	GroundSlope = 45.0f;
	HeightMin = -262144.0f;
	HeightMax = 262144.0f;
	ZOffsetMin = 0.0f;
	ZOffsetMax = 0.0f;
	LandscapeLayer = NAME_None;
	MaxInstancesPerCluster = 100;
	MaxClusterRadius = 10000.0f;
	DisplayOrder = 0;
	IsSelected = false;
	ShowNothing = false;
	ShowPaintSettings = true;
	ShowInstanceSettings = false;
	ReapplyDensityAmount = 1.0f;
	CollisionWithWorld = false;
	CollisionScale = FVector(0.9f, 0.9f, 0.9f);
	VertexColorMask = FOLIAGEVERTEXCOLORMASK_Disabled;
	VertexColorMaskThreshold = 0.5f;

	CastShadow = true;
	bCastDynamicShadow = true;
	bCastStaticShadow = true;
	bAffectDynamicIndirectLighting = false;
	bCastShadowAsTwoSided = false;
	bReceivesDecals = false;

	bOverrideLightMapRes = false;
	OverriddenLightMapRes = 32;

	BodyInstance.SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
}

UFoliageType_InstancedStaticMesh::UFoliageType_InstancedStaticMesh(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Mesh = nullptr;
}

#if WITH_EDITOR
void UFoliageType::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Ensure that OverriddenLightMapRes is a factor of 4
	OverriddenLightMapRes = FMath::Max(OverriddenLightMapRes + 3 & ~3, 4);
}
#endif

//
// FFoliageMeshInfo
//
FFoliageMeshInfo::FFoliageMeshInfo()
#if WITH_EDITOR
	: InstanceHash(GIsEditor ? new FFoliageInstanceHash() : nullptr)
#endif
{ }


#if WITH_EDITOR

void FFoliageMeshInfo::CheckValid()
{
#if DO_FOLIAGE_CHECK
	int32 ClusterTotal = 0;
	int32 ComponentTotal = 0;

	for (FFoliageInstanceCluster& Cluster : InstanceClusters)
	{
		check(Cluster.ClusterComponent != nullptr);
		ClusterTotal += Cluster.InstanceIndices.Num();
		ComponentTotal += Cluster.ClusterComponent->PerInstanceSMData.Num();
	}

	check(ClusterTotal == ComponentTotal);

	int32 FreeTotal = 0;
	int32 InstanceTotal = 0;
	for (int32 InstanceIdx = 0; InstanceIdx < Instances.Num(); InstanceIdx++)
	{
		if (Instances[InstanceIdx].ClusterIndex != -1)
		{
			InstanceTotal++;
		}
		else
		{
			FreeTotal++;
		}
	}

	check( ClusterTotal == InstanceTotal );
	check( FreeInstanceIndices.Num() == FreeTotal );

	InstanceHash->CheckInstanceCount(InstanceTotal);

	int32 ComponentHashTotal = 0;
	for( TMap<UActorComponent*, FFoliageComponentHashInfo >::TConstIterator It(ComponentHash); It; ++It )
	{
		ComponentHashTotal += It.Value().Instances.Num();
	}
	check( ComponentHashTotal == InstanceTotal);

#if FOLIAGE_CHECK_TRANSFORM
	// Check transforms match up with editor data
	int32 MismatchCount = 0;
	for( int32 ClusterIdx=0;ClusterIdx<InstanceClusters.Num();ClusterIdx++ )
	{
		TArray<int32> Indices = InstanceClusters(ClusterIdx).InstanceIndices;
		UInstancedStaticMeshComponent* Comp = InstanceClusters(ClusterIdx).ClusterComponent;
		for( int32 InstIdx=0;InstIdx<Indices.Num();InstIdx++ )
		{
			int32 InstanceIdx = Indices(InstIdx);

			FTransform InstanceToWorldEd = Instances(InstanceIdx).GetInstanceTransform();
			FTransform InstanceToWorldCluster = Comp->PerInstanceSMData(InstIdx).Transform * Comp->GetComponentToWorld();

			if( !InstanceToWorldEd.Equals(InstanceToWorldCluster) )
			{
				Comp->PerInstanceSMData(InstIdx).Transform = InstanceToWorldEd.ToMatrixWithScale();
				MismatchCount++;
			}
		}
	}

	if( MismatchCount != 0 )
	{
		UE_LOG(LogInstancedFoliage, Log, TEXT("%s: transform mismatch: %d"), *InstanceClusters(0).ClusterComponent->StaticMesh->GetName(), MismatchCount);
	}
#endif

#endif
}

void FFoliageMeshInfo::AddInstance(AInstancedFoliageActor* InIFA, UFoliageType* InSettings, const FFoliageInstance& InNewInstance)
{
	InIFA->Modify();

	// Add the instance taking either a free slot or adding a new item.
	int32 InstanceIndex = FreeInstanceIndices.Num() > 0 ?  FreeInstanceIndices.Pop() : Instances.AddUninitialized();

	FFoliageInstance& AddedInstance = Instances[InstanceIndex];
	AddedInstance = InNewInstance;

	// Add the instance to the hash
	InstanceHash->InsertInstance(InNewInstance.Location, InstanceIndex);
	FFoliageComponentHashInfo* ComponentHashInfo = ComponentHash.Find(InNewInstance.Base);
	if (ComponentHashInfo == nullptr)
	{
		ComponentHashInfo = &ComponentHash.Add(InNewInstance.Base, FFoliageComponentHashInfo(InNewInstance.Base));
	}
	ComponentHashInfo->Instances.Add(InstanceIndex);

	// Find the best cluster to allocate the instance to.
	FFoliageInstanceCluster* BestCluster = nullptr;
	int32 BestClusterIndex = INDEX_NONE;
	float BestClusterDistSq = FLT_MAX;

	int32 MaxInstancesPerCluster = InSettings->MaxInstancesPerCluster;
	float MaxClusterRadiusSq = FMath::Square(InSettings->MaxClusterRadius);

	for (int32 ClusterIdx = 0; ClusterIdx < InstanceClusters.Num(); ClusterIdx++)
	{
		FFoliageInstanceCluster& Cluster = InstanceClusters[ClusterIdx];
		if (Cluster.InstanceIndices.Num() < MaxInstancesPerCluster)
		{
			float DistSq = (Cluster.Bounds.Origin - InNewInstance.Location).SizeSquared();
			if (DistSq < BestClusterDistSq && DistSq < MaxClusterRadiusSq)
			{
				BestCluster = &Cluster;
				BestClusterIndex = ClusterIdx;
				BestClusterDistSq = DistSq;
			}
		}
	}

	// Calculate transform for the instance
	FTransform InstanceToWorld = InNewInstance.GetInstanceWorldTransform();

	// Bounds of the new instance
	FBoxSphereBounds InstanceWorldBounds = InSettings->GetStaticMesh()->GetBounds().TransformBy(InstanceToWorld);

	if (BestCluster == nullptr)
	{
		BestClusterIndex = InstanceClusters.Num();
		BestCluster = new(InstanceClusters)FFoliageInstanceCluster(
			ConstructObject<UInstancedStaticMeshComponent>(UInstancedStaticMeshComponent::StaticClass(), InIFA, NAME_None, RF_Transactional),
			InstanceWorldBounds
			);

		BestCluster->ClusterComponent->Mobility = EComponentMobility::Static;

		BestCluster->ClusterComponent->StaticMesh = InSettings->GetStaticMesh();
		BestCluster->ClusterComponent->bSelectable = true;
		BestCluster->ClusterComponent->bHasPerInstanceHitProxies = true;
		BestCluster->ClusterComponent->InstancingRandomSeed = FMath::Rand();
		BestCluster->ClusterComponent->InstanceStartCullDistance = InSettings->StartCullDistance;
		BestCluster->ClusterComponent->InstanceEndCullDistance   = InSettings->EndCullDistance;

		BestCluster->ClusterComponent->CastShadow                     = InSettings->CastShadow;
		BestCluster->ClusterComponent->bCastDynamicShadow             = InSettings->bCastDynamicShadow;
		BestCluster->ClusterComponent->bCastStaticShadow              = InSettings->bCastStaticShadow;
		BestCluster->ClusterComponent->bAffectDynamicIndirectLighting = InSettings->bAffectDynamicIndirectLighting;
		BestCluster->ClusterComponent->bCastShadowAsTwoSided          = InSettings->bCastShadowAsTwoSided;
		BestCluster->ClusterComponent->bReceivesDecals                = InSettings->bReceivesDecals;
		BestCluster->ClusterComponent->bOverrideLightMapRes           = InSettings->bOverrideLightMapRes;
		BestCluster->ClusterComponent->OverriddenLightMapRes          = InSettings->OverriddenLightMapRes;

		BestCluster->ClusterComponent->BodyInstance.CopyBodyInstancePropertiesFrom(&InSettings->BodyInstance);

		BestCluster->ClusterComponent->AttachTo(InIFA->GetRootComponent());
		BestCluster->ClusterComponent->RegisterComponent();
		
		// Use only instance translation as a component transform
		BestCluster->ClusterComponent->SetWorldTransform(FTransform(InstanceToWorld.GetTranslation()));
	}
	else
	{
		BestCluster->ClusterComponent->Modify();
		BestCluster->ClusterComponent->InvalidateLightingCache();
		BestCluster->Bounds = BestCluster->Bounds + InstanceWorldBounds;
	}

	BestCluster->InstanceIndices.Add(InstanceIndex);

	// Save the cluster index
	AddedInstance.ClusterIndex = BestClusterIndex;

	// Add the instance to the component
	BestCluster->ClusterComponent->AddInstanceWorldSpace(InstanceToWorld);

	if (BestCluster->ClusterComponent->SelectedInstances.Num() > 0)
	{
		BestCluster->ClusterComponent->SelectedInstances.Add(false);
	}

	// Update PrimitiveComponent's culling distance taking into account the radius of the bounds, as
	// it is based on the center of the component's bounds.
	float CullDistance = InSettings->EndCullDistance > 0 ? (float)InSettings->EndCullDistance + BestCluster->Bounds.SphereRadius : 0.f;
	BestCluster->ClusterComponent->LDMaxDrawDistance = CullDistance;
	BestCluster->ClusterComponent->CachedMaxDrawDistance = CullDistance;

	CheckValid();
}

void FFoliageMeshInfo::RemoveInstances(AInstancedFoliageActor* InIFA, const TArray<int32>& InInstancesToRemove)
{
	bool bRemoved = false;

	InIFA->Modify();

	// Used to store a list of instances to remove sorted by cluster
	TMap<int32, TSet<int32> > ClusterInstanceSetMap;

	// Remove instances from the hash
	for (TArray<int32>::TConstIterator It(InInstancesToRemove); It; ++It)
	{
		int32 InstanceIndex = *It;
		const FFoliageInstance& Instance = Instances[InstanceIndex];
		InstanceHash->RemoveInstance(Instance.Location, InstanceIndex);
		FFoliageComponentHashInfo* ComponentHashInfo = ComponentHash.Find(Instance.Base);
		if (ComponentHashInfo)
		{
			ComponentHashInfo->Instances.Remove(InstanceIndex);
			if (ComponentHashInfo->Instances.Num() == 0)
			{
				// Remove the component from the component hash if this is the last instance.
				ComponentHash.Remove(Instance.Base);
			}
		}

		// Store in ClusterInstanceSetMap
		int32 ClusterIndex = Instances[InstanceIndex].ClusterIndex;
		TSet<int32>& ClusterInstances = ClusterInstanceSetMap.FindOrAdd(ClusterIndex);
		ClusterInstances.Add(InstanceIndex);
	}

	// Process each cluster in turn
	for (TMap<int32, TSet<int32> >::TIterator It(ClusterInstanceSetMap); It; ++It)
	{
		int32 CurrentClusterIndex = It.Key();
		FFoliageInstanceCluster& Cluster = InstanceClusters[CurrentClusterIndex];
		UInstancedStaticMeshComponent* ClusterComponent = Cluster.ClusterComponent;
		bool bRemovedFromComponent = false;

		TSet<int32>& ClusterInstancesToRemove = It.Value();

		// Look through all indices in this cluster
		for (int32 ClusterInstanceIdx = 0; ClusterInstanceIdx < Cluster.InstanceIndices.Num(); ClusterInstanceIdx++)
		{
			int32 InstanceIndex = Cluster.InstanceIndices[ClusterInstanceIdx];

			// Check if this instance is one we need to remove
			if (ClusterInstancesToRemove.Contains(InstanceIndex))
			{
				// Remove the instance data and the index array entry
				if (ClusterComponent->IsRegistered())
				{
					ClusterComponent->UnregisterComponent();
				}
				ClusterComponent->Modify();
				ClusterComponent->PerInstanceSMData.RemoveAtSwap(ClusterInstanceIdx);
				if (ClusterInstanceIdx < ClusterComponent->SelectedInstances.Num())
				{
					ClusterComponent->SelectedInstances.RemoveAtSwap(ClusterInstanceIdx);
				}

				if (ClusterInstanceIdx < ClusterComponent->InstanceBodies.Num())
				{
					delete ClusterComponent->InstanceBodies[ClusterInstanceIdx];
					ClusterComponent->InstanceBodies.RemoveAtSwap(ClusterInstanceIdx);
				}

				Cluster.InstanceIndices.RemoveAtSwap(ClusterInstanceIdx);
				ClusterInstanceIdx--;
				bRemovedFromComponent = true;

				// Invalidate this instance's editor data so we reuse the slot later
				FFoliageInstance& InstanceEditorData = Instances[InstanceIndex];
				InstanceEditorData.ClusterIndex = INDEX_NONE;
				InstanceEditorData.Base = nullptr;

				// And remember the slot to reuse it.
				FreeInstanceIndices.Add(InstanceIndex);

				// Remove it from the selection.
				SelectedIndices.Remove(InstanceIndex);
			}
		}

		if (bRemovedFromComponent)
		{
			//!! need to update bounds for this cluster
			ClusterComponent->RegisterComponent();
			bRemoved = true;
		}
	}

	if (bRemoved)
	{
		// See if we can delete any clusters.
		for (int32 ClusterIdx = 0; ClusterIdx < InstanceClusters.Num(); ClusterIdx++)
		{
			if (InstanceClusters[ClusterIdx].InstanceIndices.Num() == 0)
			{
				// Unregister the component to remove it from the scene
				UInstancedStaticMeshComponent* Component = InstanceClusters[ClusterIdx].ClusterComponent;

				// Do not reregister this component if register all components gets called again
				Component->bAutoRegister = false;

				// Remove cluster
				InstanceClusters.RemoveAt(ClusterIdx);

				// update the ClusterIndex for remaining foliage instances
				for (int32 InstanceIdx = 0; InstanceIdx < Instances.Num(); InstanceIdx++)
				{
					if (Instances[InstanceIdx].ClusterIndex > ClusterIdx)
					{
						Instances[InstanceIdx].ClusterIndex--;
					}
				}

				ClusterIdx--;
			}

			InIFA->CheckSelection();
		}

		CheckValid();
	}
}

void FFoliageMeshInfo::PreMoveInstances(AInstancedFoliageActor* InIFA, const TArray<int32>& InInstancesToMove)
{
	// Remove instances from the hash
	for (TArray<int32>::TConstIterator It(InInstancesToMove); It; ++It)
	{
		int32 InstanceIndex = *It;
		const FFoliageInstance& Instance = Instances[InstanceIndex];
		InstanceHash->RemoveInstance(Instance.Location, InstanceIndex);
	}
}


void FFoliageMeshInfo::PostUpdateInstances(AInstancedFoliageActor* InIFA, const TArray<int32>& InInstancesUpdated, bool bReAddToHash)
{
	for (TArray<int32>::TConstIterator It(InInstancesUpdated); It; ++It)
	{
		int32 InstanceIndex = *It;
		const FFoliageInstance& Instance = Instances[InstanceIndex];

		// Update this instances' transform in the UInstancedStaticMeshComponent
		if (InstanceClusters.IsValidIndex(Instance.ClusterIndex))
		{
			FFoliageInstanceCluster& Cluster = InstanceClusters[Instance.ClusterIndex];
			Cluster.ClusterComponent->Modify();
			int32 ClusterInstanceDataIndex = Cluster.InstanceIndices.Find(InstanceIndex);
			check(ClusterInstanceDataIndex != INDEX_NONE);
			check(Cluster.ClusterComponent->PerInstanceSMData.IsValidIndex(ClusterInstanceDataIndex));
			Cluster.ClusterComponent->MarkRenderStateDirty();

			// Update bounds
			FTransform InstanceToWorld = Instance.GetInstanceWorldTransform();
			Cluster.Bounds = Cluster.Bounds + Cluster.ClusterComponent->StaticMesh->GetBounds().TransformBy(InstanceToWorld);
			// Update transform in InstancedStaticMeshComponent (in component local space)
			// Transform from world space to local space
			FTransform InstanceToLocal = InstanceToWorld.GetRelativeTransform(Cluster.ClusterComponent->ComponentToWorld);
			Cluster.ClusterComponent->PerInstanceSMData[ClusterInstanceDataIndex].Transform = InstanceToLocal.ToMatrixWithScale();
			Cluster.ClusterComponent->InvalidateLightingCache();
		}

		if (bReAddToHash)
		{
			// Re-add instance to the hash
			InstanceHash->InsertInstance(Instance.Location, InstanceIndex);
		}
	}
}

void FFoliageMeshInfo::PostMoveInstances(AInstancedFoliageActor* InIFA, const TArray<int32>& InInstancesMoved)
{
	PostUpdateInstances(InIFA, InInstancesMoved, true);
}

void FFoliageMeshInfo::DuplicateInstances(AInstancedFoliageActor* InIFA, UFoliageType* InSettings, const TArray<int32>& InInstancesToDuplicate)
{
	for (int32 InstanceIndex : InInstancesToDuplicate)
	{
		const FFoliageInstance TempInstance = Instances[InstanceIndex];
		AddInstance(InIFA, InSettings, TempInstance);
	}
}

/* Get the number of placed instances */
int32 FFoliageMeshInfo::GetInstanceCount() const
{
	int32 Count = 0;
	for (int32 Idx = 0; Idx < Instances.Num(); Idx++)
	{
		if (Instances[Idx].ClusterIndex != -1)
		{
			Count++;
		}
	}
	return Count;
}

// Destroy existing clusters and reassign all instances to new clusters
void FFoliageMeshInfo::ReallocateClusters(AInstancedFoliageActor* InIFA, UFoliageType* InSettings)
{
	// Detach all components
	InIFA->UnregisterAllComponents();

	// Delete any existing clusters from the components array
	for (FFoliageInstanceCluster& Cluster : InstanceClusters)
	{
		UInstancedStaticMeshComponent* Component = Cluster.ClusterComponent;
		if (Component != nullptr)
		{
			Component->PerInstanceSMData.Empty();
			Component->bAutoRegister = false;
		}
	}

	TArray<FFoliageInstance> OldInstances = Instances;
	OldInstances.RemoveAllSwap([](FFoliageInstance& Instance){ return Instance.ClusterIndex == INDEX_NONE; });

	// Remove everything
	InstanceClusters.Empty();
	Instances.Empty();
	InstanceHash->Empty();
	ComponentHash.Empty();
	FreeInstanceIndices.Empty();
	SelectedIndices.Empty();

	// Re-add
	for (FFoliageInstance& Instance : OldInstances)
	{
		Instance.ClusterIndex = INDEX_NONE;
		AddInstance(InIFA, InSettings, Instance);
	}

	InIFA->RegisterAllComponents();
}

// Update settings in the clusters based on the current settings (eg culling distance, collision, ...)
//void FFoliageMeshInfo::UpdateClusterSettings(AInstancedFoliageActor* InIFA)
//{
//	for (FFoliageInstanceCluster& Cluster : InstanceClusters)
//	{
//		UInstancedStaticMeshComponent* ClusterComponent = Cluster.ClusterComponent;
//		ClusterComponent->Modify();
//		ClusterComponent->MarkRenderStateDirty();
//
//		// Copy settings
//		ClusterComponent->InstanceStartCullDistance = Settings->StartCullDistance;
//		ClusterComponent->InstanceEndCullDistance = Settings->EndCullDistance;
//
//		// Update PrimitiveComponent's culling distance taking into account the radius of the bounds, as
//		// it is based on the center of the component's bounds.
//		float CullDistance = Settings->EndCullDistance > 0 ? (float)Settings->EndCullDistance + Cluster.Bounds.SphereRadius : 0.f;
//		ClusterComponent->LDMaxDrawDistance = CullDistance;
//		ClusterComponent->CachedMaxDrawDistance = CullDistance;
//	}
//
//	InIFA->MarkComponentsRenderStateDirty();
//}

void FFoliageMeshInfo::GetInstancesInsideSphere(const FSphere& Sphere, TArray<int32>& OutInstances)
{
	auto TempInstances = InstanceHash->GetInstancesOverlappingBox(FBox::BuildAABB(Sphere.Center, FVector(Sphere.W)));
	for (int32 Idx : TempInstances)
	{
		if (FSphere(Instances[Idx].Location, 0.f).IsInside(Sphere))
		{
			OutInstances.Add(Idx);
		}
	}
}

// Returns whether or not there is are any instances overlapping the sphere specified
bool FFoliageMeshInfo::CheckForOverlappingSphere(const FSphere& Sphere)
{
	auto TempInstances = InstanceHash->GetInstancesOverlappingBox(FBox::BuildAABB(Sphere.Center, FVector(Sphere.W)));
	for (int32 Idx : TempInstances)
	{
		if (FSphere(Instances[Idx].Location, 0.f).IsInside(Sphere))
		{
			return true;
		}
	}
	return false;
}

// Returns whether or not there is are any instances overlapping the instance specified, excluding the set of instances provided
bool FFoliageMeshInfo::CheckForOverlappingInstanceExcluding(int32 TestInstanceIdx, float Radius, TSet<int32>& ExcludeInstances)
{
	FSphere Sphere(Instances[TestInstanceIdx].Location, Radius);

	auto TempInstances = InstanceHash->GetInstancesOverlappingBox(FBox::BuildAABB(Sphere.Center, FVector(Sphere.W)));
	for (int32 Idx : TempInstances)
	{
		if (Idx != TestInstanceIdx && !ExcludeInstances.Contains(Idx) && FSphere(Instances[Idx].Location, 0.f).IsInside(Sphere))
		{
			return true;
		}
	}
	return false;
}

void FFoliageMeshInfo::SelectInstances(AInstancedFoliageActor* InIFA, bool bSelect, TArray<int32>& InInstances)
{
	InIFA->Modify();

	if (bSelect)
	{
		for (int32 i : InInstances)
		{
			SelectedIndices.AddUnique(i);
		}

		for (FFoliageInstanceCluster& Cluster : InstanceClusters)
		{
			// Apply any selections in the component
			Cluster.ClusterComponent->Modify();
			Cluster.ClusterComponent->MarkRenderStateDirty();
			if (Cluster.ClusterComponent->SelectedInstances.Num() != Cluster.InstanceIndices.Num())
			{
				Cluster.ClusterComponent->SelectedInstances.Init(false, Cluster.InstanceIndices.Num());
			}

			for (int32 ClusterInstanceIdx = 0; ClusterInstanceIdx < Cluster.InstanceIndices.Num(); ClusterInstanceIdx++)
			{
				int32 InstanceIdx = Cluster.InstanceIndices[ClusterInstanceIdx];
				if (InInstances.Find(InstanceIdx) != INDEX_NONE)
				{
					Cluster.ClusterComponent->SelectedInstances[ClusterInstanceIdx] = true;
				}
			}
		}
	}
	else
	{
		for (int32 i : InInstances)
		{
			SelectedIndices.RemoveSingleSwap(i);
		}

		for (FFoliageInstanceCluster& Cluster : InstanceClusters)
		{
			if (Cluster.ClusterComponent->SelectedInstances.Num() != 0)
			{
				// Remove any selections from the component
				Cluster.ClusterComponent->Modify();
				Cluster.ClusterComponent->MarkRenderStateDirty();

				for (int32 ClusterInstanceIdx = 0; ClusterInstanceIdx < Cluster.InstanceIndices.Num(); ClusterInstanceIdx++)
				{
					int32 InstanceIdx = Cluster.InstanceIndices[ClusterInstanceIdx];
					if (InInstances.Find(InstanceIdx) != INDEX_NONE)
					{
						Cluster.ClusterComponent->SelectedInstances[ClusterInstanceIdx] = false;
					}
				}
			}
		}
	}
}

#endif	//WITH_EDITOR

//
// AInstancedFoliageActor
//
AInstancedFoliageActor::AInstancedFoliageActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	USceneComponent* SceneComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("RootComponent0"));
	RootComponent = SceneComponent;
	RootComponent->Mobility = EComponentMobility::Static;
	
	SetActorEnableCollision(true);
#if WITH_EDITORONLY_DATA
	bListedInSceneOutliner = false;
#endif // WITH_EDITORONLY_DATA
	PrimaryActorTick.bCanEverTick = false;
}

#if WITH_EDITOR
AInstancedFoliageActor* AInstancedFoliageActor::GetInstancedFoliageActorForCurrentLevel(UWorld* InWorld, bool bCreateIfNone)
{
	for (TActorIterator<AInstancedFoliageActor> It(InWorld); It; ++It)
	{
		AInstancedFoliageActor* InstancedFoliageActor = *It;
		if (InstancedFoliageActor)
		{
			if (InstancedFoliageActor->GetLevel()->IsCurrentLevel() && !InstancedFoliageActor->IsPendingKill())
			{
				return InstancedFoliageActor;
			}
		}
	}

	return bCreateIfNone ? InWorld->SpawnActor<AInstancedFoliageActor>() : nullptr;
}


AInstancedFoliageActor* AInstancedFoliageActor::GetInstancedFoliageActorForLevel(ULevel* InLevel)
{
	if (!InLevel)
	{
		return nullptr;
	}

	for (int32 ActorIndex = 0; ActorIndex < InLevel->Actors.Num(); ++ActorIndex)
	{
		AInstancedFoliageActor* InstancedFoliageActor = Cast<AInstancedFoliageActor>(InLevel->Actors[ActorIndex]);
		if (InstancedFoliageActor && !InstancedFoliageActor->IsPendingKill())
		{
			return InstancedFoliageActor;
		}
	}
	return nullptr;
}

void AInstancedFoliageActor::MoveInstancesForMovedComponent(UActorComponent* InComponent)
{
	bool bUpdatedInstances = false;
	bool bFirst = true;

	for (auto& MeshPair : FoliageMeshes)
	{
		FFoliageMeshInfo& MeshInfo = *MeshPair.Value;
		FFoliageComponentHashInfo* ComponentHashInfo = MeshInfo.ComponentHash.Find(InComponent);
		if (ComponentHashInfo)
		{
			if (bFirst)
			{
				bFirst = false;
				Modify();
			}

			FVector OldLocation = ComponentHashInfo->CachedLocation;
			FRotator OldRotation = ComponentHashInfo->CachedRotation;
			FVector OldDrawScale = ComponentHashInfo->CachedDrawScale;
			ComponentHashInfo->UpdateLocationFromActor(InComponent);

			for (int32 InstanceIndex : ComponentHashInfo->Instances)
			{
				FFoliageInstance& Instance = MeshInfo.Instances[InstanceIndex];

				MeshInfo.InstanceHash->RemoveInstance(Instance.Location, InstanceIndex);

				// Apply change
				FMatrix DeltaTransform =
					FRotationMatrix(Instance.Rotation) *
					FTranslationMatrix(Instance.Location) *
					FTranslationMatrix(-OldLocation) *
					FInverseRotationMatrix(OldRotation) *
					FScaleMatrix(ComponentHashInfo->CachedDrawScale / OldDrawScale) *
					FRotationMatrix(ComponentHashInfo->CachedRotation) *
					FTranslationMatrix(ComponentHashInfo->CachedLocation);

				// Extract rotation and position
				Instance.Location = DeltaTransform.GetOrigin();
				Instance.Rotation = DeltaTransform.Rotator();

				// Update this instances' transform in the UInstancedStaticMeshComponent
				if (MeshInfo.InstanceClusters.IsValidIndex(Instance.ClusterIndex))
				{
					FFoliageInstanceCluster& Cluster = MeshInfo.InstanceClusters[Instance.ClusterIndex];
					Cluster.ClusterComponent->Modify();
					int32 ClusterInstanceDataIndex = Cluster.InstanceIndices.Find(InstanceIndex);
					check(ClusterInstanceDataIndex != INDEX_NONE);
					check(Cluster.ClusterComponent->PerInstanceSMData.IsValidIndex(ClusterInstanceDataIndex));
					Cluster.ClusterComponent->MarkRenderStateDirty();
					bUpdatedInstances = true;
					// Update bounds
					FTransform InstanceToWorld = Instance.GetInstanceWorldTransform();
					Cluster.Bounds = Cluster.Bounds + Cluster.ClusterComponent->StaticMesh->GetBounds().TransformBy(InstanceToWorld);
					// Update transform in InstancedStaticMeshComponent
					FTransform InstanceToLocal = InstanceToWorld.GetRelativeTransform(Cluster.ClusterComponent->ComponentToWorld);
					Cluster.ClusterComponent->PerInstanceSMData[ClusterInstanceDataIndex].Transform = InstanceToLocal.ToMatrixWithScale();
					Cluster.ClusterComponent->InvalidateLightingCache();
				}

				// Re-add the new instance location to the hash
				MeshInfo.InstanceHash->InsertInstance(Instance.Location, InstanceIndex);
			}
		}
	}
}

void AInstancedFoliageActor::DeleteInstancesForComponent(UActorComponent* InComponent)
{
	for (auto& MeshPair : FoliageMeshes)
	{
		FFoliageMeshInfo& MeshInfo = *MeshPair.Value;
		const FFoliageComponentHashInfo* ComponentHashInfo = MeshInfo.ComponentHash.Find(InComponent);
		if (ComponentHashInfo)
		{
			MeshInfo.RemoveInstances(this, ComponentHashInfo->Instances.Array());
		}
	}
}

void AInstancedFoliageActor::MoveInstancesForComponentToCurrentLevel(UActorComponent* InComponent)
{
	AInstancedFoliageActor* NewIFA = AInstancedFoliageActor::GetInstancedFoliageActorForCurrentLevel(InComponent->GetWorld());

	for (auto& MeshPair : FoliageMeshes)
	{
		FFoliageMeshInfo& MeshInfo = *MeshPair.Value;
		UFoliageType* FoliageType = MeshPair.Key;

		// Duplicate the foliage type if it's not shared
		if (FoliageType->GetOutermost() == GetOutermost())
		{
			FoliageType = (UFoliageType*)StaticDuplicateObject(FoliageType, NewIFA, nullptr, RF_AllFlags & ~(RF_Standalone | RF_Public));
		}
		
		const FFoliageComponentHashInfo* ComponentHashInfo = MeshInfo.ComponentHash.Find(InComponent);
		if (ComponentHashInfo)
		{
			FFoliageMeshInfo* NewMeshInfo = NewIFA->FindOrAddMesh(FoliageType);

			// Add the foliage to the new level
			for (int32 InstanceIndex : ComponentHashInfo->Instances)
			{
				NewMeshInfo->AddInstance(NewIFA, FoliageType, MeshInfo.Instances[InstanceIndex]);
			}

			// Remove from old level
			MeshInfo.RemoveInstances(this, ComponentHashInfo->Instances.Array());
		}
	}
}

void AInstancedFoliageActor::MoveInstancesToNewComponent(UPrimitiveComponent* InOldComponent, UPrimitiveComponent* InNewComponent)
{
	AInstancedFoliageActor* NewIFA = AInstancedFoliageActor::GetInstancedFoliageActorForLevel(InNewComponent->GetTypedOuter<ULevel>());
	if (!NewIFA)
	{
		NewIFA = InNewComponent->GetWorld()->SpawnActor<AInstancedFoliageActor>();
	}
	check(NewIFA);

	for (auto& MeshPair : FoliageMeshes)
	{
		FFoliageMeshInfo& MeshInfo = *MeshPair.Value;
		UFoliageType* FoliageType = MeshPair.Key;

		// Duplicate the foliage type if it's not shared
		if (FoliageType->GetOutermost() == GetOutermost())
		{
			FoliageType = (UFoliageType*)StaticDuplicateObject(FoliageType, NewIFA, nullptr, RF_AllFlags & ~(RF_Standalone | RF_Public));
		}

		const FFoliageComponentHashInfo* ComponentHashInfo = MeshInfo.ComponentHash.Find(InOldComponent);
		if (ComponentHashInfo)
		{
			// For same level can just remap the instances, otherwise we have to do a more complex move
			if (NewIFA == this)
			{
				FFoliageComponentHashInfo NewComponentHashInfo = MoveTemp(*ComponentHashInfo);
				NewComponentHashInfo.UpdateLocationFromActor(InNewComponent);

				// Update the instances
				for (int32 InstanceIndex : NewComponentHashInfo.Instances)
				{
					MeshInfo.Instances[InstanceIndex].Base = InNewComponent;
				}

				// Update the hash
				MeshInfo.ComponentHash.Add(InNewComponent, MoveTemp(NewComponentHashInfo));
				MeshInfo.ComponentHash.Remove(InOldComponent);
			}
			else
			{
				FFoliageMeshInfo* NewMeshInfo = NewIFA->FindOrAddMesh(FoliageType);

				// Add the foliage to the new level
				for (int32 InstanceIndex : ComponentHashInfo->Instances)
				{
					FFoliageInstance NewInstance = MeshInfo.Instances[InstanceIndex];
					NewInstance.Base = InNewComponent;
					NewInstance.ClusterIndex = INDEX_NONE;
					NewMeshInfo->AddInstance(NewIFA, FoliageType, NewInstance);
				}

				// Remove from old level
				MeshInfo.RemoveInstances(this, ComponentHashInfo->Instances.Array());
				check(!MeshInfo.ComponentHash.Contains(InOldComponent));
			}
		}
	}
}

TMap<UFoliageType*, TArray<const FFoliageInstancePlacementInfo*>> AInstancedFoliageActor::GetInstancesForComponent(UActorComponent* InComponent)
{
	TMap<UFoliageType*, TArray<const FFoliageInstancePlacementInfo*>> Result;

	for (auto& MeshPair : FoliageMeshes)
	{
		const FFoliageMeshInfo& MeshInfo = *MeshPair.Value;

		const FFoliageComponentHashInfo* ComponentHashInfo = MeshInfo.ComponentHash.Find(InComponent);
		if (ComponentHashInfo)
		{
			TArray<const FFoliageInstancePlacementInfo*>& Array = Result.Add(MeshPair.Key, TArray<const FFoliageInstancePlacementInfo*>());
			Array.Empty(ComponentHashInfo->Instances.Num());

			for (int32 InstanceIndex : ComponentHashInfo->Instances)
			{
				const FFoliageInstancePlacementInfo* Instance = &MeshInfo.Instances[InstanceIndex];
				Array.Add(Instance);
			}
		}
	}

	return Result;
}

FFoliageMeshInfo* AInstancedFoliageActor::FindMesh(UFoliageType* InType)
{
	TUniqueObj<FFoliageMeshInfo>* MeshInfoEntry = FoliageMeshes.Find(InType);
	FFoliageMeshInfo* MeshInfo = MeshInfoEntry ? &MeshInfoEntry->Get() : nullptr;
	return MeshInfo;
}

FFoliageMeshInfo* AInstancedFoliageActor::FindOrAddMesh(UFoliageType* InType)
{
	TUniqueObj<FFoliageMeshInfo>* MeshInfoEntry = FoliageMeshes.Find(InType);
	FFoliageMeshInfo* MeshInfo = MeshInfoEntry ? &MeshInfoEntry->Get() : AddMesh(InType);
	return MeshInfo;
}

FFoliageMeshInfo* AInstancedFoliageActor::AddMesh(UStaticMesh* InMesh, UFoliageType** OutSettings)
{
	check(GetSettingsForMesh(InMesh) == nullptr);

	MarkPackageDirty();

	UFoliageType_InstancedStaticMesh* Settings = nullptr;
#if WITH_EDITORONLY_DATA
	if (InMesh->FoliageDefaultSettings)
	{
		// TODO: Can't we just use this directly?
		Settings = DuplicateObject<UFoliageType_InstancedStaticMesh>(InMesh->FoliageDefaultSettings, this);
	}
	else
#endif
	{
		Settings = ConstructObject<UFoliageType_InstancedStaticMesh>(UFoliageType_InstancedStaticMesh::StaticClass(), this);
	}
	Settings->Mesh = InMesh;

	FFoliageMeshInfo* MeshInfo = AddMesh(Settings);

	const FBoxSphereBounds MeshBounds = InMesh->GetBounds();

	Settings->MeshBounds = MeshBounds;

	// Make bottom only bound
	FBox LowBound = MeshBounds.GetBox();
	LowBound.Max.Z = LowBound.Min.Z + (LowBound.Max.Z - LowBound.Min.Z) * 0.1f;

	float MinX = FLT_MAX, MaxX = FLT_MIN, MinY = FLT_MAX, MaxY = FLT_MIN;
	Settings->LowBoundOriginRadius = FVector::ZeroVector;

	if (InMesh->RenderData)
	{
		FPositionVertexBuffer& PositionVertexBuffer = InMesh->RenderData->LODResources[0].PositionVertexBuffer;
		for (uint32 Index = 0; Index < PositionVertexBuffer.GetNumVertices(); ++Index)
		{
			const FVector& Pos = PositionVertexBuffer.VertexPosition(Index);
			if (Pos.Z < LowBound.Max.Z)
			{
				MinX = FMath::Min(MinX, Pos.X);
				MinY = FMath::Min(MinY, Pos.Y);
				MaxX = FMath::Max(MaxX, Pos.X);
				MaxY = FMath::Max(MaxY, Pos.Y);
			}
		}
	}

	Settings->LowBoundOriginRadius = FVector((MinX + MaxX), (MinY + MaxY), FMath::Sqrt(FMath::Square(MaxX - MinX) + FMath::Square(MaxY - MinY))) * 0.5f;

	if (OutSettings)
	{
		*OutSettings = Settings;
	}

	return MeshInfo;
}

FFoliageMeshInfo* AInstancedFoliageActor::AddMesh(UFoliageType* InType)
{
	check(FoliageMeshes.Find(InType) == nullptr);

	MarkPackageDirty();

	if (InType->DisplayOrder == 0)
	{
		int32 MaxDisplayOrder = 0;
		for (auto& MeshPair : FoliageMeshes)
		{
			if (MeshPair.Key->DisplayOrder > MaxDisplayOrder)
			{
				MaxDisplayOrder = MeshPair.Key->DisplayOrder;
			}
		}
		InType->DisplayOrder = MaxDisplayOrder + 1;
	}

	FFoliageMeshInfo* MeshInfo = &*FoliageMeshes.Add(InType);
	InType->IsSelected = true;

	return MeshInfo;
}

void AInstancedFoliageActor::RemoveMesh(UFoliageType* InSettings)
{
	Modify();
	MarkPackageDirty();
	UnregisterAllComponents();

	// Remove all components for this mesh from the Components array.
	FFoliageMeshInfo* MeshInfo = FindMesh(InSettings);
	if (MeshInfo)
	{
		for (FFoliageInstanceCluster& Cluster : MeshInfo->InstanceClusters)
		{
			UInstancedStaticMeshComponent* Component = Cluster.ClusterComponent;
			if (Component != nullptr)
			{
				Component->bAutoRegister = false;
			}
		}
	}

	FoliageMeshes.Remove(InSettings);
	RegisterAllComponents();
	CheckSelection();
}

UFoliageType* AInstancedFoliageActor::GetSettingsForMesh(UStaticMesh* InMesh, FFoliageMeshInfo** OutMeshInfo)
{
	UFoliageType* Type = nullptr;
	FFoliageMeshInfo* MeshInfo = nullptr;

	for (auto& MeshPair : FoliageMeshes)
	{
		UFoliageType* Settings = MeshPair.Key;
		if (Settings && Settings->GetStaticMesh() == InMesh)
		{
			Type = MeshPair.Key;
			MeshInfo = &*MeshPair.Value;
			break;
		}
	}

	if (OutMeshInfo)
	{
		*OutMeshInfo = MeshInfo;
	}
	return Type;
}

void AInstancedFoliageActor::SelectInstance(UInstancedStaticMeshComponent* InComponent, int32 InComponentInstanceIndex, bool bToggle)
{
	bool bNeedsUpdate = false;

	Modify();

	// If we're not toggling, we need to first deselect everything else
	if (!bToggle)
	{
		for (auto& MeshPair : FoliageMeshes)
		{
			FFoliageMeshInfo& MeshInfo = *MeshPair.Value;

			if (MeshInfo.SelectedIndices.Num() > 0)
			{
				for (int32 ClusterIdx = 0; ClusterIdx < MeshInfo.InstanceClusters.Num(); ClusterIdx++)
				{
					FFoliageInstanceCluster& Cluster = MeshInfo.InstanceClusters[ClusterIdx];
					if (Cluster.ClusterComponent->SelectedInstances.Num() > 0)
					{
						Cluster.ClusterComponent->Modify();
						Cluster.ClusterComponent->SelectedInstances.Empty();
						Cluster.ClusterComponent->MarkRenderStateDirty();
					}
				}
				bNeedsUpdate = true;
			}

			MeshInfo.SelectedIndices.Empty();
		}
	}

	if (InComponent)
	{
		UFoliageType* Type = nullptr;
		FFoliageMeshInfo* MeshInfo = nullptr;

		Type = GetSettingsForMesh(InComponent->StaticMesh, &MeshInfo);

		if (MeshInfo)
		{
			FFoliageInstanceCluster* Cluster = MeshInfo->InstanceClusters.FindByPredicate([InComponent](FFoliageInstanceCluster& Cluster){ return Cluster.ClusterComponent == InComponent; });
			if (Cluster)
			{
				InComponent->Modify();
				int32 InstanceIndex = Cluster->InstanceIndices[InComponentInstanceIndex];
				int32 SelectedIndex = MeshInfo->SelectedIndices.Find(InstanceIndex);

				bNeedsUpdate = true;

				// Deselect if it's already selected.
				if (InComponentInstanceIndex < InComponent->SelectedInstances.Num())
				{
					InComponent->SelectedInstances[InComponentInstanceIndex] = false;
					InComponent->MarkRenderStateDirty();
				}
				if (SelectedIndex != INDEX_NONE)
				{
					MeshInfo->SelectedIndices.RemoveAt(SelectedIndex);
				}

				if (bToggle && SelectedIndex != INDEX_NONE)
				{
					if (SelectedMesh == Type && MeshInfo->SelectedIndices.Num() == 0)
					{
						SelectedMesh = nullptr;
					}
				}
				else
				{
					// Add the selection
					if (InComponent->SelectedInstances.Num() < InComponent->PerInstanceSMData.Num())
					{
						InComponent->SelectedInstances.Init(false, Cluster->InstanceIndices.Num());
					}
					InComponent->SelectedInstances[InComponentInstanceIndex] = true;
					InComponent->MarkRenderStateDirty();

					SelectedMesh = Type;
					MeshInfo->SelectedIndices.Insert(InstanceIndex, 0);
				}
			}
		}
	}

	if (bNeedsUpdate)
	{
		// Update selection
		MarkComponentsRenderStateDirty();
	}

	CheckSelection();
}

void AInstancedFoliageActor::ApplySelectionToComponents(bool bApply)
{
	bool bNeedsUpdate = false;

	for (auto& MeshPair : FoliageMeshes)
	{
		FFoliageMeshInfo& MeshInfo = *MeshPair.Value;

		if (bApply)
		{
			if (MeshInfo.SelectedIndices.Num() > 0)
			{
				for (FFoliageInstanceCluster& Cluster : MeshInfo.InstanceClusters)
				{
					// Apply any selections in the component
					Cluster.ClusterComponent->SelectedInstances.Init(false, Cluster.InstanceIndices.Num());
					Cluster.ClusterComponent->MarkRenderStateDirty();
					bNeedsUpdate = true;

					for (int32 ClusterInstanceIdx = 0; ClusterInstanceIdx < Cluster.InstanceIndices.Num(); ClusterInstanceIdx++)
					{
						int32 InstanceIdx = Cluster.InstanceIndices[ClusterInstanceIdx];
						if (MeshInfo.SelectedIndices.Find(InstanceIdx) != INDEX_NONE)
						{
							Cluster.ClusterComponent->SelectedInstances[ClusterInstanceIdx] = true;
						}
					}
				}
			}
		}
		else
		{
			for (FFoliageInstanceCluster& Cluster : MeshInfo.InstanceClusters)
			{
				if (Cluster.ClusterComponent->SelectedInstances.Num() > 0)
				{
					// remove any selections in the component
					Cluster.ClusterComponent->SelectedInstances.Empty();
					Cluster.ClusterComponent->MarkRenderStateDirty();
					bNeedsUpdate = true;
				}
			}
		}
	}

	if (bNeedsUpdate)
	{
		// Update selection
		MarkComponentsRenderStateDirty();
	}
}

void AInstancedFoliageActor::CheckSelection()
{
	// Check if we have to change the selection.
	if (SelectedMesh != nullptr)
	{
		FFoliageMeshInfo* MeshInfo = FindMesh(SelectedMesh);
		if (MeshInfo && MeshInfo->SelectedIndices.Num() > 0)
		{
			return;
		}
	}

	SelectedMesh = nullptr;

	// Try to find a new selection
	for (auto& MeshPair : FoliageMeshes)
	{
		FFoliageMeshInfo& MeshInfo = *MeshPair.Value;
		if (MeshInfo.SelectedIndices.Num() > 0)
		{
			SelectedMesh = MeshPair.Key;
			return;
		}
	}
}

FVector AInstancedFoliageActor::GetSelectionLocation()
{
	FVector Result(0, 0, 0);

	if (SelectedMesh != nullptr)
	{
		FFoliageMeshInfo* MeshInfo = FindMesh(SelectedMesh);
		if (MeshInfo && MeshInfo->SelectedIndices.Num() > 0)
		{
			Result = MeshInfo->Instances[MeshInfo->SelectedIndices[0]].Location;
		}
	}

	return Result;
}

void AInstancedFoliageActor::MapRebuild()
{
	// Map rebuilt - this may have modified the BSP components and thrown the previous ones away - if so we need to migrate the foliage across.
	UE_LOG(LogInstancedFoliage, Log, TEXT("Map Rebuilt - Update all BSP painted foliage!"));

	TMap<UFoliageType*, TArray<FFoliageInstance>> NewInstances;
	TArray<UModelComponent*> RemovedModelComponents;
	UWorld* World = GetWorld();
	check(World);

	// For each foliage brush, represented by the mesh/info pair
	for (auto& MeshPair : FoliageMeshes)
	{
		// each target component has some foliage instances
		FFoliageMeshInfo const& MeshInfo = *MeshPair.Value;
		UFoliageType* Settings = MeshPair.Key;
		check(Settings);

		for (auto& ComponentFoliagePair : MeshInfo.ComponentHash)
		{
			// BSP components are UModelComponents - they are the only ones we need to change
			UModelComponent* TargetComponent = Cast<UModelComponent>(ComponentFoliagePair.Key);
			if (TargetComponent)
			{
				// Delete its instances later
				RemovedModelComponents.Add(TargetComponent);

				FFoliageComponentHashInfo const& FoliageInfo = ComponentFoliagePair.Value;

				// We have to test each instance to see if we can migrate it across
				for (int32 InstanceIdx : FoliageInfo.Instances)
				{
					// Use a line test against the world, similar to FoliageEditMode.
					check(MeshInfo.Instances.IsValidIndex(InstanceIdx));
					FFoliageInstance const& Instance = MeshInfo.Instances[InstanceIdx];

					FFoliageInstance NewInstance = Instance;
					NewInstance.ClusterIndex = INDEX_NONE;

					FTransform InstanceToWorld = Instance.GetInstanceWorldTransform();
					FVector Down(-FVector::UpVector);
					FVector Start(InstanceToWorld.TransformPosition(FVector::UpVector));
					FVector End(InstanceToWorld.TransformPosition(Down));

					FHitResult Result;
					bool bHit = World->LineTraceSingle(Result, Start, End, FCollisionQueryParams(true), FCollisionObjectQueryParams(ECC_WorldStatic));

					if (bHit && Result.Component.IsValid() && Result.Component->IsA(UModelComponent::StaticClass()))
					{
						NewInstance.Base = CastChecked<UPrimitiveComponent>(Result.Component.Get());
						NewInstances.FindOrAdd(Settings).Add(NewInstance);
					}
				}
			}
		}
	}

	// Remove all existing & broken instances & component references.
	for (UModelComponent* Component : RemovedModelComponents)
	{
		DeleteInstancesForComponent(Component);
	}

	// And then finally add our new instances to the correct target components.
	for (auto& NewInstancePair : NewInstances)
	{
		UFoliageType* Settings = NewInstancePair.Key;
		check(Settings);
		FFoliageMeshInfo& MeshInfo = *FindOrAddMesh(Settings);
		for (FFoliageInstance& Instance : NewInstancePair.Value)
		{
			MeshInfo.AddInstance(this, Settings, Instance);
		}
	}
}

void AInstancedFoliageActor::ApplyLevelTransform(const FTransform& LevelTransform)
{
	// Apply transform to foliage editor only data
	if (GIsEditor)
	{
		for (auto& MeshPair : FoliageMeshes)
		{
			FFoliageMeshInfo& MeshInfo = *MeshPair.Value;
		
			MeshInfo.InstanceHash->Empty();
			for (int32 InstanceIdx = 0; InstanceIdx < MeshInfo.Instances.Num(); InstanceIdx++)
			{
				FFoliageInstance& Instance = MeshInfo.Instances[InstanceIdx];
				FTransform NewTransform = Instance.GetInstanceWorldTransform() * LevelTransform;
			
				Instance.Location		= NewTransform.GetLocation();
				Instance.Rotation		= NewTransform.GetRotation().Rotator();
				Instance.DrawScale3D	= NewTransform.GetScale3D();
			
				// Rehash instance location
				MeshInfo.InstanceHash->InsertInstance(Instance.Location, InstanceIdx);
			}

			for (auto It = MeshInfo.ComponentHash.CreateIterator(); It; ++It)
			{
				// We assume here that component we painted foliage on, was transformed as well
				FFoliageComponentHashInfo& Info = It.Value();
				Info.UpdateLocationFromActor(It.Key());
			}

			// Recalc cluster bounds
			for (FFoliageInstanceCluster& Cluster : MeshInfo.InstanceClusters)
			{
				Cluster.Bounds = Cluster.ClusterComponent->CalcBounds(Cluster.ClusterComponent->ComponentToWorld);
			}
		}
	}
}

#endif // WITH_EDITOR

struct FFoliageMeshInfo_Old
{
	TArray<FFoliageInstanceCluster> InstanceClusters;
	TArray<FFoliageInstance> Instances;
	//FFoliageInstanceHash* InstanceHash;
	//TMap<UActorComponent*, FFoliageComponentHashInfo> ComponentHash;
	//TArray<int32> FreeInstanceIndices;
	//TArray<int32> SelectedIndices;
	UFoliageType_InstancedStaticMesh* Settings; // Type remapped via +ActiveClassRedirects
};
FArchive& operator<<(FArchive& Ar, FFoliageMeshInfo_Old& MeshInfo)
{
	Ar << MeshInfo.InstanceClusters;
	Ar << MeshInfo.Instances;
	Ar << MeshInfo.Settings;

	return Ar;
}

void AInstancedFoliageActor::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	if (Ar.UE4Ver() < VER_UE4_FOLIAGE_SETTINGS_TYPE)
	{
		TMap<UStaticMesh*, FFoliageMeshInfo_Old> OldFoliageMeshes;
		Ar << OldFoliageMeshes;
		for (auto& OldMeshInfo : OldFoliageMeshes)
		{
			FFoliageMeshInfo NewMeshInfo;
			NewMeshInfo.InstanceClusters = MoveTemp(OldMeshInfo.Value.InstanceClusters);
#if WITH_EDITORONLY_DATA
			NewMeshInfo.Instances = MoveTemp(OldMeshInfo.Value.Instances);
#endif

			UFoliageType_InstancedStaticMesh* FoliageType = OldMeshInfo.Value.Settings;
			if (FoliageType == nullptr)
			{
				// If the Settings object was null, eg the user forgot to save their settings asset, create a new one.
				FoliageType = ConstructObject<UFoliageType_InstancedStaticMesh>(UFoliageType_InstancedStaticMesh::StaticClass(), this);
			}

			if (FoliageType->Mesh == nullptr)
			{
				FoliageType->Modify();
				FoliageType->Mesh = OldMeshInfo.Key;
			}
			else if (FoliageType->Mesh != OldMeshInfo.Key)
			{
				// If mesh doesn't match (two meshes sharing the same settings object?) then we need to duplicate as that is no longer supported
				FoliageType = (UFoliageType_InstancedStaticMesh*)StaticDuplicateObject(FoliageType, this, nullptr, RF_AllFlags & ~(RF_Standalone | RF_Public));
				FoliageType->Mesh = OldMeshInfo.Key;
			}
			FoliageMeshes.Add(FoliageType, TUniqueObj<FFoliageMeshInfo>(MoveTemp(NewMeshInfo)));
		}
	}
	else
	{
		Ar << FoliageMeshes;
	}

	if (Ar.UE4Ver() < VER_UE4_FOLIAGE_STATIC_LIGHTING_SUPPORT)
	{
		for (auto& MeshInfo : FoliageMeshes)
		{
			for (const FFoliageInstanceCluster& Cluster : MeshInfo.Value->InstanceClusters)
			{
				if (Ar.UE4Ver() < VER_UE4_FOLIAGE_STATIC_LIGHTING_SUPPORT)
				{
					Cluster.ClusterComponent->SetMobility(EComponentMobility::Static);
				}
				if (Ar.UE4Ver() < VER_UE4_FOLIAGE_COLLISION)
				{
					Cluster.ClusterComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				}
			}
		}
	}

	// Attach cluster components to a root component
	if (Ar.UE4Ver() < VER_UE4_ADD_ROOTCOMPONENT_TO_FOLIAGEACTOR && GIsEditor && Ar.IsLoading())
	{
		TArray<UInstancedStaticMeshComponent*> ClusterComponents;
		GetComponents(ClusterComponents);
		for (UInstancedStaticMeshComponent* Component : ClusterComponents)
		{
			Component->AttachTo(GetRootComponent(), NAME_None, EAttachLocation::KeepWorldPosition);
		}
	}
}

void AInstancedFoliageActor::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITORONLY_DATA
	if (GetLinkerUE4Version() < VER_UE4_DISALLOW_FOLIAGE_ON_BLUEPRINTS)
	{
		for (auto& MeshPair : FoliageMeshes)
		{
			for (FFoliageInstance& Instance : MeshPair.Value->Instances)
			{
				// Clear out the Base for any instances based on blueprint-created components,
				// as those components will be destroyed when the construction scripts are
				// re-run, leaving dangling references and causing crashes (woo!)
				if (Instance.Base && Instance.Base->bCreatedByConstructionScript)
				{
					Instance.Base = NULL;
				}
			}
		}
	}
#endif

#if WITH_EDITOR
	if (GIsEditor)
	{
		{
			bool bContainsNull = FoliageMeshes.Remove(nullptr) > 0;
			if (bContainsNull)
			{
				FMessageLog("MapCheck").Warning()
					->AddToken(FUObjectToken::Create(this))
					->AddToken(FTextToken::Create(LOCTEXT("MapCheck_Message_FoliageMissingStaticMesh", "Foliage instances for a missing static mesh have been removed.")))
					->AddToken(FMapErrorToken::Create(FMapErrors::FoliageMissingStaticMesh));
				while (bContainsNull)
				{
					bContainsNull = FoliageMeshes.Remove(nullptr) > 0;
				}
			}
		}
		for (auto& MeshPair : FoliageMeshes)
		{
			// Find the per-mesh info matching the mesh.
			FFoliageMeshInfo& MeshInfo = *MeshPair.Value;

			// Update the FreeInstanceIndices list and hash.
			for (int32 InstanceIdx = 0; InstanceIdx < MeshInfo.Instances.Num(); InstanceIdx++)
			{
				FFoliageInstance& Instance = MeshInfo.Instances[InstanceIdx];
				if (Instance.ClusterIndex == INDEX_NONE)
				{
					// Add invalid instances to the FreeInstanceIndices list.
					MeshInfo.FreeInstanceIndices.Add(InstanceIdx);
					Instance.Base = nullptr;
				}
				else
				{
					// Add valid instances to the hash.
					MeshInfo.InstanceHash->InsertInstance(Instance.Location, InstanceIdx);
					FFoliageComponentHashInfo* ComponentHashInfo = MeshInfo.ComponentHash.Find(Instance.Base);
					if (ComponentHashInfo == nullptr)
					{
						ComponentHashInfo = &MeshInfo.ComponentHash.Add(Instance.Base, FFoliageComponentHashInfo(Instance.Base));
					}
					ComponentHashInfo->Instances.Add(InstanceIdx);
				}
			}

			// Check if any of the clusters are nullptr
			int32 NumMissingComponents = 0;
			for (FFoliageInstanceCluster& Cluster : MeshInfo.InstanceClusters)
			{
				if (Cluster.ClusterComponent == nullptr)
				{
					NumMissingComponents++;
				}
			}
			if (NumMissingComponents > 0)
			{
				FFormatNamedArguments Arguments;
				Arguments.Add(TEXT("MissingCount"), NumMissingComponents);
				Arguments.Add(TEXT("MeshName"), FText::FromString(MeshPair.Key->GetName()));
				FMessageLog("MapCheck").Warning()
					->AddToken(FUObjectToken::Create(this))
					->AddToken(FTextToken::Create(FText::Format(LOCTEXT("MapCheck_Message_FoliageMissingClusterComponent", "Foliage in this map is missing {MissingCount} cluster component(s) for static mesh {MeshName}. Opening the Foliage tool will this problem."), Arguments)))
					->AddToken(FMapErrorToken::Create(FMapErrors::FoliageMissingClusterComponent));
			}
		}
	}
#endif
}

//
// Serialize all our UObjects for RTGC 
//
void AInstancedFoliageActor::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	AInstancedFoliageActor* This = CastChecked<AInstancedFoliageActor>(InThis);
	if (This->SelectedMesh)
	{
		Collector.AddReferencedObject(This->SelectedMesh, This);
	}

	for (auto& MeshPair : This->FoliageMeshes)
	{
		Collector.AddReferencedObject(MeshPair.Key, This);
		FFoliageMeshInfo& MeshInfo = *MeshPair.Value;

#if WITH_EDITORONLY_DATA
		for (FFoliageInstance& Instance : MeshInfo.Instances)
		{
			if (Instance.Base != nullptr)
			{
				Collector.AddReferencedObject(Instance.Base, This);
			}
		}
#endif

		for (FFoliageInstanceCluster& Cluster : MeshInfo.InstanceClusters)
		{
			Collector.AddReferencedObject(Cluster.ClusterComponent, This);
		}
	}
	Super::AddReferencedObjects(This, Collector);
}

/** InstancedStaticMeshInstance hit proxy */
void HInstancedStaticMeshInstance::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(Component);
}

void AInstancedFoliageActor::ApplyWorldOffset(const FVector& InOffset, bool bWorldShift)
{
	Super::ApplyWorldOffset(InOffset, bWorldShift);

	if (GIsEditor)
	{
		for (auto& MeshPair : FoliageMeshes)
		{
			FFoliageMeshInfo& MeshInfo = *MeshPair.Value;

			for (FFoliageInstanceCluster& Cluster : MeshInfo.InstanceClusters)
			{
				Cluster.Bounds.Origin += InOffset;
			}

#if WITH_EDITORONLY_DATA
			MeshInfo.InstanceHash->Empty();
			for (int32 InstanceIdx = 0; InstanceIdx < MeshInfo.Instances.Num(); InstanceIdx++)
			{
				FFoliageInstance& Instance = MeshInfo.Instances[InstanceIdx];
				if (Instance.ClusterIndex != INDEX_NONE)
				{
					Instance.Location += InOffset;
					// Rehash instance location
					MeshInfo.InstanceHash->InsertInstance(Instance.Location, InstanceIdx);
				}
			}

			for (auto It = MeshInfo.ComponentHash.CreateIterator(); It; ++It)
			{
				// We assume here that component we painted foliage on will be shifted by same value
				FFoliageComponentHashInfo& Info = It.Value();
				Info.CachedLocation += InOffset;
			}
#endif
		}
	}
}

#undef LOCTEXT_NAMESPACE
