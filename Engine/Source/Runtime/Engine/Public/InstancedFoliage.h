// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	InstancedFoliage.h: Instanced foliage type definitions.
=============================================================================*/

#pragma once

/**
 * Flags stored with each instance
 */
enum EFoliageInstanceFlags
{
	FOLIAGE_AlignToNormal				= 0x00000001,
	FOLIAGE_NoRandomYaw					= 0x00000002,
	FOLIAGE_Readjusted					= 0x00000004,
};

/**
 *	FFoliageInstancePlacementInfo - placement info an individual instance
 */
struct FFoliageInstancePlacementInfo
{
	FVector Location;
	FRotator Rotation;
	FRotator PreAlignRotation;
	FVector DrawScale3D;
	float ZOffset;
	uint32 Flags;

	FFoliageInstancePlacementInfo()
	:	Location(0.f,0.f,0.f)
	,	Rotation(0,0,0)
	,	PreAlignRotation(0,0,0)
	,	DrawScale3D(1.f,1.f,1.f)
	,	ZOffset(0.f)
	,	Flags(0)
	{}
};

/**
 *	FFoliageInstance - editor info an individual instance
 */
struct FFoliageInstance : public FFoliageInstancePlacementInfo
{
	class UActorComponent* Base;
	int32 ClusterIndex;	// -1 if this instance is invalid and its array slot can be reused.

	FFoliageInstance()
	:	Base(NULL)
	,	ClusterIndex(-1)
	{}


	friend FArchive& operator<<( FArchive& Ar, FFoliageInstance& Instance );

	FMatrix GetInstanceTransform() const
	{
		FMatrix InstanceTransform = FScaleMatrix(DrawScale3D);
		InstanceTransform *= FRotationMatrix(Rotation);
		InstanceTransform *= FTranslationMatrix(Location);
		check( !InstanceTransform.ContainsNaN() );
		return InstanceTransform;
	}

	void AlignToNormal( const FVector& InNormal, float AlignMaxAngle=0.f )
	{
		Flags |= FOLIAGE_AlignToNormal;

		FRotator AlignRotation = InNormal.Rotation();
		// Static meshes are authored along the vertical axis rather than the X axis, so we add 90 degrees to the static mesh's Pitch.
		AlignRotation.Pitch -= 90.f;
		// Clamp its value inside +/- one rotation
		AlignRotation.Pitch = FRotator::NormalizeAxis(AlignRotation.Pitch);

		// limit the maximum pitch angle if it's > 0.
		if( AlignMaxAngle > 0.f )
		{
			int32 MaxPitch = AlignMaxAngle;
			if( AlignRotation.Pitch > MaxPitch )
			{
				AlignRotation.Pitch = MaxPitch;
			}
			else
			if( AlignRotation.Pitch < -MaxPitch )
			{
				AlignRotation.Pitch = -MaxPitch;
			}								
		}

		PreAlignRotation = Rotation;
		Rotation = FRotator( FQuat(AlignRotation) * FQuat(Rotation) );
	}
};

/**
 *	FFoliageInstanceCluster - render info for a cluster of meshes
 */
struct FFoliageInstanceCluster
{
	UInstancedStaticMeshComponent* ClusterComponent;

	FBoxSphereBounds Bounds;
	TArray<int32> InstanceIndices;	// index into editor editor Instances array

	FFoliageInstanceCluster()
	{}
	FFoliageInstanceCluster(UInstancedStaticMeshComponent* InClusterComponent, const FBoxSphereBounds& InBounds)
	:	ClusterComponent(InClusterComponent)
	,	Bounds(InBounds)
	{}
	friend FArchive& operator<<( FArchive& Ar, FFoliageInstanceCluster& Cluster );
};

/** 
 * FFoliageComponentHashInfo
 * Cached instance list and component location info stored in the ComponentHash.
 * Used for moving quick updates after operations on components with foliage painted on them.
 */
