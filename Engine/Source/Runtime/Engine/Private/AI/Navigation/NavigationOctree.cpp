// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "NavigationOctree.h"
#include "RecastHelpers.h"
#include "AI/Navigation/NavRelevantComponent.h"

#if NAVOCTREE_CONTAINS_COLLISION_DATA
#include "RecastNavMeshGenerator.h"
#endif // NAVOCTREE_CONTAINS_COLLISION_DATA

//----------------------------------------------------------------------//
// FNavigationOctree
//----------------------------------------------------------------------//
FNavigationOctree::FNavigationOctree(FVector Origin, float Radius)
	: Super(Origin, Radius)
	, bGatherGeometry(false)
	, NodesMemory(0)
{
	INC_DWORD_STAT_BY( STAT_NavigationMemory, sizeof(*this) );
}

FNavigationOctree::~FNavigationOctree()
{
	DEC_DWORD_STAT_BY( STAT_NavigationMemory, sizeof(*this) );
	DEC_MEMORY_STAT_BY(STAT_Navigation_CollisionTreeMemory, NodesMemory);
}

void FNavigationOctree::SetNavigableGeometryStoringMode(ENavGeometryStoringMode NavGeometryMode)
{
	bGatherGeometry = (NavGeometryMode == FNavigationOctree::StoreNavGeometry);
}

void FNavigationOctree::AddNode(UObject* ElementOb, INavRelevantInterface* NavElement, const FBox& Bounds, FNavigationOctreeElement& Data)
{
	Data.Owner = ElementOb;
	Data.Bounds = Bounds;

#if NAVOCTREE_CONTAINS_COLLISION_DATA
	UActorComponent* ActorComp = Cast<UActorComponent>(ElementOb);
	if (bGatherGeometry && ActorComp)
	{
		ComponentExportDelegate.ExecuteIfBound(ActorComp, Data.Data);
	}

	if (NavElement)
	{
		SCOPE_CYCLE_COUNTER(STAT_Navigation_GatheringNavigationModifiersSync);
		NavElement->GetNavigationData(Data.Data);
	}

	// shrink arrays before counting memory
	// it will be reallocated when adding to octree and RemoveNode will have different value returned by GetAllocatedSize()
	Data.Shrink();

	const int32 ElementMemory = Data.GetAllocatedSize();
	NodesMemory += ElementMemory;
	INC_MEMORY_STAT_BY(STAT_Navigation_CollisionTreeMemory, ElementMemory);
#endif

	AddElement(Data);
}

void FNavigationOctree::AppendToNode(const FOctreeElementId& Id, INavRelevantInterface* NavElement, const FBox& Bounds, FNavigationOctreeElement& Data)
{
	FNavigationOctreeElement OrgData = GetElementById(Id);

	Data = OrgData;
	Data.Bounds = Bounds + OrgData.Bounds.GetBox();

#if NAVOCTREE_CONTAINS_COLLISION_DATA
	if (NavElement)
	{
		SCOPE_CYCLE_COUNTER(STAT_Navigation_GatheringNavigationModifiersSync);
		NavElement->GetNavigationData(Data.Data);
	}

	// shrink arrays before counting memory
	// it will be reallocated when adding to octree and RemoveNode will have different value returned by GetAllocatedSize()
	Data.Shrink();

	const int32 OrgElementMemory = OrgData.GetAllocatedSize();
	const int32 NewElementMemory = Data.GetAllocatedSize();
	const int32 MemoryDelta = NewElementMemory - OrgElementMemory;

	NodesMemory += MemoryDelta;
	INC_MEMORY_STAT_BY(STAT_Navigation_CollisionTreeMemory, MemoryDelta);
#endif

	RemoveElement(Id);
	AddElement(Data);
}

void FNavigationOctree::RemoveNode(const FOctreeElementId& Id)
{
#if NAVOCTREE_CONTAINS_COLLISION_DATA
	FNavigationOctreeElement& Data = GetElementById(Id);
	const int32 ElementMemory = Data.GetAllocatedSize();
	NodesMemory -= ElementMemory;
	DEC_MEMORY_STAT_BY(STAT_Navigation_CollisionTreeMemory, ElementMemory);
#endif

	RemoveElement(Id);
}

bool FNavigationRelevantData::IsMatchingFilter(const FNavigationOctreeFilter& Filter) const
{
	return (Filter.bIncludeGeometry && HasGeometry()) ||
		(Filter.bIncludeOffmeshLinks && (Modifiers.HasPotentialLinks() || Modifiers.HasLinks())) ||
		(Filter.bIncludeAreas && Modifiers.HasAreas()) ||
		(Filter.bIncludeMetaAreas && Modifiers.HasMetaAreas());
}

void FNavigationRelevantData::Shrink()
{
	CollisionData.Shrink();
	VoxelData.Shrink();
	Modifiers.Shrink();
}

#if NAVSYS_DEBUG
FORCENOINLINE
#endif // NAVSYS_DEBUG
void FNavigationOctreeSemantics::SetElementId(const FNavigationOctreeElement& Element, FOctreeElementId Id)
{
	UWorld* World = NULL;
	UObject* ElementOwner = Element.Owner.Get();

	if (AActor* Actor = Cast<AActor>(ElementOwner))
	{
		World = Actor->GetWorld();
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