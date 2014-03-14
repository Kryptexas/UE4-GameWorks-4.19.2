// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * An actor used to generate collision events (begin/end overlap) as inputs into the scripting system.
 */

#pragma once
#include "TriggerBase.generated.h"

UCLASS(dependson=AMatineeActor, ClassGroup=Common, abstract, ConversionRoot, MinimalAPI)
class ATriggerBase : public AActor
{
	GENERATED_UCLASS_BODY()

	/** Base collision component */
	UPROPERTY(Category=TriggerBase, VisibleAnywhere, BlueprintReadOnly)
	TSubobjectPtr<class UShapeComponent> CollisionComponent;

	// Reference to the billboard component
	UPROPERTY()
	TSubobjectPtr<UBillboardComponent> SpriteComponent;
};



