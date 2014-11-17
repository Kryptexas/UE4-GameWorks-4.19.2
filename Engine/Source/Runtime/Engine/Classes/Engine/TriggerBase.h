// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "GameFramework/Actor.h"
#include "TriggerBase.generated.h"

class UShapeComponent;
class UBillboardComponent;

/** An actor used to generate collision events (begin/end overlap) in the level. */
UCLASS(ClassGroup=Common, abstract, ConversionRoot, MinimalAPI)
class ATriggerBase : public AActor
{
	GENERATED_UCLASS_BODY()

	/** Shape component used for collision */
	UPROPERTY(Category=TriggerBase, VisibleAnywhere, BlueprintReadOnly)
	TSubobjectPtr<UShapeComponent> CollisionComponent;

	/** Billboard used to see the trigger in the editor */
	UPROPERTY()
	TSubobjectPtr<UBillboardComponent> SpriteComponent;
};



