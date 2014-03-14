// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

//=============================================================================
// The Basic constraint actor class.
//=============================================================================

#include "PhysicsConstraintActor.generated.h"

UCLASS(ConversionRoot, MinimalAPI)
class APhysicsConstraintActor : public ARigidBodyBase
{
	GENERATED_UCLASS_BODY()

	// Cached reference to constraint component
	UPROPERTY(Category=ConstraintActor, VisibleAnywhere, BlueprintReadOnly, meta=(ExposeFunctionCategories="JointDrive,Physics|Components|PhysicsConstraint"))
	TSubobjectPtr<class UPhysicsConstraintComponent> ConstraintComp;
	
	UPROPERTY()
	class AActor* ConstraintActor1_DEPRECATED;
	UPROPERTY()
	class AActor* ConstraintActor2_DEPRECATED;
	UPROPERTY()
	uint32 bDisableCollision_DEPRECATED:1;

	// Begin UObject Interface
	virtual void PostLoad() OVERRIDE;
#if WITH_EDITOR
	virtual void LoadedFromAnotherClass(const FName& OldClassName) OVERRIDE;
#endif // WITH_EDITOR	
	// End UObject Interface
};



