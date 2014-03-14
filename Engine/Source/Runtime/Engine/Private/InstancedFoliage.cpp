// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	InstancedFoliage.cpp: Instanced foliage implementation.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineFoliageClasses.h"
#include "MessageLog.h"
#include "UObjectToken.h"
#include "MapErrors.h"

#define LOCTEXT_NAMESPACE "InstancedFoliage"

#define DO_FOLIAGE_CHECK			0			// whether to validate foliage data during editing.
#define FOLIAGE_CHECK_TRANSFORM		0			// whether to compare transforms between render and painting data.


DEFINE_LOG_CATEGORY_STATIC(LogInstancedFoliage, Log, All);

IMPLEMENT_HIT_PROXY(HInstancedStaticMeshInstance,HHitProxy);

//
// Serializers for struct data
//

FArchive& operator<<( FArchive& Ar, FFoliageInstance& Instance )
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

FArchive& operator<<( FArchive& Ar, FFoliageInstanceCluster& Cluster )
{
	Ar << Cluster.Bounds;
	Ar << Cluster.ClusterComponent;

	//!! editor only
	Ar << Cluster.InstanceIndices;

	return Ar;
}

FArchive& operator<<( FArchive& Ar, FFoliageMeshInfo& MeshInfo )
{
	Ar << MeshInfo.InstanceClusters;
	//!! editor only
	Ar << MeshInfo.Instances;

	// Serialize the transient data for undo.
	if( Ar.IsTransacting() )
	{
		Ar << *MeshInfo.InstanceHash;
		Ar << MeshInfo.ComponentHash;
		Ar << MeshInfo.FreeInstanceIndices;
		Ar << MeshInfo.SelectedIndices;
	}

		Ar << MeshInfo.Settings;

	return Ar;
}

FArchive& operator<<( FArchive& Ar, FFoliageComponentHashInfo& ComponentHashInfo )
{
	return Ar << ComponentHashInfo.CachedLocation << ComponentHashInfo.CachedRotation << ComponentHashInfo.CachedDrawScale << ComponentHashInfo.Instances;
}

//
// UInstancedFoliageSettings
//

