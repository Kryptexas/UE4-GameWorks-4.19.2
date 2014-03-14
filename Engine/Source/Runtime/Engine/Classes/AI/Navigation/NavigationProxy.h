// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NavigationProxy.generated.h"

UCLASS(ClassGroup=Building)
class ENGINE_API UNavigationProxy : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	AActor* MyOwner;

	/** If set, actor will cause update even if proxy is already added to octree */
	uint32 bDirty : 1;
};
