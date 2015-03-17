// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/**
 * A base class for the helper object that holds thumbnail information an asset.
 */

#pragma once
#include "ThumbnailInfo.generated.h"

UCLASS(MinimalAPI)
class UThumbnailInfo : public UObject
{
	GENERATED_BODY()
public:
	ENGINE_API UThumbnailInfo(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};



