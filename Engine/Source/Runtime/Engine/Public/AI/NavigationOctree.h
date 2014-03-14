// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GenericOctree.h"

struct ENGINE_API FNavigationOctreeFilter
{
	/** pass when actor has geometry */
	uint32 bIncludeGeometry : 1;
	/** pass when actor has any offmesh link modifier */
	uint32 bIncludeOffmeshLinks : 1;
	/** pass when actor has any area modifier */
	uint32 bIncludeAreas : 1;
	/** pass when actor has any modifier with meta area */
	uint32 bIncludeMetaAreas : 1;

	FNavigationOctreeFilter() :
		bIncludeGeometry(false), bIncludeOffmeshLinks(false), bIncludeAreas(false), bIncludeMetaAreas(false)
	{}
};

// @todo consider optional structures that can contain a delegate instead of 
// actual copy of collision data
struct ENGINE_API FNavigationRelevantData
{
	DECLARE_DELEGATE_RetVal_OneParam(bool, FFilterNavDataDelegate, const struct FNavDataConfig*);

	/** exported geometry (used by recast navmesh as FRecastGeometryCache) */
	TNavStatArray<uint8> CollisionData;

	/** cached voxels (used by recast navmesh as FRecastVoxelCache) */
	TNavStatArray<uint8> VoxelData;

	/** bounds of geometry (unreal coords) */
	FBox Bounds;

	/** called to check if hosted geometry should be used for given FNavDataConfig. If not set then "true" is assumed. */
	FFilterNavDataDelegate ShouldUseGeometryDelegate;

	/** additional modifiers: areas and external links */
	FCompositeNavModifier Modifiers;

	FORCEINLINE bool HasGeometry() const { return VoxelData.Num() || CollisionData.Num(); }
	FORCEINLINE bool HasModifiers() const { return !Modifiers.IsEmpty(); }
	FORCEINLINE bool IsEmpty() const { return !HasGeometry() && !HasModifiers(); }
	FORCEINLINE uint32 GetAllocatedSize() const { return CollisionData.GetAllocatedSize() + VoxelData.GetAllocatedSize() + Modifiers.GetAllocatedSize(); }
	FORCEINLINE int32 GetDirtyFlag() const { return (HasGeometry() ? ENavigationDirtyFlag::Geometry : 0) | (HasModifiers() ? ENavigationDirtyFlag::DynamicModifier : 0); }

	bool IsMatchingFilter(const FNavigationOctreeFilter& Filter) const;
};

struct ENGINE_API FNavigationOctreeElement
{
	FBoxSphereBounds Bounds;
	TWeakObjectPtr<UObject> Owner;

#if NAVOCTREE_CONTAINS_COLLISION_DATA
	FNavigationRelevantData Data;
#endif // NAVOCTREE_CONTAINS_COLLISION_DATA

	FORCEINLINE bool IsEmpty() const
	{
		const FBox BBox = Bounds.GetBox();
#if NAVOCTREE_CONTAINS_COLLISION_DATA
		return Data.IsEmpty() && (BBox.IsValid == 0 || BBox.GetSize().IsNearlyZero());
#else
		return BBox.IsValid == 0 || BBox.GetSize().IsNearlyZero();
#endif // NAVOCTREE_CONTAINS_COLLISION_DATA
	}

	FORCEINLINE bool IsMatchingFilter(const FNavigationOctreeFilter& Filter) const
	{
#if NAVOCTREE_CONTAINS_COLLISION_DATA
		return Data.IsMatchingFilter(Filter);
#endif // NAVOCTREE_CONTAINS_COLLISION_DATA
		return false;
	}

#if NAVOCTREE_CONTAINS_COLLISION_DATA
	/** 
	 *	retrieves Modifier, if it doesn't contain any "Meta Navigation Areas". 
	 *	If it does then retrieves a copy with meta areas substituted with
	 *	appropriate non-meta areas, depending on NavAgent
	 */
	FORCEINLINE FCompositeNavModifier GetModifierForAgent(const struct FNavAgentProperties* NavAgent = NULL) const 
	{ 
		return Data.Modifiers.HasMetaAreas() ? Data.Modifiers.GetInstantiatedMetaModifier(NavAgent, Owner) : Data.Modifiers;
	}

