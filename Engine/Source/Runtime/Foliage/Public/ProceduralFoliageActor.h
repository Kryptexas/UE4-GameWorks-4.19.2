// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ProceduralFoliageActor.generated.h"

class UProceduralFoliageComponent;

UCLASS()
class FOLIAGE_API AProceduralFoliageActor: public AActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Category = ProceduralFoliageTile, VisibleAnywhere, BlueprintReadOnly)
	UProceduralFoliageComponent* ProceduralComponent;
};