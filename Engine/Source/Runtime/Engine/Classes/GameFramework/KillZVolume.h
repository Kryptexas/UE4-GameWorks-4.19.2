// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "KillZVolume.generated.h"

UCLASS()
class ENGINE_API AKillZVolume : public APhysicsVolume
{
	GENERATED_UCLASS_BODY()
	
	//Begin PhysicsVolume Interface
	virtual void ActorEnteredVolume(class AActor* Other) OVERRIDE;
	//End PhysicsVolume Interface
};



