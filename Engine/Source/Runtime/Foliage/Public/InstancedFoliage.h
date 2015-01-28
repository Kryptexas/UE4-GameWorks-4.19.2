// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	InstancedFoliage.h: Instanced foliage type definitions.
  =============================================================================*/
#pragma once
#include "FoliageInstanceBase.h"

//
// Forward declarations.
//
class UHierarchicalInstancedStaticMeshComponent;
class AInstancedFoliageActor;
class UFoliageType;
struct FFoliageInstanceHash;

/**
 * Flags stored with each instance
 */
enum EFoliageInstanceFlags
{
	FOLIAGE_AlignToNormal	= 0x00000001,
	FOLIAGE_NoRandomYaw		= 0x00000002,
	FOLIAGE_Readjusted		= 0x00000004,
	FOLIAGE_InstanceDeleted	= 0x00000008,	// Used only for migration from pre-HierarchicalISM foliage.
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
		: Location(0.f, 0.f, 0.f)
		, Rotation(0, 0, 0)
		, PreAlignRotation(0, 0, 0)
		, DrawScale3D(1.f, 1.f, 1.f)
		, ZOffset(0.f)
		, Flags(0)
	{}
};

/**
 *	Legacy instance
 */
struct FFoliageInstance_Deprecated : public FFoliageInstancePlacementInfo
{
	UActorComponent* Base;
	FGuid ProceduralGuid;
	friend FArchive& operator<<(FArchive& Ar, FFoliageInstance_Deprecated& Instance);
};

/**
 *	FFoliageInstance - editor info an individual instance
 */
struct FFoliageInstance : public FFoliageInstancePlacementInfo
{
	// ID of base this instance was painted on
	FFoliageInstanceBaseId BaseId;

	FGuid ProceduralGuid;

	FFoliageInstance()
	: BaseId(0)
	{}
	
	friend FArchive& operator<<(FArchive& Ar, FFoliageInstance& Instance);

	FTransform GetInstanceWorldTransform() const
	{
		return FTransform(Rotation, Location, DrawScale3D);
	}

	void AlignToNormal(const FVector& InNormal, float AlignMaxAngle = 0.f)
	{
		Flags |= FOLIAGE_AlignToNormal;

		FRotator AlignRotation = InNormal.Rotation();
		// Static meshes are authored along the vertical axis rather than the X axis, so we add 90 degrees to the static mesh's Pitch.
		AlignRotation.Pitch -= 90.f;
		// Clamp its value inside +/- one rotation
		AlignRotation.Pitch = FRotator::NormalizeAxis(AlignRotation.Pitch);

		// limit the maximum pitch angle if it's > 0.
		if (AlignMaxAngle > 0.f)
		{
			int32 MaxPitch = AlignMaxAngle;
			if (AlignRotation.Pitch > MaxPitch)
			{
				AlignRotation.Pitch = MaxPitch;
			}
			else if (AlignRotation.Pitch < -MaxPitch)
			{
				AlignRotation.Pitch = -MaxPitch;
			}
		}

		PreAlignRotation = Rotation;
		Rotation = FRotator(FQuat(AlignRotation) * FQuat(Rotation));
	}
};

struct FFoliageMeshInfo_Deprecated
{
	UHierarchicalInstancedStaticMeshComponent* Component;

#if WITH_EDITORONLY_DATA
	// Allows us to detect if FoliageType was updated while this level wasn't loaded
	FGuid FoliageTypeUpdateGuid;

	// Editor-only placed instances
	TArray<FFoliageInstance_Deprecated> Instances;
#endif

	FFoliageMeshInfo_Deprecated()
		: Component(nullptr)
	{
	}

	friend FArchive& operator<<(FArchive& Ar, FFoliageMeshInfo_Deprecated& MeshInfo);
};

/**
 *	FFoliageMeshInfo - editor info for all matching foliage meshes
 */
struct FFoliageMeshInfo
{
	UHierarchicalInstancedStaticMeshComponent* Component;

#if WITH_EDITORONLY_DATA
	// Allows us to detect if FoliageType was updated while this level wasn't loaded
	FGuid FoliageTypeUpdateGuid;

	// Editor-only placed instances
	TArray<FFoliageInstance> Instances;

	// Transient, editor-only locality hash of instances
	TUniquePtr<FFoliageInstanceHash> InstanceHash;

	// Transient, editor-only set of instances per component
	TMap<FFoliageInstanceBaseId, TSet<int32>> ComponentHash;

	// Transient, editor-only list of selected instances.
	TSet<int32> SelectedIndices;
#endif

	FOLIAGE_API FFoliageMeshInfo();

	~FFoliageMeshInfo() // =default
	{ }

