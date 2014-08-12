// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "NavigationOctree.h"
#include "RecastHelpers.h"
#include "AI/Navigation/NavRelevantComponent.h"
#include "AI/Navigation/NavigationProxy.h"

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

void FNavigationOctree::AddActor(AActor& Actor, FNavigationOctreeElement& Data)
{
	if (Actor.IsPendingKill())
	{
		return;
	}

	INavRelevantActorInterface* NavRelevantActor = InterfaceCast<INavRelevantActorInterface>(&Actor);
	UNavigationProxy* ProxyOb = NavRelevantActor ? NavRelevantActor->GetNavigationProxy() : NULL;
	UObject* NodeOwner = ProxyOb ? (UObject*)ProxyOb : &Actor;

	Data.Owner = NodeOwner;

	const FBox BBox = Actor.GetComponentsBoundingBox();
	if (BBox.IsValid && !BBox.GetSize().IsNearlyZero())
	{
		Data.Bounds = BBox;
	
#if NAVOCTREE_CONTAINS_COLLISION_DATA
		bool bExportGeometry = true;

		{	
			SCOPE_CYCLE_COUNTER(STAT_Navigation_GatheringNavigationModifiersSync);
			if (NavRelevantActor)
			{
				bExportGeometry = NavRelevantActor->GetNavigationRelevantData(Data.Data);
			}
		}

		if (bExportGeometry)
		{
			SCOPE_CYCLE_COUNTER(STAT_Navigation_ActorsGeometryExportSync);
			NavigableGeometryExportDelegate.ExecuteIfBound(Actor, Data.Data);
		}
#endif // NAVOCTREE_CONTAINS_COLLISION_DATA

		AddNode(Data);
	}
}

void FNavigationOctree::AddComponent(UActorComponent& ActorComp, const FBox& Bounds, FNavigationOctreeElement& Data)
{
	if (ActorComp.IsPendingKill())
	{
		return;
	}

	Data.Owner = &ActorComp;
	Data.Bounds = Bounds;

#if NAVOCTREE_CONTAINS_COLLISION_DATA
	if (bGatherGeometry)
	{
		NavigableGeometryComponentExportDelegate.ExecuteIfBound(ActorComp, Data.Data);
	}

	UNavRelevantComponent* NavRelevantComponent = Cast<UNavRelevantComponent>(&ActorComp);
	if (NavRelevantComponent)
	{
		NavRelevantComponent->OnApplyModifiers(Data.Data.Modifiers);
	}
#endif // NAVOCTREE_CONTAINS_COLLISION_DATA

	AddNode(Data);
}

void FNavigationOctree::SetNavigableGeometryStoringMode(ENavGeometryStoringMode NavGeometryMode)
{
	bGatherGeometry = (NavGeometryMode == FNavigationOctree::StoreNavGeometry);
}

void FNavigationOctree::AddNode(FNavigationOctreeElement& Data)
{
#if NAVOCTREE_CONTAINS_COLLISION_DATA
	const int32 ElementMemory = Data.GetAllocatedSize();
	NodesMemory += ElementMemory;
	INC_MEMORY_STAT_BY(STAT_Navigation_CollisionTreeMemory, ElementMemory);
#endif

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