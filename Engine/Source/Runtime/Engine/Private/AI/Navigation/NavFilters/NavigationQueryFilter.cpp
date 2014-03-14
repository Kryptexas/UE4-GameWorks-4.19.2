// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UNavigationQueryFilter::UNavigationQueryFilter(const class FPostConstructInitializeProperties& PCIP) 
	: Super(PCIP)
{
	QueryFilter = MakeShareable(new FNavigationQueryFilter());
}

TSharedPtr<const FNavigationQueryFilter> UNavigationQueryFilter::GetQueryFilter(const TSubclassOf<UNavigationQueryFilter>& FilterClass)
{
	const UNavigationQueryFilter* DefaultObject = FilterClass.GetDefaultObject();
	return DefaultObject ? DefaultObject->GetQueryFilter() : NULL;
}