	FFoliageMeshInfo(FFoliageMeshInfo&& Other)
		// even VC++2013 doesn't support "=default" on move constructors
		: Component(Other.Component)
#if WITH_EDITORONLY_DATA
		, FoliageTypeUpdateGuid(MoveTemp(Other.FoliageTypeUpdateGuid))
		, Instances(MoveTemp(Other.Instances))
		, InstanceHash(MoveTemp(Other.InstanceHash))
		, ComponentHash(MoveTemp(Other.ComponentHash))
		, SelectedIndices(MoveTemp(Other.SelectedIndices))
#endif
	{ }

	FFoliageMeshInfo& operator=(FFoliageMeshInfo&& Other)
		// even VC++2013 doesn't support "=default" on move assignment
	{
		Component = Other.Component;
#if WITH_EDITORONLY_DATA
		FoliageTypeUpdateGuid = MoveTemp(Other.FoliageTypeUpdateGuid);
		Instances = MoveTemp(Other.Instances);
		InstanceHash = MoveTemp(Other.InstanceHash);
		ComponentHash = MoveTemp(Other.ComponentHash);
		SelectedIndices = MoveTemp(Other.SelectedIndices);
#endif

		return *this;
	}

#if WITH_EDITOR
	FOLIAGE_API void AddInstance(AInstancedFoliageActor* InIFA, const UFoliageType* InSettings, const FFoliageInstance& InNewInstance);
	FOLIAGE_API void AddInstance(AInstancedFoliageActor* InIFA, const UFoliageType* InSettings, const FFoliageInstance& InNewInstance, UActorComponent* InBaseComponent);
	FOLIAGE_API void RemoveInstances(AInstancedFoliageActor* InIFA, const TArray<int32>& InInstancesToRemove);
	FOLIAGE_API void PreMoveInstances(AInstancedFoliageActor* InIFA, const TArray<int32>& InInstancesToMove);
	FOLIAGE_API void PostMoveInstances(AInstancedFoliageActor* InIFA, const TArray<int32>& InInstancesMoved);
	FOLIAGE_API void PostUpdateInstances(AInstancedFoliageActor* InIFA, const TArray<int32>& InInstancesUpdated, bool bReAddToHash = false);
	FOLIAGE_API void DuplicateInstances(AInstancedFoliageActor* InIFA, UFoliageType* InSettings, const TArray<int32>& InInstancesToDuplicate);
	FOLIAGE_API void GetInstancesInsideSphere(const FSphere& Sphere, TArray<int32>& OutInstances);
	FOLIAGE_API bool CheckForOverlappingSphere(const FSphere& Sphere);
	FOLIAGE_API bool CheckForOverlappingInstanceExcluding(int32 TestInstanceIdx, float Radius, TSet<int32>& ExcludeInstances);
	
	// Destroy existing clusters and reassign all instances to new clusters
	FOLIAGE_API void ReallocateClusters(AInstancedFoliageActor* InIFA, UFoliageType* InSettings);

	FOLIAGE_API void ReapplyInstancesToComponent();

	FOLIAGE_API void SelectInstances(AInstancedFoliageActor* InIFA, bool bSelect, TArray<int32>& Instances);

	// Get the number of placed instances
	FOLIAGE_API int32 GetInstanceCount() const;

	FOLIAGE_API void AddToHash(int32 InstanceIdx);
	FOLIAGE_API void RemoveFromHash(int32 InstanceIdx);
		
	// For debugging. Validate state after editing.
	void CheckValid();
#endif

	friend FArchive& operator<<(FArchive& Ar, FFoliageMeshInfo& MeshInfo);

#if PLATFORM_COMPILER_HAS_DEFAULTED_FUNCTIONS
	FFoliageMeshInfo(const FFoliageMeshInfo& Other) = delete;
	const FFoliageMeshInfo& operator=(const FFoliageMeshInfo& Other) = delete;
#else
private:
	FFoliageMeshInfo(const FFoliageMeshInfo& Other);
	const FFoliageMeshInfo& operator=(const FFoliageMeshInfo& Other);
#endif
};

//
// FFoliageInstanceHash
//

#define FOLIAGE_HASH_CELL_BITS 9	// 512x512 grid

struct FFoliageInstanceHash
{
private:
	const int32 HashCellBits;
	TMultiMap<uint64, int32> CellMap;

	uint64 MakeKey(int32 CellX, int32 CellY)
	{
		return ((uint64)(*(uint32*)(&CellX)) << 32) | (*(uint32*)(&CellY) & 0xffffffff);
	}

	uint64 MakeKey(const FVector& Location)
	{
		return  MakeKey(FMath::FloorToInt(Location.X) >> HashCellBits, FMath::FloorToInt(Location.Y) >> HashCellBits);
	}

	// Locality map
	//TMap<uint64, TSet<int32>> CellMap;
public:
	FFoliageInstanceHash(int32 InHashCellBits = FOLIAGE_HASH_CELL_BITS)
	:	HashCellBits(InHashCellBits)
	{}

