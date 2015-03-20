// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ProceduralFoliageVolume.generated.h"

class UProceduralFoliageComponent;

UCLASS()
class FOLIAGE_API AProceduralFoliageVolume: public AVolume
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Category = ProceduralFoliage, VisibleAnywhere, BlueprintReadOnly)
	UProceduralFoliageComponent* ProceduralComponent;

#if WITH_EDITOR
	virtual bool GetReferencedContentObjects(TArray<UObject*>& Objects) const override;
#endif
};