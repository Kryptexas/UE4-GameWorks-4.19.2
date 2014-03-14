// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BodyInstance2D.generated.h"

USTRUCT()
struct PAPER2D_API FBodyInstance2D
{
	GENERATED_USTRUCT_BODY()


	/** If true, this body will use simulation. If false, will be 'fixed' (ie kinematic) and move where it is told. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics)
	uint32 bSimulatePhysics:1;

	UPROPERTY(EditAnywhere, Category=Physics)
	float HackWidth;

	UPROPERTY(EditAnywhere, Category=Physics)
	float HackHeight;

	//@TODO: SO MUCH EVIL: this controls whether or not this sprite has been nominated to draw the physics debug scene...
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics)
	uint32 bDrawPhysicsSceneMaster:1;


	FBodyInstance2D();

	void InitBody(class UBodySetup2D* Setup, const class UPaperSprite* SpriteSetupHack, const FTransform& Transform, class UPrimitiveComponent* PrimComp);//,class FPhysScene* InRBScene, class physx::PxAggregate* InAggregate = NULL);
	void TermBody();

	/** See if this body is valid. */
	bool IsValidBodyInstance() const;

	/** Get current transform in world space from physics body. */
	FTransform GetUnrealWorldTransform() const;

	/** Set this body to be fixed (kinematic) or not. */
	void SetInstanceSimulatePhysics(bool bSimulate);

	/** 
	 *	Move the physics body to a new pose. 
	 */
	void SetBodyTransform(const FTransform& NewTransform);

public:
	/** PrimitiveComponent containing this body.   */
	TWeakObjectPtr<class UPrimitiveComponent> OwnerComponentPtr;

protected:
	class b2Body* BodyInstancePtr;
};
