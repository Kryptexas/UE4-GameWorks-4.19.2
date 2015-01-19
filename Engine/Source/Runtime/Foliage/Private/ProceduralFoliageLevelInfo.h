// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ProceduralFoliageLevelInfo.generated.h"

/** Procedural info is used for tagging which foliage instances belong to the owning procedural foliage asset in the level */

//
UCLASS()
class FOLIAGE_API AProceduralFoliageLevelInfo : public AActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	FGuid ProceduralContentGuid;
};
