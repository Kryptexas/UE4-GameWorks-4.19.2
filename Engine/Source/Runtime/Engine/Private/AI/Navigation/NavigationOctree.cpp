// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "NavigationOctree.h"
#include "RecastHelpers.h"
#include "AI/Navigation/NavRelevantComponent.h"
#include "RecastNavMeshGenerator.h"

//----------------------------------------------------------------------//
// FNavigationOctree
//----------------------------------------------------------------------//
FNavigationOctree::FNavigationOctree(const FVector& Origin, float Radius)
	: TOctree<FNavigationOctreeElement, FNavigationOctreeSemantics>(Origin, Radius)
	, bPerformGeometryGatheringLazily(false)
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

void FNavigationOctree::SetLazyGeometryGatheringEnabled(bool bInEnable)
{
	bPerformGeometryGatheringLazily = bInEnable;
}

void FNavigationOctree::SetNavigableGeometryStoringMode(ENavGeometryStoringMode NavGeometryMode)
{
	bGatherGeometry = (NavGeometryMode == FNavigationOctree::StoreNavGeometry);
}

void FNavigationOctree::DemandGatheringGeometry(const FNavigationOctreeElement& Element) const
{
	UObject* ElementOb = Element.Owner.Get();
	if (ElementOb == nullptr || bGatherGeometry == false)
	{
		return;
	}
		
	UActorComponent* ActorComp = Cast<UActorComponent>(ElementOb);
	if (ActorComp)
	{
		FNavigationOctreeElement* MutableElement = const_cast<FNavigationOctreeElement*>(&Element);

		ComponentExportDelegate.ExecuteIfBound(ActorComp, MutableElement->Data);

		// shrink arrays before counting memory
		// it will be reallocated when adding to octree and RemoveNode will have different value returned by GetAllocatedSize()
		MutableElement->Shrink();

		// mark this element as no longer needing geometry gathering
		MutableElement->Data.bPendingLazyGeometryGathering = false;

		const int32 ElementMemory = MutableElement->Data.GetGeometryAllocatedSize();
		const_cast<FNavigationOctree*>(this)->NodesMemory += ElementMemory;
		INC_MEMORY_STAT_BY(STAT_Navigation_CollisionTreeMemory, ElementMemory);
	}
}

void FNavigationOctree::AddNode(UObject* ElementOb, INavRelevantInterface* NavElement, const FBox& Bounds, FNavigationOctreeElement& Element)
{
	// we assume NavElement is ElementOb already cast
	//check(ElementOb == NavElement);
	Element.Owner = ElementOb;
	Element.Bounds = Bounds;

	if (bGatherGeometry)
	{
		UActorComponent* ActorComp = Cast<UActorComponent>(ElementOb);
		if (ActorComp && (bPerformGeometryGatheringLazily == false))
		{
			ComponentExportDelegate.ExecuteIfBound(ActorComp, Element.Data);
		}
		else
		{
			Element.Data.bPendingLazyGeometryGathering = true;
		}
	}

	if (NavElement)
	{
		SCOPE_CYCLE_COUNTER(STAT_Navigation_GatheringNavigationModifiersSync);
		NavElement->GetNavigationData(Element.Data);
	}

	// shrink arrays before counting memory
	// it will be reallocated when adding to octree and RemoveNode will have different value returned by GetAllocatedSize()
	Element.Shrink();

	const int32 ElementMemory = Element.GetAllocatedSize();
	NodesMemory += ElementMemory;
	INC_MEMORY_STAT_BY(STAT_Navigation_CollisionTreeMemory, ElementMemory);

	AddElement(Element);
}

void FNavigationOctree::AppendToNode(const FOctreeElementId& Id, INavRelevantInterface* NavElement, const FBox& Bounds, FNavigationOctreeElement& Data)
{
	FNavigationOctreeElement OrgData = GetElementById(Id);

	Data = OrgData;
	Data.Bounds = Bounds + OrgData.Bounds.GetBox();

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

	RemoveElement(Id);
	AddElement(Data);
}

void FNavigationOctree::UpdateNode(const FOctreeElementId& Id, const FBox& NewBounds)
{
	FNavigationOctreeElement ElementCopy = GetElementById(Id);
	RemoveElement(Id);
	ElementCopy.Bounds = NewBounds;
	AddElement(ElementCopy);
}

void FNavigationOctree::RemoveNode(const FOctreeElementId& Id)
{
	FNavigationOctreeElement& Data = GetElementById(Id);
	const int32 ElementMemory = Data.GetAllocatedSize();
	NodesMemory -= ElementMemory;
	DEC_MEMORY_STAT_BY(STAT_Navigation_CollisionTreeMemory, ElementMemory);

	RemoveElement(Id);
}

const FNavigationRelevantData* FNavigationOctree::GetDataForID(const FOctreeElementId& Id) const
{
	if (Id.IsValidId() == false)
	{
		return nullptr;
	}

	const FNavigationOctreeElement& OctreeElement = GetElementById(Id);

	return &OctreeElement.Data;
}

//----------------------------------------------------------------------//
// FNavigationRelevantData
//----------------------------------------------------------------------//

bool FNavigationRelevantData::HasPerInstanceTransforms() const
{
	return NavDataPerInstanceTransformDelegate.IsBound();
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

//----------------------------------------------------------------------//
// FNavigationOctreeSemantics
//----------------------------------------------------------------------//
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