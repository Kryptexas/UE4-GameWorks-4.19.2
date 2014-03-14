// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VectorFieldVolume: Volume encompassing a vector field.
=============================================================================*/

#pragma once

#include "VectorFieldVolume.generated.h"

UCLASS(hidecategories=(Object, Advanced, Collision), MinimalAPI)
class AVectorFieldVolume : public AActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=VectorFieldVolume)
	TSubobjectPtr<class UVectorFieldComponent> VectorFieldComponent;

	UPROPERTY()
	TSubobjectPtr<UBillboardComponent> SpriteComponent;
};



