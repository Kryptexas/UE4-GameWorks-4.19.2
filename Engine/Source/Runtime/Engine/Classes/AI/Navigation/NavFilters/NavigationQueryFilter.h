// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NavigationQueryFilter.generated.h"

USTRUCT()
struct ENGINE_API FNavigationFilterArea
{
	GENERATED_USTRUCT_BODY()

	/** navigation area class */
	UPROPERTY(EditAnywhere, Category=Area)
	TSubclassOf<class UNavArea> AreaClass;

	/** override for travel cost */
	UPROPERTY(EditAnywhere, Category=Area, meta=(EditCondition="bOverrideTravelCost",ClampMin=1))
	float TravelCostOverride;

	/** override for entering cost */
	UPROPERTY(EditAnywhere, Category=Area, meta=(EditCondition="bOverrideEnteringCost",ClampMin=0))
	float EnteringCostOverride;

	/** mark as excluded */
	UPROPERTY(EditAnywhere, Category=Area)
	uint32 bIsExcluded : 1;

	UPROPERTY()
	uint32 bOverrideTravelCost : 1;

	UPROPERTY()
	uint32 bOverrideEnteringCost : 1;

	FNavigationFilterArea() : TravelCostOverride(1.0f) {}
};

// 
// Use UNavigationSystem.DescribeFilterFlags() to setup user friendly names of flags
// 
USTRUCT()
struct ENGINE_API FNavigationFilterFlags
{
	GENERATED_USTRUCT_BODY()

#if CPP
	union
	{
		struct
		{
#endif
	UPROPERTY(EditAnywhere, Category=Flags)
	uint32 bNavFlag0 : 1;
	UPROPERTY(EditAnywhere, Category=Flags)
	uint32 bNavFlag1 : 1;
	UPROPERTY(EditAnywhere, Category=Flags)
	uint32 bNavFlag2 : 1;
	UPROPERTY(EditAnywhere, Category=Flags)
	uint32 bNavFlag3 : 1;
	UPROPERTY(EditAnywhere, Category=Flags)
	uint32 bNavFlag4 : 1;
	UPROPERTY(EditAnywhere, Category=Flags)
	uint32 bNavFlag5 : 1;
	UPROPERTY(EditAnywhere, Category=Flags)
	uint32 bNavFlag6 : 1;
	UPROPERTY(EditAnywhere, Category=Flags)
	uint32 bNavFlag7 : 1;
	UPROPERTY(EditAnywhere, Category=Flags)
	uint32 bNavFlag8 : 1;
	UPROPERTY(EditAnywhere, Category=Flags)
	uint32 bNavFlag9 : 1;
	UPROPERTY(EditAnywhere, Category=Flags)
	uint32 bNavFlag10 : 1;
	UPROPERTY(EditAnywhere, Category=Flags)
	uint32 bNavFlag11 : 1;
	UPROPERTY(EditAnywhere, Category=Flags)
	uint32 bNavFlag12 : 1;
	UPROPERTY(EditAnywhere, Category=Flags)
	uint32 bNavFlag13 : 1;
	UPROPERTY(EditAnywhere, Category=Flags)
	uint32 bNavFlag14 : 1;
	UPROPERTY(EditAnywhere, Category=Flags)
	uint32 bNavFlag15 : 1;
#if CPP
		};
		uint16 Packed;
	};
#endif
};

/** Class containing definition of a navigation query filter */
UCLASS(Abstract, Blueprintable)
class ENGINE_API UNavigationQueryFilter : public UObject
{
	GENERATED_UCLASS_BODY()
	
	/** list of overrides for navigation areas */
	UPROPERTY(EditAnywhere, Category=Filter)
	TArray<FNavigationFilterArea> Areas;

	/** required flags of navigation nodes */
	UPROPERTY(EditAnywhere, Category=Filter)
	FNavigationFilterFlags IncludeFlags;

	/** forbidden flags of navigation nodes */
	UPROPERTY(EditAnywhere, Category=Filter)
	FNavigationFilterFlags ExcludeFlags;

	/** get filter for given navigation data and initialize on first access */
	TSharedPtr<const struct FNavigationQueryFilter> GetQueryFilter(const class ANavigationData* NavData) const;
	
	/** helper functions for accessing filter */
	static TSharedPtr<const struct FNavigationQueryFilter> GetQueryFilter(const class AAIController* AI, UClass* FilterClass);
	static TSharedPtr<const struct FNavigationQueryFilter> GetQueryFilter(const class ANavigationData* NavData, UClass* FilterClass);

	template<class T>
	static TSharedPtr<const struct FNavigationQueryFilter> GetQueryFilter(const class ANavigationData* NavData, UClass* FilterClass = T::StaticClass())
	{
		return GetQueryFilter(NavData, FilterClass);
	}

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif

protected:

	/** helper functions for adding area overrides */
	void AddTravelCostOverride(TSubclassOf<class UNavArea> AreaClass, float TravelCost);
	void AddEnteringCostOverride(TSubclassOf<class UNavArea> AreaClass, float EnteringCost);
	void AdExcludedArea(TSubclassOf<class UNavArea> AreaClass);

	/** find index of area data */
	int32 FindAreaOverride(TSubclassOf<class UNavArea> AreaClass) const;

	/** setup filter for given navigation data, use to create custom filters */
	virtual void InitializeFilter(const class ANavigationData* NavData, struct FNavigationQueryFilter* Filter) const;
};