struct FFoliageComponentHashInfo
{
	// tors
	FFoliageComponentHashInfo()
	:	CachedLocation(0,0,0)
	,	CachedRotation(0,0,0)
	,	CachedDrawScale(1,1,1)
	{}

	FFoliageComponentHashInfo( UActorComponent* InComponent )
	:	CachedLocation(0,0,0)
	,	CachedRotation(0,0,0)
	,	CachedDrawScale(1,1,1)
	{
		UpdateLocationFromActor(InComponent);
	}

	// Cache the location and rotation from the actor
	void UpdateLocationFromActor( UActorComponent* InComponent )
	{
		if( InComponent )
		{
			AActor* Owner = Cast<AActor>(InComponent->GetOuter());
			if( Owner )
			{
				const USceneComponent* RootComponent = Owner->GetRootComponent();
				if( RootComponent )
				{
					CachedLocation = RootComponent->RelativeLocation;
					CachedRotation = RootComponent->RelativeRotation;
					CachedDrawScale = RootComponent->RelativeScale3D;
				}
			}
		}
	}

	// serializer
	friend FArchive& operator<<( FArchive& Ar, FFoliageComponentHashInfo& ComponentHashInfo );

	FVector CachedLocation;
	FRotator CachedRotation;
	FVector CachedDrawScale;
	TSet<int32> Instances;
};

/**
 *	FFoliageMeshInfo - editor info for all matching foliage meshes
 */
struct FFoliageMeshInfo
{
	// Cluster allocation data and pointers to components
	TArray<FFoliageInstanceCluster> InstanceClusters;

	// Editor-only placed instances
	TArray<FFoliageInstance> Instances;
	
	// Transient, editor-only locality hash of instances
	struct FFoliageInstanceHash* InstanceHash;

	// Transient, editor-only set of instances per component
	TMap<class UActorComponent*, FFoliageComponentHashInfo > ComponentHash;

	// Transient, editor-only list of free instance slots.
	TArray<int32> FreeInstanceIndices;

	// Transient, editor-only list of selected instances.
	TArray<int32> SelectedIndices;

	// UI settings
	class UInstancedFoliageSettings* Settings;

	FFoliageMeshInfo();
	FFoliageMeshInfo( const FFoliageMeshInfo& Other );

	~FFoliageMeshInfo();

#if WITH_EDITOR
	ENGINE_API void AddInstance( class AInstancedFoliageActor* InIFA, class UStaticMesh* InMesh, const FFoliageInstance& InNewInstance );
	ENGINE_API void RemoveInstances( class AInstancedFoliageActor* InIFA, const TArray<int32>& InInstancesToRemove );
	ENGINE_API void PreMoveInstances( class AInstancedFoliageActor* InIFA, const TArray<int32>& InInstancesToMove );
	ENGINE_API void PostMoveInstances( class AInstancedFoliageActor* InIFA, const TArray<int32>& InInstancesMoved );
	ENGINE_API void PostUpdateInstances( class AInstancedFoliageActor* InIFA, const TArray<int32>& InInstancesUpdated, bool bReAddToHash=false );
	ENGINE_API void DuplicateInstances( class AInstancedFoliageActor* InIFA, class UStaticMesh* InMesh, const TArray<int32>& InInstancesToDuplicate );
	ENGINE_API void GetInstancesInsideSphere( const FSphere& Sphere, TArray<int32>& OutInstances );
	ENGINE_API bool CheckForOverlappingSphere( const FSphere& Sphere );
	ENGINE_API bool CheckForOverlappingInstanceExcluding( int32 TestInstanceIdx, float Radius, TSet<int32>& ExcludeInstances );

	// Destroy existing clusters and reassign all instances to new clusters
	ENGINE_API void ReallocateClusters( class AInstancedFoliageActor* InIFA, class UStaticMesh* InMesh );
	// Update settings in the clusters based on the current settings (eg culling distance, collision, ...)
	ENGINE_API void UpdateClusterSettings( class AInstancedFoliageActor* InIFA );
	ENGINE_API void SelectInstances( class AInstancedFoliageActor* InIFA, bool bSelect, TArray<int32>& Instances );

