// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 *
 */

#pragma once
#include "TargetPoint.generated.h"

UCLASS(MinimalAPI)
class ATargetPoint : public AActor
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Display)
	TSubobjectPtr<class UBillboardComponent> SpriteComponent;

	UPROPERTY()
	TSubobjectPtr<class UArrowComponent> ArrowComponent;
#endif
};



