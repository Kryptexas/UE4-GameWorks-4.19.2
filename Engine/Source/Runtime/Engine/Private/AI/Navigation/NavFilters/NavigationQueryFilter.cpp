// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
// @todo AIModule circular dependency
#include "Navigation/NavigationComponent.h"
#include "AI/Navigation/NavFilters/NavigationQueryFilter.h"

UNavigationQueryFilter::UNavigationQueryFilter(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	IncludeFlags.Packed = 0xffff;
	ExcludeFlags.Packed = 0;
}

TSharedPtr<const FNavigationQueryFilter> UNavigationQueryFilter::GetQueryFilter(const ANavigationData* NavData) const
{
	TSharedPtr<const FNavigationQueryFilter> SharedFilter = NavData->GetQueryFilter(GetClass());
	if (!SharedFilter.IsValid())
	{
		FNavigationQueryFilter* NavFilter = new FNavigationQueryFilter();
		NavFilter->SetFilterImplementation(NavData->GetDefaultQueryFilterImpl());

		InitializeFilter(NavData, NavFilter);

		SharedFilter = MakeShareable(NavFilter);
		((ANavigationData*)NavData)->StoreQueryFilter(GetClass(), SharedFilter);
	}

	return SharedFilter;
}

void UNavigationQueryFilter::InitializeFilter(const ANavigationData* NavData, FNavigationQueryFilter* Filter) const
{
	// apply overrides
	for (int32 i = 0; i < Areas.Num(); i++)
	{
		const FNavigationFilterArea& AreaData = Areas[i];
		
		const int32 AreaId = NavData->GetAreaID(AreaData.AreaClass);
		if (AreaId == INDEX_NONE)
		{
			UE_LOG(LogNavigation, Warning, TEXT("Can't find area '%s' in '%s' for filter: %s"),
				*GetNameSafe(AreaData.AreaClass), *GetNameSafe(NavData), *GetName());

			continue;
		}

		if (AreaData.bIsExcluded)
		{
			Filter->SetExcludedArea(AreaId);
		}
		else
		{
			if (AreaData.bOverrideTravelCost)
			{
				Filter->SetAreaCost(AreaId, FMath::Max(1.0f, AreaData.TravelCostOverride));
			}

			if (AreaData.bOverrideEnteringCost)
			{
				Filter->SetFixedAreaEnteringCost(AreaId, FMath::Max(0.0f, AreaData.EnteringCostOverride));
			}
		}
	}

	// apply flags
	Filter->SetIncludeFlags(IncludeFlags.Packed);
	Filter->SetExcludeFlags(ExcludeFlags.Packed);
}

TSharedPtr<const struct FNavigationQueryFilter> UNavigationQueryFilter::GetQueryFilter(const ANavigationData* NavData, UClass* FilterClass)
{
	UNavigationQueryFilter* DefFilterOb = FilterClass ? FilterClass->GetDefaultObject<UNavigationQueryFilter>() : NULL;
	return NavData && DefFilterOb ? DefFilterOb->GetQueryFilter(NavData) : NULL;
}

void UNavigationQueryFilter::AddTravelCostOverride(TSubclassOf<class UNavArea> AreaClass, float TravelCost)
{
	int32 Idx = FindAreaOverride(AreaClass);
	if (Idx == INDEX_NONE)
	{
		FNavigationFilterArea AreaData;
		AreaData.AreaClass = AreaClass;

		Idx = Areas.Add(AreaData);
	}

	Areas[Idx].bOverrideTravelCost = true;
	Areas[Idx].TravelCostOverride = TravelCost;
}

void UNavigationQueryFilter::AddEnteringCostOverride(TSubclassOf<class UNavArea> AreaClass, float EnteringCost)
{
	int32 Idx = FindAreaOverride(AreaClass);
	if (Idx == INDEX_NONE)
	{
		FNavigationFilterArea AreaData;
		AreaData.AreaClass = AreaClass;

		Idx = Areas.Add(AreaData);
	}

	Areas[Idx].bOverrideEnteringCost = true;
	Areas[Idx].EnteringCostOverride = EnteringCost;
}

void UNavigationQueryFilter::AdExcludedArea(TSubclassOf<class UNavArea> AreaClass)
{
	int32 Idx = FindAreaOverride(AreaClass);
	if (Idx == INDEX_NONE)
	{
		FNavigationFilterArea AreaData;
		AreaData.AreaClass = AreaClass;

		Idx = Areas.Add(AreaData);
	}

	Areas[Idx].bIsExcluded = true;
}

int32 UNavigationQueryFilter::FindAreaOverride(TSubclassOf<class UNavArea> AreaClass) const
{
	for (int32 i = 0; i < Areas.Num(); i++)
	{
		if (Areas[i].AreaClass == AreaClass)
		{
			return i;
		}
	}

	return INDEX_NONE;
}

#if WITH_EDITOR
void UNavigationQueryFilter::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// remove cached filter settings from existing NavigationSystems
	const TArray<FWorldContext>& Worlds = GEngine->GetWorldContexts();
	for (int32 i = 0; i < Worlds.Num(); i++)
	{
		UNavigationSystem* NavSys = UNavigationSystem::GetCurrent(Worlds[i].World());
		if (NavSys)
		{
			NavSys->ResetCachedFilter(GetClass());
		}
	}
}
#endif