UInstancedFoliageSettings::UInstancedFoliageSettings(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		FName NAME_None;
		FConstructorStatics()
			: NAME_None(TEXT("None"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

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
	LandscapeLayer = ConstructorStatics.NAME_None;
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
	bAffectDynamicIndirectLighting = false;
	bCastStaticShadow = true;
	bCastHiddenShadow = false;
	bCastShadowAsTwoSided = false;
}

//
// FFoliageMeshInfo
//
FFoliageMeshInfo::FFoliageMeshInfo()
:	InstanceHash(NULL)
,	Settings(NULL)
{
	if( GIsEditor )
	{
		InstanceHash = new FFoliageInstanceHash();
	}
}

FFoliageMeshInfo::FFoliageMeshInfo(const FFoliageMeshInfo& Other)
:	InstanceHash(NULL)
,	Settings(NULL)
{
	if( GIsEditor )
	{
		InstanceHash = new FFoliageInstanceHash();
	}	
}

FFoliageMeshInfo::~FFoliageMeshInfo()
{
	if( GIsEditor )
	{
		delete InstanceHash;
	}
}

#if WITH_EDITOR


void FFoliageMeshInfo::CheckValid()
{
#if DO_FOLIAGE_CHECK
	int32 ClusterTotal=0;
	int32 ComponentTotal=0;

	for( int32 ClusterIdx=0;ClusterIdx<InstanceClusters.Num();ClusterIdx++ )
	{
		check( InstanceClusters[ClusterIdx].ClusterComponent != NULL);
		ClusterTotal += InstanceClusters[ClusterIdx].InstanceIndices.Num();
		ComponentTotal += InstanceClusters[ClusterIdx].ClusterComponent->PerInstanceSMData.Num();
	}

	check( ClusterTotal == ComponentTotal );

	int32 FreeTotal = 0;
	int32 InstanceTotal = 0;
	for( int32 InstanceIdx=0; InstanceIdx<Instances.Num(); InstanceIdx++ )
	{
		if( Instances[InstanceIdx].ClusterIndex != -1 )
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
	for( TMap<class UActorComponent*, FFoliageComponentHashInfo >::TConstIterator It(ComponentHash); It; ++It )
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

			FMatrix InstanceTransform = Instances(InstanceIdx).GetInstanceTransform();
			FMatrix& CompTransform = Comp->PerInstanceSMData(InstIdx).Transform;
			
			if( InstanceTransform != CompTransform )
			{
				CompTransform = InstanceTransform;
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

void FFoliageMeshInfo::AddInstance( AInstancedFoliageActor* InIFA, UStaticMesh* InMesh, const FFoliageInstance& InNewInstance )
{
	InIFA->Modify();

	// Add the instance taking either a free slot or adding a new item.
	int32 InstanceIndex = FreeInstanceIndices.Num() > 0 ?  FreeInstanceIndices.Pop() : Instances.AddUninitialized();

	FFoliageInstance& AddedInstance = Instances[InstanceIndex];
	AddedInstance = InNewInstance;

	// Add the instance to the hash
	InstanceHash->InsertInstance(InNewInstance.Location, InstanceIndex);
	FFoliageComponentHashInfo* ComponentHashInfo = ComponentHash.Find(InNewInstance.Base);
	if( ComponentHashInfo == NULL )
	{
		ComponentHashInfo = &ComponentHash.Add(InNewInstance.Base, FFoliageComponentHashInfo(InNewInstance.Base));
	}
	ComponentHashInfo->Instances.Add(InstanceIndex);

	// Find the best cluster to allocate the instance to.
	FFoliageInstanceCluster* BestCluster = NULL;
	int32 BestClusterIndex = INDEX_NONE;
	float BestClusterDistSq = FLT_MAX;

	int32 MaxInstancesPerCluster = Settings->MaxInstancesPerCluster;
	float MaxClusterRadiusSq = FMath::Square(Settings->MaxClusterRadius);

	for( int32 ClusterIdx=0;ClusterIdx<InstanceClusters.Num();ClusterIdx++ )
	{
		FFoliageInstanceCluster& Cluster = InstanceClusters[ClusterIdx];
		if( Cluster.InstanceIndices.Num() < MaxInstancesPerCluster )
		{
			float DistSq = (Cluster.Bounds.Origin - InNewInstance.Location).SizeSquared();
			if( DistSq < BestClusterDistSq && DistSq < MaxClusterRadiusSq )
			{
				BestCluster = &Cluster;
				BestClusterIndex = ClusterIdx;
				BestClusterDistSq = DistSq;
			}
		}
	}

	// Calculate transform for the instance
	FMatrix InstanceTransform = InNewInstance.GetInstanceTransform();

	if( BestCluster == NULL )
	{
		BestClusterIndex = InstanceClusters.Num();
		BestCluster = new(InstanceClusters) FFoliageInstanceCluster(
			ConstructObject<UInstancedStaticMeshComponent>(UInstancedStaticMeshComponent::StaticClass(),InIFA,NAME_None,RF_Transactional),
			InMesh->GetBounds().TransformBy(InstanceTransform)
			);
		// Set IsPendingKill() to true so that when the initial undo record is made,
		// the component will be treated as destroyed, in that undo an add will
		// actually work
		BestCluster->ClusterComponent->SetFlags(RF_PendingKill);
		BestCluster->ClusterComponent->Modify( false );
		BestCluster->ClusterComponent->ClearFlags(RF_PendingKill);
			
		BestCluster->ClusterComponent->StaticMesh = InMesh;
		BestCluster->ClusterComponent->bSelectable = true;
		BestCluster->ClusterComponent->bHasPerInstanceHitProxies = true;
		BestCluster->ClusterComponent->InstancingRandomSeed = FMath::Rand();
		BestCluster->ClusterComponent->InstanceStartCullDistance = Settings->StartCullDistance;
		BestCluster->ClusterComponent->InstanceEndCullDistance = Settings->EndCullDistance;

		BestCluster->ClusterComponent->CastShadow = Settings->CastShadow;
		BestCluster->ClusterComponent->bCastDynamicShadow = Settings->bCastDynamicShadow;
		BestCluster->ClusterComponent->bAffectDynamicIndirectLighting = Settings->bAffectDynamicIndirectLighting;
		BestCluster->ClusterComponent->bCastStaticShadow = Settings->bCastStaticShadow;
		BestCluster->ClusterComponent->bCastHiddenShadow = Settings->bCastHiddenShadow;
		BestCluster->ClusterComponent->bCastShadowAsTwoSided = Settings->bCastShadowAsTwoSided;

		BestCluster->ClusterComponent->SetRelativeTransform(FTransform::Identity);
		BestCluster->ClusterComponent->RegisterComponent();
	}
	else
	{
		BestCluster->ClusterComponent->Modify();
		BestCluster->ClusterComponent->InvalidateLightingCache();
	}

	BestCluster->InstanceIndices.Add(InstanceIndex);

	// Save the cluster index
	AddedInstance.ClusterIndex = BestClusterIndex;
	
	// Add the instance to the component
	BestCluster->ClusterComponent->AddInstance(FTransform(InstanceTransform));

	if( BestCluster->ClusterComponent->SelectedInstances.Num() > 0 )
	{
		BestCluster->ClusterComponent->SelectedInstances.Add(false);
	}

	// Update the bounds for the cluster
	BestCluster->Bounds = BestCluster->ClusterComponent->CalcBounds(FTransform::Identity);

	// Update PrimitiveComponent's culling distance taking into account the radius of the bounds, as
	// it is based on the center of the component's bounds.
	float CullDistance = Settings->EndCullDistance > 0 ? (float)Settings->EndCullDistance + BestCluster->Bounds.SphereRadius : 0.f;
	BestCluster->ClusterComponent->LDMaxDrawDistance = CullDistance;
	BestCluster->ClusterComponent->CachedMaxDrawDistance = CullDistance;

	CheckValid();
}

void FFoliageMeshInfo::RemoveInstances( AInstancedFoliageActor* InIFA, const TArray<int32>& InInstancesToRemove )
{
	bool bRemoved = false;

	InIFA->Modify();

	// Used to store a list of instances to remove sorted by cluster
	TMap<int32, TSet<int32> > ClusterInstanceSetMap;

	// Remove instances from the hash
	for( TArray<int32>::TConstIterator It(InInstancesToRemove); It; ++It )
	{
		int32 InstanceIndex = *It;
		const FFoliageInstance& Instance = Instances[InstanceIndex];
		InstanceHash->RemoveInstance(Instance.Location, InstanceIndex);
		FFoliageComponentHashInfo* ComponentHashInfo = ComponentHash.Find(Instance.Base);
		if( ComponentHashInfo )
		{
			ComponentHashInfo->Instances.Remove(InstanceIndex);
			if( ComponentHashInfo->Instances.Num() == 0 )
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
	for( TMap<int32, TSet<int32> >::TIterator It(ClusterInstanceSetMap); It; ++It )
	{
		int32 CurrentClusterIndex = It.Key();
		FFoliageInstanceCluster& Cluster = InstanceClusters[CurrentClusterIndex];
		UInstancedStaticMeshComponent* ClusterComponent = Cluster.ClusterComponent;
		bool bRemovedFromComponent = false;

		TSet<int32>& ClusterInstancesToRemove = It.Value();

		// Look through all indices in this cluster
		for( int32 ClusterInstanceIdx=0;ClusterInstanceIdx<Cluster.InstanceIndices.Num();ClusterInstanceIdx++ )
		{
			int32 InstanceIndex = Cluster.InstanceIndices[ClusterInstanceIdx];

			// Check if this instance is one we need to remove
			if( ClusterInstancesToRemove.Contains(InstanceIndex) )
			{
				// Remove the instance data and the index array entry
				if( ClusterComponent->IsRegistered() )
				{
					ClusterComponent->UnregisterComponent();
				}
				ClusterComponent->Modify();
				ClusterComponent->PerInstanceSMData.RemoveAtSwap(ClusterInstanceIdx);
				if( ClusterInstanceIdx < ClusterComponent->SelectedInstances.Num() )
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
				InstanceEditorData.ClusterIndex = -1;
				InstanceEditorData.Base = NULL;

				// And remember the slot to reuse it.
				FreeInstanceIndices.Add(InstanceIndex);

				// Remove it from the selection.
				SelectedIndices.Remove(InstanceIndex);
			}
		}

		if( bRemovedFromComponent )
		{
			//!! need to update bounds for this cluster
			ClusterComponent->RegisterComponent();
			bRemoved = true;
		}
	}

	if( bRemoved )
	{
		// See if we can delete any clusters.
		for( int32 ClusterIdx=0;ClusterIdx<InstanceClusters.Num();ClusterIdx++ )
		{
			if( InstanceClusters[ClusterIdx].InstanceIndices.Num() == 0 )
			{			
				// Unregister the component to remove it from the scene
				UInstancedStaticMeshComponent* Component = InstanceClusters[ClusterIdx].ClusterComponent;

				// Do not reregister this component if register all components gets called again
				Component->bAutoRegister = false;

				// Remove cluster
				InstanceClusters.RemoveAt(ClusterIdx);

				// update the ClusterIndex for remaining foliage instances
				for( int32 InstanceIdx=0;InstanceIdx<Instances.Num();InstanceIdx++ )
				{
					if( Instances[InstanceIdx].ClusterIndex > ClusterIdx )
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

void FFoliageMeshInfo::PreMoveInstances( class AInstancedFoliageActor* InIFA, const TArray<int32>& InInstancesToMove )
{
	// Remove instances from the hash
	for( TArray<int32>::TConstIterator It(InInstancesToMove); It; ++It )
	{
		int32 InstanceIndex = *It;
		const FFoliageInstance& Instance = Instances[InstanceIndex];
		InstanceHash->RemoveInstance(Instance.Location, InstanceIndex);
	}
}


void FFoliageMeshInfo::PostUpdateInstances( class AInstancedFoliageActor* InIFA, const TArray<int32>& InInstancesUpdated, bool bReAddToHash )
{
	for( TArray<int32>::TConstIterator It(InInstancesUpdated); It; ++It )
	{
		int32 InstanceIndex = *It;
		const FFoliageInstance& Instance = Instances[InstanceIndex];

		// Update this instances' transform in the UInstancedStaticMeshComponent
		if( InstanceClusters.IsValidIndex(Instance.ClusterIndex) )
		{
			FFoliageInstanceCluster& Cluster = InstanceClusters[Instance.ClusterIndex];
			Cluster.ClusterComponent->Modify();
			int32 ClusterInstanceDataIndex = Cluster.InstanceIndices.Find(InstanceIndex);
			check(ClusterInstanceDataIndex != INDEX_NONE);					
			check(Cluster.ClusterComponent->PerInstanceSMData.IsValidIndex(ClusterInstanceDataIndex));
			Cluster.ClusterComponent->MarkRenderStateDirty();

			// Update bounds
			FMatrix InstanceTransform = Instance.GetInstanceTransform();
			Cluster.Bounds = Cluster.Bounds + Cluster.ClusterComponent->StaticMesh->GetBounds().TransformBy(InstanceTransform);
			// Update transform in InstancedStaticMeshComponent
			Cluster.ClusterComponent->PerInstanceSMData[ClusterInstanceDataIndex].Transform = InstanceTransform;
			Cluster.ClusterComponent->InvalidateLightingCache();
		}
		
		if( bReAddToHash )
		{
			// Re-add instance to the hash
			InstanceHash->InsertInstance(Instance.Location, InstanceIndex);
		}
	}
}

void FFoliageMeshInfo::PostMoveInstances( class AInstancedFoliageActor* InIFA, const TArray<int32>& InInstancesMoved )
{
	PostUpdateInstances( InIFA, InInstancesMoved, true );
}

void FFoliageMeshInfo::DuplicateInstances( class AInstancedFoliageActor* InIFA, class UStaticMesh* InMesh, const TArray<int32>& InInstancesToDuplicate )
{
	for( TArray<int32>::TConstIterator It(InInstancesToDuplicate); It; ++It )
	{
		int32 InstanceIndex = *It;
		const FFoliageInstance TempInstance = Instances[InstanceIndex];
		AddInstance(InIFA, InMesh, TempInstance);
	}
}

/* Get the number of placed instances */
int32 FFoliageMeshInfo::GetInstanceCount() const
{
	int32 Count = 0;
	for( int32 Idx=0;Idx<Instances.Num();Idx++ )
	{
		if( Instances[Idx].ClusterIndex != -1 )
		{
			Count++;
		}
	}
	return Count;
}

// Destroy existing clusters and reassign all instances to new clusters
void FFoliageMeshInfo::ReallocateClusters( AInstancedFoliageActor* InIFA, UStaticMesh* InMesh )
{
	// Detach all components
	InIFA->UnregisterAllComponents();

	// Delete any existing clusters from the components array
	for( int32 ClusterIndex=0;ClusterIndex < InstanceClusters.Num(); ClusterIndex++ )
	{
		UInstancedStaticMeshComponent* Component = InstanceClusters[ClusterIndex].ClusterComponent;
		if( Component != NULL )
		{
			Component->bAutoRegister = false;
		}
	}

	TArray<FFoliageInstance> OldInstances = Instances;
	for( int32 Idx=0;Idx<OldInstances.Num();Idx++ )
	{
		if( OldInstances[Idx].ClusterIndex == -1 )
		{
			OldInstances.RemoveAtSwap(Idx);
			Idx--;
		}
		else
		{
			OldInstances[Idx].ClusterIndex = -1;
		}
	}

	// Remove everything
	InstanceClusters.Empty();
	Instances.Empty();
	InstanceHash->Empty();
	ComponentHash.Empty();
	FreeInstanceIndices.Empty();
	SelectedIndices.Empty();

	// Re-add
	for( int32 Idx=0;Idx<OldInstances.Num();Idx++ )
	{
		AddInstance( InIFA, InMesh, OldInstances[Idx] );
	}

	InIFA->RegisterAllComponents();
}

// Update settings in the clusters based on the current settings (eg culling distance, collision, ...)
void FFoliageMeshInfo::UpdateClusterSettings( AInstancedFoliageActor* InIFA )
{
	for( int32 ClusterIdx=0;ClusterIdx<InstanceClusters.Num();ClusterIdx++ )
	{
		UInstancedStaticMeshComponent* ClusterComponent = InstanceClusters[ClusterIdx].ClusterComponent;
		ClusterComponent->Modify();
		ClusterComponent->MarkRenderStateDirty();

		// Copy settings
		ClusterComponent->InstanceStartCullDistance = Settings->StartCullDistance;
		ClusterComponent->InstanceEndCullDistance = Settings->EndCullDistance;	

		// Update PrimitiveComponent's culling distance taking into account the radius of the bounds, as
		// it is based on the center of the component's bounds.
		float CullDistance = Settings->EndCullDistance > 0 ? (float)Settings->EndCullDistance + InstanceClusters[ClusterIdx].Bounds.SphereRadius : 0.f;
		ClusterComponent->LDMaxDrawDistance = CullDistance;
		ClusterComponent->CachedMaxDrawDistance = CullDistance;
	}

	InIFA->MarkComponentsRenderStateDirty();
}

void FFoliageMeshInfo::GetInstancesInsideSphere( const FSphere& Sphere, TArray<int32>& OutInstances )
{
	TSet<int32> TempInstances;
	InstanceHash->GetInstancesOverlappingBox(FBox::BuildAABB(Sphere.Center, FVector(Sphere.W,Sphere.W,Sphere.W)), TempInstances );

	for( TSet<int32>::TConstIterator It(TempInstances); It; ++It )
	{
		if( FSphere(Instances[*It].Location,0.f).IsInside(Sphere) )
		{
			OutInstances.Add(*It);
		}
	}	
}

// Returns whether or not there is are any instances overlapping the sphere specified
bool FFoliageMeshInfo::CheckForOverlappingSphere( const FSphere& Sphere )
{
	TSet<int32> TempInstances;
	InstanceHash->GetInstancesOverlappingBox(FBox::BuildAABB(Sphere.Center, FVector(Sphere.W,Sphere.W,Sphere.W)), TempInstances );

	for( TSet<int32>::TConstIterator It(TempInstances); It; ++It )
	{
		if( FSphere(Instances[*It].Location,0.f).IsInside(Sphere) )
		{
			return true;
		}
	}
	return false;
}

// Returns whether or not there is are any instances overlapping the instance specified, excluding the set of instances provided
bool FFoliageMeshInfo::CheckForOverlappingInstanceExcluding( int32 TestInstanceIdx, float Radius, TSet<int32>& ExcludeInstances )
{
	FSphere Sphere( Instances[TestInstanceIdx].Location,Radius);
	TSet<int32> TempInstances;
	InstanceHash->GetInstancesOverlappingBox(FBox::BuildAABB(Sphere.Center, FVector(Sphere.W,Sphere.W,Sphere.W)), TempInstances );

	for( TSet<int32>::TConstIterator It(TempInstances); It; ++It )
	{
		if( *It != TestInstanceIdx && !ExcludeInstances.Contains(*It) && FSphere(Instances[*It].Location,0.f).IsInside(Sphere) )
		{
			return true;
		}
	}
	return false;
}

void FFoliageMeshInfo::SelectInstances( AInstancedFoliageActor* InIFA, bool bSelect, TArray<int32>& InInstances )
{
	InIFA->Modify();

	if( bSelect )
	{
		for( int32 i=0;i<InInstances.Num();i++ )
		{
			SelectedIndices.AddUnique(InInstances[i]);
		}

		for( int32 ClusterIdx=0;ClusterIdx<InstanceClusters.Num();ClusterIdx++ )
		{
			FFoliageInstanceCluster& Cluster = InstanceClusters[ClusterIdx];

			// Apply any selections in the component
			Cluster.ClusterComponent->Modify();
			Cluster.ClusterComponent->MarkRenderStateDirty();
			if( Cluster.ClusterComponent->SelectedInstances.Num() != Cluster.InstanceIndices.Num() )
			{
				Cluster.ClusterComponent->SelectedInstances.Init(false, Cluster.InstanceIndices.Num());
			}

			for( int32 ClusterInstanceIdx=0;ClusterInstanceIdx<Cluster.InstanceIndices.Num();ClusterInstanceIdx++ )
			{
				int32 InstanceIdx = Cluster.InstanceIndices[ClusterInstanceIdx];
				if( InInstances.Find(InstanceIdx) != INDEX_NONE )
				{
					Cluster.ClusterComponent->SelectedInstances[ClusterInstanceIdx] = true;
				}
			}
		}
	}
	else
	{
		for( int32 i=0;i<InInstances.Num();i++ )
		{
			SelectedIndices.RemoveSingleSwap(InInstances[i]);
		}

		for( int32 ClusterIdx=0;ClusterIdx<InstanceClusters.Num();ClusterIdx++ )
		{
			FFoliageInstanceCluster& Cluster = InstanceClusters[ClusterIdx];

			if( Cluster.ClusterComponent->SelectedInstances.Num() != 0 )
			{
				// Remove any selections from the component
				Cluster.ClusterComponent->Modify();
				Cluster.ClusterComponent->MarkRenderStateDirty();

				for( int32 ClusterInstanceIdx=0;ClusterInstanceIdx<Cluster.InstanceIndices.Num();ClusterInstanceIdx++ )
				{
					int32 InstanceIdx = Cluster.InstanceIndices[ClusterInstanceIdx];
					if( InInstances.Find(InstanceIdx) != INDEX_NONE )
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
AInstancedFoliageActor::AInstancedFoliageActor(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	SetActorEnableCollision(false);
#if WITH_EDITORONLY_DATA
	bListedInSceneOutliner = false;
#endif // WITH_EDITORONLY_DATA
}

#if WITH_EDITOR
AInstancedFoliageActor* AInstancedFoliageActor::GetInstancedFoliageActor(UWorld* InWorld,bool bCreateIfNone)
{
	for( TActorIterator<AInstancedFoliageActor> It(InWorld);It;++It )
	{
		AInstancedFoliageActor* InstancedFoliageActor = *It;
		if( InstancedFoliageActor )
		{
			if( InstancedFoliageActor->GetLevel()->IsCurrentLevel() && !InstancedFoliageActor->IsPendingKill() )
			{
				return InstancedFoliageActor;
			}
		}
	}

	return bCreateIfNone ? InWorld->SpawnActor<AInstancedFoliageActor>() : NULL;
}


AInstancedFoliageActor* AInstancedFoliageActor::GetInstancedFoliageActorForLevel(ULevel* InLevel)
{
	if (!InLevel)
	{
		return NULL;
	}

	for (int32 ActorIndex = 0; ActorIndex < InLevel->Actors.Num(); ++ActorIndex)
	{
		AInstancedFoliageActor* InstancedFoliageActor = Cast<AInstancedFoliageActor>(InLevel->Actors[ActorIndex]);
		if (InstancedFoliageActor && !InstancedFoliageActor->IsPendingKill())
		{
			return InstancedFoliageActor;
		}
	}
	return NULL;
}

void AInstancedFoliageActor::SnapInstancesForLandscape( class ULandscapeHeightfieldCollisionComponent* InLandscapeComponent, const FBox& InInstanceBox )
{
	for( TMap<class UStaticMesh*, struct FFoliageMeshInfo>::TIterator MeshIt(FoliageMeshes); MeshIt; ++MeshIt )
	{
		// Find the per-mesh info matching the mesh.
		FFoliageMeshInfo& MeshInfo = MeshIt.Value();

		FFoliageComponentHashInfo* ComponentHashInfo = MeshInfo.ComponentHash.Find(InLandscapeComponent);
		if( ComponentHashInfo )
		{
			float TraceExtentSize = InLandscapeComponent->Bounds.SphereRadius * 2.f + 10.f; // extend a little
			FVector TraceVector = InLandscapeComponent->GetOwner()->GetRootComponent()->ComponentToWorld.GetUnitAxis( EAxis::Z ) * TraceExtentSize;

			bool bFirst = true;
			TArray<int32> InstancesToRemove;
			for( TSet<int32>::TConstIterator InstIt(ComponentHashInfo->Instances); InstIt; ++InstIt )
			{
				int32 InstanceIndex = *InstIt;
				FFoliageInstance& Instance = MeshInfo.Instances[InstanceIndex];

				// Test location should remove any Z offset
				FVector TestLocation = FMath::Abs(Instance.ZOffset) > KINDA_SMALL_NUMBER
					?	(FVector)Instance.GetInstanceTransform().TransformPosition(FVector(0,0,-Instance.ZOffset))
					:	Instance.Location;

				if( InInstanceBox.IsInside(TestLocation) )
				{
					if( bFirst )
					{
						bFirst = false;
						Modify();
					}

					FVector Start = TestLocation + TraceVector;
					FVector End   = TestLocation - TraceVector;

					static FName TraceTag = FName(TEXT("FoliageSnapToLandscape"));
					TArray<FHitResult> Results;
					UWorld* World = InLandscapeComponent->GetWorld();
					check(World);
					World->LineTraceMulti(Results, Start, End, FCollisionQueryParams(TraceTag, true), FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects));
				
					bool bFoundHit = false;
					for( int32 HitIdx=0; HitIdx<Results.Num(); HitIdx++ )
					{
						const FHitResult& Hit = Results[HitIdx];
						if( Hit.Component == InLandscapeComponent )
						{
							bFoundHit = true;
							if( (TestLocation - Hit.Location).SizeSquared() > KINDA_SMALL_NUMBER )
							{
								// Remove instance location from the hash. Do not need to update ComponentHash as we re-add below.
								MeshInfo.InstanceHash->RemoveInstance(Instance.Location, InstanceIndex);
							
								// Update the instance editor data
								Instance.Location = Hit.Location;

								if( Instance.Flags & FOLIAGE_AlignToNormal )
								{
									// Remove previous alignment and align to new normal.
									Instance.Rotation = Instance.PreAlignRotation;
									Instance.AlignToNormal(Hit.Normal, MeshInfo.Settings->AlignMaxAngle);
								}

								// Reapply the Z offset in local space
								if( FMath::Abs(Instance.ZOffset) > KINDA_SMALL_NUMBER )
								{
									Instance.Location = Instance.GetInstanceTransform().TransformPosition(FVector(0,0,Instance.ZOffset));
								}

								// Todo: add do validation with other parameters such as max/min height etc.

								// Update this instances' transform in the UInstancedStaticMeshComponent
								if( MeshInfo.InstanceClusters.IsValidIndex(Instance.ClusterIndex) )
								{
									FFoliageInstanceCluster& Cluster = MeshInfo.InstanceClusters[Instance.ClusterIndex];

									Cluster.ClusterComponent->Modify();
									
									int32 ClusterInstanceDataIndex = Cluster.InstanceIndices.Find(InstanceIndex);
									
									check(ClusterInstanceDataIndex != INDEX_NONE);					
									check(Cluster.ClusterComponent->PerInstanceSMData.IsValidIndex(ClusterInstanceDataIndex));
									Cluster.ClusterComponent->MarkRenderStateDirty();
									
									// Update bounds
									FMatrix InstanceTransform = Instance.GetInstanceTransform();
									Cluster.Bounds = Cluster.Bounds + Cluster.ClusterComponent->StaticMesh->GetBounds().TransformBy(InstanceTransform);
									
									// Update transform in InstancedStaticMeshComponent
									Cluster.ClusterComponent->PerInstanceSMData[ClusterInstanceDataIndex].Transform = InstanceTransform;
									Cluster.ClusterComponent->InvalidateLightingCache();
								}

								// Re-add the new instance location to the hash
								MeshInfo.InstanceHash->InsertInstance(Instance.Location, InstanceIndex);
							}
							break;
						}
					}

					if( !bFoundHit )
					{
						// Couldn't find new spot - remove instance
						InstancesToRemove.Add(InstanceIndex);
					}
				}
			}

			// Remove any unused instances
			MeshInfo.RemoveInstances(this, InstancesToRemove);
		}
	}
}

void AInstancedFoliageActor::MoveInstancesForMovedComponent( class UActorComponent* InComponent )
{
	bool bUpdatedInstances = false;
	bool bFirst = true;

	for( TMap<class UStaticMesh*, struct FFoliageMeshInfo>::TIterator MeshIt(FoliageMeshes); MeshIt; ++MeshIt )
	{
		FFoliageMeshInfo& MeshInfo = MeshIt.Value();
		FFoliageComponentHashInfo* ComponentHashInfo = MeshInfo.ComponentHash.Find(InComponent);
		if( ComponentHashInfo )
		{
			if( bFirst )
			{
				bFirst = false;
				Modify();
			}

			FVector OldLocation = ComponentHashInfo->CachedLocation;
			FRotator OldRotation = ComponentHashInfo->CachedRotation;
			FVector OldDrawScale = ComponentHashInfo->CachedDrawScale;
			ComponentHashInfo->UpdateLocationFromActor(InComponent);

			for( TSet<int32>::TConstIterator InstIt(ComponentHashInfo->Instances); InstIt; ++InstIt )
			{
				int32 InstanceIndex = *InstIt;
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
				if( MeshInfo.InstanceClusters.IsValidIndex(Instance.ClusterIndex) )
				{
					FFoliageInstanceCluster& Cluster = MeshInfo.InstanceClusters[Instance.ClusterIndex];
					Cluster.ClusterComponent->Modify();
					int32 ClusterInstanceDataIndex = Cluster.InstanceIndices.Find(InstanceIndex);
					check(ClusterInstanceDataIndex != INDEX_NONE);					
					check(Cluster.ClusterComponent->PerInstanceSMData.IsValidIndex(ClusterInstanceDataIndex));
					Cluster.ClusterComponent->MarkRenderStateDirty();
					bUpdatedInstances = true;
					// Update bounds
					FMatrix InstanceTransform = Instance.GetInstanceTransform();
					Cluster.Bounds = Cluster.Bounds + Cluster.ClusterComponent->StaticMesh->GetBounds().TransformBy(InstanceTransform);
					// Update transform in InstancedStaticMeshComponent
					Cluster.ClusterComponent->PerInstanceSMData[ClusterInstanceDataIndex].Transform = InstanceTransform;
					Cluster.ClusterComponent->InvalidateLightingCache();
				}

				// Re-add the new instance location to the hash
				MeshInfo.InstanceHash->InsertInstance(Instance.Location, InstanceIndex);
			}	
		}
	}
}

void AInstancedFoliageActor::DeleteInstancesForComponent( class UActorComponent* InComponent )
{
	for( TMap<class UStaticMesh*, struct FFoliageMeshInfo>::TIterator MeshIt(FoliageMeshes); MeshIt; ++MeshIt )
	{
		FFoliageMeshInfo& MeshInfo = MeshIt.Value();
		const FFoliageComponentHashInfo* ComponentHashInfo = MeshInfo.ComponentHash.Find(InComponent);
		if( ComponentHashInfo )
		{
			MeshInfo.RemoveInstances(this, ComponentHashInfo->Instances.Array());
		}
	}
}

void AInstancedFoliageActor::MoveInstancesForComponentToCurrentLevel( class UActorComponent* InComponent )
{
	AInstancedFoliageActor* NewIFA = AInstancedFoliageActor::GetInstancedFoliageActor(InComponent->GetWorld());

	for( TMap<class UStaticMesh*, struct FFoliageMeshInfo>::TIterator MeshIt(FoliageMeshes); MeshIt; ++MeshIt )
	{
		FFoliageMeshInfo& MeshInfo = MeshIt.Value();

		const FFoliageComponentHashInfo* ComponentHashInfo = MeshInfo.ComponentHash.Find(InComponent);
		if( ComponentHashInfo )
		{
			FFoliageMeshInfo* NewMeshInfo = NewIFA->FoliageMeshes.Find( MeshIt.Key() );
			if( !NewMeshInfo )
			{
				NewMeshInfo = NewIFA->AddMesh( MeshIt.Key() );
			}

			// Add the foliage to the new level
			for( TSet<int32>::TConstIterator InstIt(ComponentHashInfo->Instances); InstIt; ++InstIt )
			{
				int32 InstanceIndex = *InstIt;
				NewMeshInfo->AddInstance( NewIFA, MeshIt.Key(), MeshInfo.Instances[InstanceIndex] );
			}

			// Remove from old level
			MeshInfo.RemoveInstances(this, ComponentHashInfo->Instances.Array());
		}
	}
}

void AInstancedFoliageActor::MoveInstancesToNewComponent( class UActorComponent* InOldComponent, class UActorComponent* InNewComponent )
{
	AInstancedFoliageActor* IFA = AInstancedFoliageActor::GetInstancedFoliageActor(InOldComponent->GetWorld());
	
	for( TMap<class UStaticMesh*, struct FFoliageMeshInfo>::TIterator MeshIt(FoliageMeshes); MeshIt; ++MeshIt )
	{
		FFoliageMeshInfo& MeshInfo = MeshIt.Value();

		const FFoliageComponentHashInfo* ComponentHashInfo = MeshInfo.ComponentHash.Find(InOldComponent);
		if( ComponentHashInfo )
		{
			// Update the instances
			for( auto InstanceIt = ComponentHashInfo->Instances.CreateConstIterator(); InstanceIt; ++InstanceIt )
			{
				MeshInfo.Instances[*InstanceIt].Base = InNewComponent;
			}

			// Update the hash
			MeshInfo.ComponentHash.Add(InNewComponent, *ComponentHashInfo);
			MeshInfo.ComponentHash.Remove(InOldComponent);
		}
	}
}

TMap<class UStaticMesh*,TArray<const FFoliageInstancePlacementInfo*> > AInstancedFoliageActor::GetInstancesForComponent( class UActorComponent* InComponent )
{
	TMap<class UStaticMesh*,TArray<const struct FFoliageInstancePlacementInfo*> > Result;

	for( TMap<class UStaticMesh*, struct FFoliageMeshInfo>::TConstIterator MeshIt(FoliageMeshes); MeshIt; ++MeshIt )
	{
		const FFoliageMeshInfo& MeshInfo = MeshIt.Value();

		const FFoliageComponentHashInfo* ComponentHashInfo = MeshInfo.ComponentHash.Find(InComponent);
		if( ComponentHashInfo )
		{
			TArray<const FFoliageInstancePlacementInfo*>& Array = Result.Add(MeshIt.Key(),TArray<const FFoliageInstancePlacementInfo*>() );
			Array.Empty(ComponentHashInfo->Instances.Num());

			for( TSet<int32>::TConstIterator InstIt(ComponentHashInfo->Instances); InstIt; ++InstIt )
			{
				int32 InstanceIndex = *InstIt;
				const FFoliageInstancePlacementInfo* Instance = &MeshInfo.Instances[InstanceIndex];
				Array.Add(Instance);
			}
		}
	}

	return Result;
}

struct FFoliageMeshInfo* AInstancedFoliageActor::AddMesh( class UStaticMesh* InMesh )
{
	check( FoliageMeshes.Find(InMesh) == NULL );

	MarkPackageDirty();

	int32 MaxDisplayOrder = 0;
	for( TMap<class UStaticMesh*, struct FFoliageMeshInfo>::TIterator It(FoliageMeshes); It; ++It )
	{
		if( It.Value().Settings->DisplayOrder > MaxDisplayOrder )
		{
			MaxDisplayOrder = It.Value().Settings->DisplayOrder;
		}
	}

	FFoliageMeshInfo& MeshInfo = FoliageMeshes.Add(InMesh, FFoliageMeshInfo());
#if WITH_EDITORONLY_DATA
	UInstancedFoliageSettings* DefaultSettings = InMesh->FoliageDefaultSettings;
	if( DefaultSettings != NULL )
	{
		MeshInfo.Settings = Cast<UInstancedFoliageSettings>(StaticDuplicateObject(DefaultSettings,this,TEXT("None")));
	}
	else
#endif
	{
		MeshInfo.Settings = ConstructObject<UInstancedFoliageSettings>(UInstancedFoliageSettings::StaticClass(), this);
	}

	const FBoxSphereBounds MeshBounds = InMesh->GetBounds();

	// Set IsPendingKill() to true so that when the initial undo record is made,
	// the component will be treated as destroyed, in that undo an add will
	// actually work
	MeshInfo.Settings->SetFlags(RF_PendingKill);
	MeshInfo.Settings->Modify( false );
	MeshInfo.Settings->ClearFlags(RF_PendingKill);
	MeshInfo.Settings->IsSelected = true;
	MeshInfo.Settings->DisplayOrder = MaxDisplayOrder+1;
	MeshInfo.Settings->MeshBounds = MeshBounds;

	// Make bottom only bound
	FBox LowBound = MeshBounds.GetBox();
	LowBound.Max.Z = LowBound.Min.Z + (LowBound.Max.Z - LowBound.Min.Z) * 0.1f;

	float MinX = FLT_MAX, MaxX = FLT_MIN, MinY = FLT_MAX, MaxY = FLT_MIN;
	MeshInfo.Settings->LowBoundOriginRadius = FVector::ZeroVector;

	if (InMesh->RenderData)
	{
		FPositionVertexBuffer& PositionVertexBuffer = InMesh->RenderData->LODResources[0].PositionVertexBuffer;
		for( uint32 Index = 0; Index < PositionVertexBuffer.GetNumVertices(); ++Index )
		{
			const FVector& Pos = PositionVertexBuffer.VertexPosition( Index );
			if (Pos.Z < LowBound.Max.Z)
			{
				MinX = FMath::Min(MinX, Pos.X);
				MinY = FMath::Min(MinY, Pos.Y);
				MaxX = FMath::Max(MaxX, Pos.X);
				MaxY = FMath::Max(MaxY, Pos.Y);
			}
		}
	}

	MeshInfo.Settings->LowBoundOriginRadius = FVector((MinX+MaxX), (MinY+MaxY), FMath::Sqrt( FMath::Square(MaxX-MinX) + FMath::Square(MaxY-MinY)) ) * 0.5f;

	return &MeshInfo;
}

void AInstancedFoliageActor::RemoveMesh( class UStaticMesh* InMesh )
{
	Modify();
	MarkPackageDirty();
	UnregisterAllComponents();

	// Remove all components for this mesh from the Components array.
	FFoliageMeshInfo* MeshInfo =  FoliageMeshes.Find(InMesh);
	if( MeshInfo )
	{
		for( int32 ClusterIndex=0;ClusterIndex < MeshInfo->InstanceClusters.Num(); ClusterIndex++ )
		{
			UInstancedStaticMeshComponent* Component = MeshInfo->InstanceClusters[ClusterIndex].ClusterComponent;
			if( Component != NULL )
			{
				Component->bAutoRegister = false;
			}
		}
	}

	FoliageMeshes.Remove(InMesh);
	RegisterAllComponents();
	CheckSelection();
}

void AInstancedFoliageActor::SelectInstance( class UInstancedStaticMeshComponent* InComponent, int32 InComponentInstanceIndex, bool bToggle )
{
	bool bNeedsUpdate = false;

	Modify();

	// If we're not toggling, we need to first deselect everything else
	if( !bToggle )
	{
		for( TMap<class UStaticMesh*, struct FFoliageMeshInfo>::TIterator MeshIt(FoliageMeshes); MeshIt; ++MeshIt )
		{
			FFoliageMeshInfo& Mesh = MeshIt.Value();

			if( Mesh.SelectedIndices.Num() > 0 )
			{
				for( int32 ClusterIdx=0;ClusterIdx<Mesh.InstanceClusters.Num();ClusterIdx++ )
				{
					FFoliageInstanceCluster& Cluster = Mesh.InstanceClusters[ClusterIdx];
					if( Cluster.ClusterComponent->SelectedInstances.Num() > 0 )
					{
						Cluster.ClusterComponent->Modify();
						Cluster.ClusterComponent->SelectedInstances.Empty();
						Cluster.ClusterComponent->MarkRenderStateDirty();
					}
				}
				bNeedsUpdate = true;
			}

			Mesh.SelectedIndices.Empty();
		}
	}

	if( InComponent )
	{
		FFoliageMeshInfo* Mesh = FoliageMeshes.Find(InComponent->StaticMesh);

		if( Mesh )
		{
			for( int32 ClusterIdx=0;ClusterIdx<Mesh->InstanceClusters.Num();ClusterIdx++ )
			{
				FFoliageInstanceCluster& Cluster = Mesh->InstanceClusters[ClusterIdx];
				if( Cluster.ClusterComponent == InComponent )
				{
					InComponent->Modify();
					int32 InstanceIndex = Cluster.InstanceIndices[InComponentInstanceIndex];
					int32 SelectedIndex = Mesh->SelectedIndices.Find(InstanceIndex);

					bNeedsUpdate = true;

					// Deselect if it's already selected.
					if( InComponentInstanceIndex < InComponent->SelectedInstances.Num() )
					{
						InComponent->SelectedInstances[InComponentInstanceIndex] = false;
						InComponent->MarkRenderStateDirty();
					}
					if( SelectedIndex != INDEX_NONE )
					{
						Mesh->SelectedIndices.RemoveAt(SelectedIndex);
					}

					if( bToggle && SelectedIndex != INDEX_NONE)
					{
						if( SelectedMesh == InComponent->StaticMesh && Mesh->SelectedIndices.Num() == 0 )
						{
							SelectedMesh = NULL;
						}
					}
					else
					{
						// Add the selection
						if( InComponent->SelectedInstances.Num() < InComponent->PerInstanceSMData.Num() )
						{
							InComponent->SelectedInstances.Init(false, Cluster.InstanceIndices.Num());
						}
						InComponent->SelectedInstances[InComponentInstanceIndex] = true;
						InComponent->MarkRenderStateDirty();

						SelectedMesh = InComponent->StaticMesh;
						Mesh->SelectedIndices.Insert(InstanceIndex,0);
					}
					break;
				}
			}
		}
	}

	if( bNeedsUpdate )
	{
		// Update selection
		MarkComponentsRenderStateDirty();
	}

	CheckSelection();
}

void AInstancedFoliageActor::ApplySelectionToComponents( bool bApply )
{
	bool bNeedsUpdate = false;

	for( TMap<class UStaticMesh*, struct FFoliageMeshInfo>::TIterator MeshIt(FoliageMeshes); MeshIt; ++MeshIt )
	{
		FFoliageMeshInfo& Mesh = MeshIt.Value();

		if( bApply )
		{
			if( Mesh.SelectedIndices.Num() > 0 )
			{
				for( int32 ClusterIdx=0;ClusterIdx<Mesh.InstanceClusters.Num();ClusterIdx++ )
				{
					FFoliageInstanceCluster& Cluster = Mesh.InstanceClusters[ClusterIdx];

					// Apply any selections in the component
					Cluster.ClusterComponent->SelectedInstances.Init(false, Cluster.InstanceIndices.Num());
					Cluster.ClusterComponent->MarkRenderStateDirty();
					bNeedsUpdate = true;

					for( int32 ClusterInstanceIdx=0;ClusterInstanceIdx<Cluster.InstanceIndices.Num();ClusterInstanceIdx++ )
					{
						int32 InstanceIdx = Cluster.InstanceIndices[ClusterInstanceIdx];
						if( Mesh.SelectedIndices.Find(InstanceIdx) != INDEX_NONE )
						{
							Cluster.ClusterComponent->SelectedInstances[ClusterInstanceIdx] = true;
						}
					}
				}
			}
		}
		else
		{
			for( int32 ClusterIdx=0;ClusterIdx<Mesh.InstanceClusters.Num();ClusterIdx++ )
			{
				FFoliageInstanceCluster& Cluster = Mesh.InstanceClusters[ClusterIdx];

				if( Cluster.ClusterComponent->SelectedInstances.Num() > 0 )
				{
					// remove any selections in the component
					Cluster.ClusterComponent->SelectedInstances.Empty();
					Cluster.ClusterComponent->MarkRenderStateDirty();
					bNeedsUpdate = true;
				}
			}
		}
	}

	if( bNeedsUpdate )
	{
		// Update selection
		MarkComponentsRenderStateDirty();
	}
}

void AInstancedFoliageActor::CheckSelection()
{
	// Check if we have to change the selection.
	if( SelectedMesh != NULL )
	{
		FFoliageMeshInfo* Mesh = FoliageMeshes.Find(SelectedMesh);
		if( Mesh && Mesh->SelectedIndices.Num() > 0 )
		{
			return;
		}
	}

	SelectedMesh = NULL;

	// Try to find a new selection
	for( TMap<class UStaticMesh*, struct FFoliageMeshInfo>::TIterator MeshIt(FoliageMeshes); MeshIt; ++MeshIt )
	{
		FFoliageMeshInfo& Mesh = MeshIt.Value();
		if( Mesh.SelectedIndices.Num() > 0 )
		{
			SelectedMesh = MeshIt.Key();
			return;
		}
	}
}

FVector AInstancedFoliageActor::GetSelectionLocation()
{
	FVector Result(0,0,0);

	if( SelectedMesh != NULL )
	{
		FFoliageMeshInfo* Mesh = FoliageMeshes.Find(SelectedMesh);
		if( Mesh && Mesh->SelectedIndices.Num() > 0 )
		{
			Result = Mesh->Instances[Mesh->SelectedIndices[0]].Location;
		}
	}

	return Result;
}

void AInstancedFoliageActor::MapRebuild()
{
	// Map rebuilt - this may have modified the BSP components and thrown the previous ones away - if so we need to migrate the foliage across.
	UE_LOG(LogInstancedFoliage, Log, TEXT("Map Rebuilt - Update all BSP painted foliage!"));

	TMap< UStaticMesh*, TArray<FFoliageInstance> > NewInstances;
	TArray<UActorComponent*> RemovedComponents;
	UWorld* World = GetWorld();
	check(World);

	// For each foliage brush, represented by the mesh/info pair
	for( TMap<UStaticMesh*, FFoliageMeshInfo>::TConstIterator MeshIt(FoliageMeshes); MeshIt; ++MeshIt )
	{
		// each target component has some foliage instances
		FFoliageMeshInfo const& MeshInfo = MeshIt.Value();
		UStaticMesh* Mesh = MeshIt.Key();
		check(Mesh);
		
		for (TMap<UActorComponent*, FFoliageComponentHashInfo >::TConstIterator ComponentFoliageIt(MeshInfo.ComponentHash); ComponentFoliageIt; ++ComponentFoliageIt)
		{
			// BSP components are UModelComponents - they are the only ones we need to change
			UActorComponent* TargetComponent = ComponentFoliageIt.Key();
			if(!TargetComponent || TargetComponent->IsA(UModelComponent::StaticClass()))
			{
				// Delete it later
				RemovedComponents.Add(TargetComponent);

				FFoliageComponentHashInfo const& FoliageInfo = ComponentFoliageIt.Value();

				// We have to test each instance to see if we can migrate it across
				for( TSet<int32>::TConstIterator InstanceIt(FoliageInfo.Instances); InstanceIt; ++InstanceIt )
				{
					// Use a line test against the world, similar to FoliageEditMode.
					check(MeshInfo.Instances.IsValidIndex(*InstanceIt));
					FFoliageInstance const& Instance = MeshInfo.Instances[*InstanceIt];

					FFoliageInstance NewInstance = Instance;
					NewInstance.ClusterIndex = -1;
					
					FMatrix InstanceTransform(Instance.GetInstanceTransform());
					FVector Down(-FVector::UpVector);
					FVector Start(InstanceTransform.TransformPosition(FVector::UpVector));
					FVector End(InstanceTransform.TransformPosition(Down));

					FHitResult Result;
					bool bHit = World->LineTraceSingle(Result, Start, End, FCollisionQueryParams(true), FCollisionObjectQueryParams(ECC_WorldStatic));
					
					if(bHit && Result.Component.IsValid() && Result.Component->IsA(UModelComponent::StaticClass()))
					{
						NewInstance.Base = Cast<UActorComponent>(Result.Component.Get());
					}
					else
					{
						// Untargeted instances remain for manual deletion.
						NewInstance.Base = NULL;
					}

					NewInstances.FindOrAdd(Mesh).Add(NewInstance);
				}
			}
		}
	}

	// Remove all existing & broken instances & component references.
	for (TArray<UActorComponent*>::TConstIterator ComponentIt(RemovedComponents); ComponentIt; ++ComponentIt)
	{
		DeleteInstancesForComponent(*ComponentIt);
	}

	// And then finally add our new instances to the correct target components.
	for (TMap< UStaticMesh*, TArray<FFoliageInstance> >::TConstIterator NewInstanceIt(NewInstances); NewInstanceIt; ++NewInstanceIt)
	{
		UStaticMesh* Mesh = NewInstanceIt.Key();
		check(Mesh);
		FFoliageMeshInfo& MeshInfo = FoliageMeshes.FindOrAdd(Mesh);
		for(TArray<FFoliageInstance>::TConstIterator InstanceIt(NewInstanceIt.Value()); InstanceIt; ++InstanceIt)
		{
			MeshInfo.AddInstance(this, Mesh, *InstanceIt);
		}
	}
}

#endif

void AInstancedFoliageActor::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar << FoliageMeshes;
}

void AInstancedFoliageActor::PostLoad()
{
	Super::PostLoad();
	if( GIsEditor )
	{
		for( TMap<class UStaticMesh*, struct FFoliageMeshInfo>::TIterator MeshIt(FoliageMeshes); MeshIt; ++MeshIt )
		{
			// Remove any NULL entries.
			if( MeshIt.Key() == NULL )
			{
				FMessageLog("MapCheck").Warning()
					->AddToken(FUObjectToken::Create(this))
					->AddToken(FTextToken::Create(LOCTEXT("MapCheck_Message_FoliageMissingStaticMesh", "Foliage instances for a missing static mesh have been removed.")))
					->AddToken(FMapErrorToken::Create(FMapErrors::FoliageMissingStaticMesh));
				MeshIt.RemoveCurrent();
				continue;
			}

			// Find the per-mesh info matching the mesh.
			FFoliageMeshInfo& MeshInfo = MeshIt.Value();

			// Update the FreeInstanceIndices list and hash.
			for( int32 InstanceIdx=0; InstanceIdx<MeshInfo.Instances.Num(); InstanceIdx++ )
			{
				FFoliageInstance& Instance = MeshInfo.Instances[InstanceIdx];
				if( Instance.ClusterIndex == -1 )
				{
					// Add invalid instances to the FreeInstanceIndices list.
					MeshInfo.FreeInstanceIndices.Add(InstanceIdx);
					Instance.Base = NULL;
				}
				else
				{
					// Add valid instances to the hash.
					MeshInfo.InstanceHash->InsertInstance(Instance.Location, InstanceIdx);
					FFoliageComponentHashInfo* ComponentHashInfo = MeshInfo.ComponentHash.Find(Instance.Base);
					if( ComponentHashInfo == NULL )
					{
						ComponentHashInfo = &MeshInfo.ComponentHash.Add(Instance.Base, FFoliageComponentHashInfo(Instance.Base));
					}
					ComponentHashInfo->Instances.Add(InstanceIdx);
				}
			}

			// Check if any of the clusters are NULL
			int32 NumMissingComponents = 0;
			for( int32 ClusterIdx=0;ClusterIdx<MeshInfo.InstanceClusters.Num();ClusterIdx++ )
			{
				if( MeshInfo.InstanceClusters[ClusterIdx].ClusterComponent == NULL )
				{
					NumMissingComponents++;
				}
			}
			if( NumMissingComponents> 0 )
			{
				FFormatNamedArguments Arguments;
				Arguments.Add(TEXT("MissingCount"), NumMissingComponents);
				Arguments.Add(TEXT("MeshName"), FText::FromString(MeshIt.Key()->GetName()));
				FMessageLog("MapCheck").Warning()
					->AddToken(FUObjectToken::Create(this))
					->AddToken(FTextToken::Create(FText::Format(LOCTEXT("MapCheck_Message_FoliageMissingClusterComponent", "Foliage in this map is missing {MissingCount} cluster component(s) for static mesh {MeshName}. Opening the Foliage tool will this problem."), Arguments) ))
					->AddToken(FMapErrorToken::Create(FMapErrors::FoliageMissingClusterComponent));
			}
		}
	}
}

//
// Serialize all our UObjects for RTGC 
//
void AInstancedFoliageActor::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	AInstancedFoliageActor* This = CastChecked<AInstancedFoliageActor>(InThis);
	if( This->SelectedMesh )
	{
		Collector.AddReferencedObject( This->SelectedMesh, This );
	}

	for( TMap<class UStaticMesh*, struct FFoliageMeshInfo>::TIterator MeshIt(This->FoliageMeshes); MeshIt; ++MeshIt )
	{
		Collector.AddReferencedObject( MeshIt.Key(), This );
		FFoliageMeshInfo& MeshInfo = MeshIt.Value();

		if(MeshInfo.Settings != NULL )
		{
			Collector.AddReferencedObject( MeshInfo.Settings, This );
		}

		for( int32 InstanceIdx=0;InstanceIdx<MeshInfo.Instances.Num();InstanceIdx++ )
		{
			class UActorComponent* Base = MeshInfo.Instances[InstanceIdx].Base;
			if( Base != NULL )
			{
				Collector.AddReferencedObject( Base, This );
			}
		}

		for( int32 ClusterIdx=0;ClusterIdx<MeshInfo.InstanceClusters.Num();ClusterIdx++ )
		{
			Collector.AddReferencedObject( MeshInfo.InstanceClusters[ClusterIdx].ClusterComponent, This );	
		}
	}
	Super::AddReferencedObjects(This, Collector);
}

/** InstancedStaticMeshInstance hit proxy */
void HInstancedStaticMeshInstance::AddReferencedObjects( FReferenceCollector& Collector )
{
	Collector.AddReferencedObject( Component );
}

void AInstancedFoliageActor::ApplyWorldOffset(const FVector& InOffset, bool bWorldShift)
{
	TArray<UActorComponent*> Components;
	GetComponents(Components);

	for (int32 CompIndex = 0; CompIndex < Components.Num(); CompIndex++)
	{
		Components[CompIndex]->ApplyWorldOffset(InOffset, bWorldShift);
	}

	if (GIsEditor)
	{
		for (auto MeshIt = FoliageMeshes.CreateIterator(); MeshIt; ++MeshIt)
		{
			FFoliageMeshInfo& MeshInfo = MeshIt.Value();
			
			for (int32 ClusterIdx = 0; ClusterIdx < MeshInfo.InstanceClusters.Num(); ClusterIdx++)
			{
				FFoliageInstanceCluster& Cluster = MeshInfo.InstanceClusters[ClusterIdx];
				Cluster.Bounds.Origin+= InOffset;
			}
			
			MeshInfo.InstanceHash->Empty();
			for (int32 InstanceIdx = 0; InstanceIdx < MeshInfo.Instances.Num(); InstanceIdx++)
			{
				FFoliageInstance& Instance = MeshInfo.Instances[InstanceIdx];
				Instance.Location+= InOffset;
				// Rehash instance location
				MeshInfo.InstanceHash->InsertInstance(Instance.Location, InstanceIdx);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