	void InsertInstance(const FVector& InstanceLocation, int32 InstanceIndex)
	{
		uint64 Key = MakeKey(InstanceLocation);

		CellMap.AddUnique(Key, InstanceIndex);
	}

	void RemoveInstance(const FVector& InstanceLocation, int32 InstanceIndex)
	{
		uint64 Key = MakeKey(InstanceLocation);
		
		int32 RemoveCount = CellMap.RemoveSingle(Key, InstanceIndex);
		check(RemoveCount == 1);
	}

	void GetInstancesOverlappingBox(const FBox& InBox, TArray<int32>& OutInstanceIndices)
	{
		int32 MinX = FMath::FloorToInt(InBox.Min.X) >> HashCellBits;
		int32 MinY = FMath::FloorToInt(InBox.Min.Y) >> HashCellBits;
		int32 MaxX = FMath::FloorToInt(InBox.Max.X) >> HashCellBits;
		int32 MaxY = FMath::FloorToInt(InBox.Max.Y) >> HashCellBits;

		for (int32 y = MinY; y <= MaxY; y++)
		{
			for (int32 x = MinX; x <= MaxX; x++)
			{
				uint64 Key = MakeKey(x, y);
				CellMap.MultiFind(Key, OutInstanceIndices);
			}
		}
	}

	TArray<int32> GetInstancesOverlappingBox(const FBox& InBox)
	{
		TArray<int32> Result;
		GetInstancesOverlappingBox(InBox, Result);
		return Result;
	}

#if UE_BUILD_DEBUG
	void CheckInstanceCount(int32 InCount)
	{
		check(CellMap.Num() == InCount);
	}
#endif

	void Empty()
	{
		CellMap.Empty();
	}

	friend FArchive& operator<<(FArchive& Ar, FFoliageInstanceHash& Hash)
	{
		if (Ar.UE4Ver() < VER_UE4_FOLIAGE_SETTINGS_TYPE)
		{
			Hash.CellMap.Reset();

			TMap<uint64, TSet<int32>> OldCellMap;
			Ar << OldCellMap;
			for (auto& CellPair : OldCellMap)
			{
				for (int32 Idx : CellPair.Value)
				{
					Hash.CellMap.AddUnique(CellPair.Key, Idx);
				}
			}
		}
		else
		{
			Ar << Hash.CellMap;
		}

		return Ar;
	}
};


/** This is kind of a hack, but is needed right now for backwards compat of code. We use it to describe the placement mode (procedural vs manual)*/
namespace EFoliagePlacementMode
{
	enum Type
	{
		Manual = 0,
		Procedural = 1,
	};

}

/** Used to define a vector along which we'd like to spawn an instance. */
struct FDesiredFoliageInstance
{
	FDesiredFoliageInstance()
	: FoliageType(nullptr)
	, MeshInfo(nullptr)
	, StartTrace(ForceInit)
	, EndTrace(ForceInit)
	, Rotation(ForceInit)
	, TraceRadius(0.f)
	, Age(0.f)
	, PlacementMode(EFoliagePlacementMode::Manual)
	{

	}

	FDesiredFoliageInstance(const FVector& InStartTrace, const FVector& InEndTrace, const float InTraceRadius = 0.f)
	: FoliageType(nullptr)
	, MeshInfo(nullptr)
	, StartTrace(InStartTrace)
	, EndTrace(InEndTrace)
	, Rotation(ForceInit)
	, TraceRadius(InTraceRadius)
	, Age(0.f)
	, PlacementMode(EFoliagePlacementMode::Manual)
	{
	}

	FGuid ProceduralGuid;
	const UFoliageType* FoliageType;
	FFoliageMeshInfo* MeshInfo;
	FVector StartTrace;
	FVector EndTrace;
	FQuat Rotation;
	float TraceRadius;
	float Age;
	EFoliagePlacementMode::Type PlacementMode;
};

// Struct to hold potential instances we've sampled
struct FOLIAGE_API FPotentialInstance
{
	FVector HitLocation;
	FVector HitNormal;
	UPrimitiveComponent* HitComponent;
	float HitWeight;
	FFoliageMeshInfo* MeshInfo;
	AInstancedFoliageActor* IFA;
	FDesiredFoliageInstance DesiredInstance;

	FPotentialInstance(FVector InHitLocation, FVector InHitNormal, UPrimitiveComponent* InHitComponent, float InHitWeight, FFoliageMeshInfo* MeshInfo = nullptr, AInstancedFoliageActor* InIFA = nullptr, const FDesiredFoliageInstance& InDesiredInstance = FDesiredFoliageInstance());
	bool PlaceInstance(const UFoliageType* Settings, FFoliageInstance& Inst, bool bSkipCollision = false);
};
