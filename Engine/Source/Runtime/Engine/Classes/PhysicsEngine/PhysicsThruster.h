// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.



#pragma once
#include "PhysicsThruster.generated.h"

/** 
 *	Attach one of these on an object using physics simulation and it will apply a force down the negative-X direction
 *	ie. point X in the direction you want the thrust in.
 */
UCLASS(hidecategories=(Input,Collision,Replication), showcategories=("Input|MouseInput", "Input|TouchInput"))
class APhysicsThruster : public ARigidBodyBase
{
	GENERATED_UCLASS_BODY()

private:
	/** Thruster component */
	UPROPERTY(Category = Physics, VisibleAnywhere, BlueprintReadOnly, meta = (ExposeFunctionCategories = "Activation,Components|Activation", AllowPrivateAccess = "true"))
	class UPhysicsThrusterComponent* ThrusterComponent;

#if WITH_EDITORONLY_DATA

	UPROPERTY()
	class UArrowComponent* ArrowComponent;

	UPROPERTY()
	class UBillboardComponent* SpriteComponent;
public:
#endif


public:
	/** Returns ThrusterComponent subobject **/
	FORCEINLINE class UPhysicsThrusterComponent* GetThrusterComponent() const { return ThrusterComponent; }
#if WITH_EDITORONLY_DATA
	/** Returns ArrowComponent subobject **/
	FORCEINLINE class UArrowComponent* GetArrowComponent() const { return ArrowComponent; }
	/** Returns SpriteComponent subobject **/
	FORCEINLINE class UBillboardComponent* GetSpriteComponent() const { return SpriteComponent; }
#endif
};



