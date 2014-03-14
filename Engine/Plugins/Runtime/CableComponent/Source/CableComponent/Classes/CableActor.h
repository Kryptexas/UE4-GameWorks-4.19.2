// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CableActor.generated.h"

UCLASS(hidecategories=(Input,Collision,Replication))
class ACableActor : public AActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Category=Cable, VisibleAnywhere, BlueprintReadOnly)
	TSubobjectPtr<class UCableComponent> CableComponent;
};
