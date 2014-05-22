// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BodyInstance2D.h"
#include "PrimitiveComponent2D.generated.h"

UCLASS(MinimalAPI)
class UPrimitiveComponent2D : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

protected:
	// Physics scene information for this component, holds a single rigid body with multiple shapes.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Collision)//, meta=(ShowOnlyInnerProperties))
	FBodyInstance2D BodyInstance2D;

public:
	// UActorComponent interface
	virtual void CreatePhysicsState() OVERRIDE;
	virtual void DestroyPhysicsState() OVERRIDE;
	// End of UActorComponent interface

	// USceneComponent interface
	virtual void OnUpdateTransform(bool bSkipPhysicsMove) OVERRIDE;
	// End of USceneComponent interface

	// UPrimitiveComponent interface
	virtual void SetSimulatePhysics(bool bSimulate) OVERRIDE;
	// End of UPrimitiveComponent interface

	/** Return the BodySetup to use for this PrimitiveComponent (single body case) */
	virtual class UBodySetup2D* GetBodySetup2D() const;

protected:
	//@TODO: Document
	virtual void CreatePhysicsState2D();

	//@TODO: Document
	virtual void DestroyPhysicsState2D();
};