	FORCEINLINE bool ShouldUseGeometry(const struct FNavDataConfig* NavConfig) const
	{ 
		return !Data.ShouldUseGeometryDelegate.IsBound() || Data.ShouldUseGeometryDelegate.Execute(NavConfig);
	}

	FORCEINLINE int32 GetAllocatedSize() const
	{
		return Data.GetAllocatedSize();
	}
#endif // NAVOCTREE_CONTAINS_COLLISION_DATA
};

struct FNavigationOctreeSemantics
{
	enum { MaxElementsPerLeaf = 16 };
	enum { MinInclusiveElementsPerNode = 7 };
	enum { MaxNodeDepth = 12 };

	typedef TInlineAllocator<MaxElementsPerLeaf> ElementAllocator;
	//typedef FDefaultAllocator ElementAllocator;

	FORCEINLINE static const FBoxSphereBounds& GetBoundingBox(const FNavigationOctreeElement& NavData)
	{
		return NavData.Bounds;
	}

	FORCEINLINE static bool AreElementsEqual(const FNavigationOctreeElement& A, const FNavigationOctreeElement& B)
	{
		return A.Owner == B.Owner;
	}

#if NAVSYS_DEBUG
	FORCENOINLINE 
#endif // NAVSYS_DEBUG
	static void SetElementId(const FNavigationOctreeElement& Element, FOctreeElementId Id)
	{
		UWorld* World = NULL;
		UObject* ElementOwner = Element.Owner.Get();

		if (AActor* Actor = Cast<AActor>(ElementOwner))
		{
			World = Actor->GetWorld();
		}
		else if (UNavigationProxy* Proxy = Cast<UNavigationProxy>(ElementOwner))
		{
			World = Proxy->MyOwner != NULL ? Proxy->MyOwner->GetWorld() : NULL;
		}
		else if (UActorComponent* AC = Cast<UActorComponent>(ElementOwner))
		{
			World = AC->GetWorld();
		}
		else if (ULevel* Level = Cast<ULevel>(ElementOwner))
		{
			World = Level->OwningWorld;
		}

		UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(World);
		if (NavSys)
		{
			NavSys->SetObjectsNavOctreeId(ElementOwner, Id);
		}
	}
};

class FNavigationOctree : public TOctree<FNavigationOctreeElement, FNavigationOctreeSemantics>
{
	typedef TOctree<FNavigationOctreeElement, FNavigationOctreeSemantics> Super;

public:
	DECLARE_DELEGATE_TwoParams(FNavigableGeometryExportDelegate, AActor&, FNavigationRelevantData&);
	FNavigableGeometryExportDelegate NavigableGeometryExportDelegate;

	DECLARE_DELEGATE_TwoParams(FNavigableGeometryComponentExportDelegate, UActorComponent&, FNavigationRelevantData&);
	FNavigableGeometryComponentExportDelegate NavigableGeometryComponentExportDelegate;

	enum ENavGeometryStoringMode {
		SkipNavGeometry,
		StoreNavGeometry,
	};

	FNavigationOctree(FVector Origin, float Radius);
	~FNavigationOctree();

	/** @param Data allow you to access data put into navoctree while not 
	 *	having to query navoctree about it just after adding */
	void AddActor(class AActor* Actor, FNavigationOctreeElement& Data);
	void AddComponent(class UActorComponent* ActorComp, const FBox& Bounds, FNavigationOctreeElement& Data);

	/** Add node and update memory stats */
	void AddNode(FNavigationOctreeElement& Data);

	/** Remove node and update memory stats */
	void RemoveNode(const FOctreeElementId* Id);

	void SetNavigableGeometryStoringMode(ENavGeometryStoringMode NavGeometryMode);

protected:

	uint32 bGatherGeometry : 1;
	uint32 NodesMemory;
};

template<>
FORCEINLINE void SetOctreeMemoryUsage(TOctree<FNavigationOctreeElement, FNavigationOctreeSemantics>* Octree, int32 NewSize)
{
	{
		DEC_DWORD_STAT_BY( STAT_NavigationMemory, Octree->TotalSizeBytes );
		DEC_DWORD_STAT_BY(STAT_Navigation_CollisionTreeMemory, Octree->TotalSizeBytes);
	}
	Octree->TotalSizeBytes = NewSize;
	{
		INC_DWORD_STAT_BY( STAT_NavigationMemory, NewSize );
		INC_DWORD_STAT_BY(STAT_Navigation_CollisionTreeMemory, NewSize);
	}
}
