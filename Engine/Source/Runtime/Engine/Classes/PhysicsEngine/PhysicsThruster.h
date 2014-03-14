// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.



#pragma once
#include "PhysicsThruster.generated.h"

/** 
 *	Attach one of these on an object using physics simulation and it will apply a force down the negative-X direction
 *	ie. point X in the direction you want the thrust in.
 */
UCLASS(hidecategories=(Input,Collision,Replication))
class APhysicsThruster : public ARigidBodyBase
{
	GENERATED_UCLASS_BODY()

	/** Thruster component */
	UPROPERTY(Category=Physics, VisibleAnywhere, BlueprintReadOnly, meta=(ExposeFunctionCategories="Activation,Components|Activation"))
	TSubobjectPtr<class UPhysicsThrusterComponent> ThrusterComponent;


#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TSubobjectPtr<class UArrowComponent> ArrowComponent;

	UPROPERTY()
	TSubobjectPtr<class UBillboardComponent> SpriteComponent;
#endif

};



