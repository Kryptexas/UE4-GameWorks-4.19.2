// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BodyInstance2D.generated.h"

USTRUCT()
struct PAPER2D_API FBodyInstance2D
{
	GENERATED_USTRUCT_BODY()

	/** If true, this body will use simulation. If false, will be 'fixed' (i.e., kinematic) and move where it is told. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Physics)
	uint32 bSimulatePhysics:1;

	/**	Allows you to override the PhysicalMaterial to use for simple collision on this body. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category = Collision)
	class UPhysicalMaterial* PhysMaterialOverride;

	FBodyInstance2D();

	void InitBody(class UBodySetup2D* Setup, const FTransform& Transform, class UPrimitiveComponent2D* PrimComp);
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

	/** Should Simulate Physics **/
	bool ShouldInstanceSimulatingPhysics() const;

	/** Returns true if this body is simulating, false if it is fixed (kinematic) */
	bool IsInstanceSimulatingPhysics() const;

public:
	/** PrimitiveComponent containing this body.   */
	TWeakObjectPtr<class UPrimitiveComponent2D> OwnerComponentPtr;

protected:

#if WITH_BOX2D
	class b2Body* BodyInstancePtr;
#endif
};