	// Get the number of placed instances
	ENGINE_API int32 GetInstanceCount() const;

	// For debugging. Validate state after editing.
	void CheckValid();
#endif

	friend FArchive& operator<<( FArchive& Ar, FFoliageMeshInfo& MeshInfo );
};

//
// FFoliageInstanceHash
//

#define FOLIAGE_HASH_CELL_BITS 9	// 512x512 grid

struct FFoliageInstanceHash
{
	int32 HashCellBits;
private:

	uint64 MakeKey( int32 CellX, int32 CellY )
	{
		return ((uint64)(*(uint32*)(&CellX)) << 32) | (*(uint32*)(&CellY) & 0xffffffff);
	}

	uint64 MakeKey( const FVector& Location )
	{
		return  MakeKey( FMath::Floor(Location.X) >> HashCellBits, FMath::Floor(Location.Y) >> HashCellBits );
	}

	// Locality map
	TMap<uint64, TSet<int32> > CellMap;
public:
	FFoliageInstanceHash( int32 InHashCellBits = FOLIAGE_HASH_CELL_BITS )
	:	HashCellBits(InHashCellBits)
	{}

	void InsertInstance(const FVector& InstanceLocation, int32 InstanceIndex)
	{
		uint64 Key = MakeKey(InstanceLocation);

		TSet<int32>& NewSet = CellMap.FindOrAdd(Key);
		NewSet.Add(InstanceIndex);
	}

	void RemoveInstance(const FVector& InstanceLocation, int32 InstanceIndex)
	{
		uint64 Key = MakeKey(InstanceLocation);

		TSet<int32>* Set = CellMap.Find(Key);
		check( Set != NULL );
		int32 RemoveCount = Set->Remove(InstanceIndex);
		check( RemoveCount == 1 );
	}

	void GetInstancesOverlappingBox( const FBox& InBox, TSet<int32>& OutInstanceIndices )
	{
		int32 MinX = FMath::Floor(InBox.Min.X) >> HashCellBits;
		int32 MinY = FMath::Floor(InBox.Min.Y) >> HashCellBits;
		int32 MaxX = FMath::Floor(InBox.Max.X) >> HashCellBits;
		int32 MaxY = FMath::Floor(InBox.Max.Y) >> HashCellBits;

		for( int32 y=MinY;y<=MaxY;y++ )
		{
			for( int32 x=MinX;x<=MaxX;x++ )
			{
				uint64 Key = MakeKey(x,y);
				TSet<int32>* Set = CellMap.Find(Key);
				if( Set != NULL )
				{
					OutInstanceIndices.Append( *Set );
				}
			}
		}
	}

#if UE_BUILD_DEBUG
	void CheckInstanceCount(int32 InCount)
	{
		int32 HashCount = 0;
		for( TMap<uint64, TSet<int32> >::TConstIterator It(CellMap); It; ++It )
		{
			HashCount += It.Value().Num();
		}	
		check( HashCount == InCount );
	}
#endif

	void Empty()
	{
		CellMap.Empty();
	}

	friend FArchive& operator<<( FArchive& Ar, FFoliageInstanceHash& Hash )
	{
		Ar << Hash.CellMap;
		return Ar;
	}
};


/** InstancedStaticMeshInstance hit proxy */
struct HInstancedStaticMeshInstance : public HHitProxy
{
	UInstancedStaticMeshComponent* Component;
	int32 InstanceIndex;

	DECLARE_HIT_PROXY( ENGINE_API );
	HInstancedStaticMeshInstance(UInstancedStaticMeshComponent* InComponent, int32 InInstanceIndex): HHitProxy(HPP_World), Component(InComponent), InstanceIndex(InInstanceIndex) {}

	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE;

	virtual EMouseCursor::Type GetMouseCursor()
	{
		return EMouseCursor::CardinalCross;
	}
};
