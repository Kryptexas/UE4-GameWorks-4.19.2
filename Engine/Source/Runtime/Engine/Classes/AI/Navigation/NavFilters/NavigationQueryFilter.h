// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NavigationQueryFilter.generated.h"

/** Class containing definition of a navigation query filter */
UCLASS(DefaultToInstanced, abstract, Config=Engine, Blueprintable, DependsOn=(UNavigationSystem))
class ENGINE_API UNavigationQueryFilter : public UObject
{
	GENERATED_UCLASS_BODY()

protected:
	TSharedPtr<FNavigationQueryFilter> QueryFilter;

public:
	TSharedPtr<const FNavigationQueryFilter> GetQueryFilter() const { return QueryFilter; }
	
	static TSharedPtr<const FNavigationQueryFilter> GetQueryFilter(const TSubclassOf<UNavigationQueryFilter>& FilterClass);
};
