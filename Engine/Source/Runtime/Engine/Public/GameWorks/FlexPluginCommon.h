#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "FlexPluginCommon.generated.h"

/** Defines values that control how the localized inertia is applied */
USTRUCT()
struct ENGINE_API FFlexInertialScale
{
	GENERATED_USTRUCT_BODY()

	/** Scale how much of local linear velocity to transmit */
	UPROPERTY(EditAnywhere, Category = LocalInertia)
	float LinearInertialScale;

	/** Scale how much of local angular velocity to transmit */
	UPROPERTY(EditAnywhere, Category = LocalInertia)
	float AngularInertialScale;

	FFlexInertialScale();
};

/** Defines flags that control how the particle behaves */
USTRUCT()
struct ENGINE_API FFlexPhase
{
	GENERATED_USTRUCT_BODY()

	/** If true, then particles will be auto-assigned a new group, by default particles will only collide with particles in different groups */
	UPROPERTY(EditAnywhere, Category = Phase)
	bool AutoAssignGroup;

	/** Manually set the group that the particles will be placed in */
	UPROPERTY(EditAnywhere, Category = Phase, meta = (editcondition = "!AutoAssignGroup"))
	int32 Group;

	/** Control whether particles interact with other particles in the same group */
	UPROPERTY(EditAnywhere, Category = Phase)
	bool SelfCollide;

	/** If true then particles will not collide or interact with any particles they overlap in the rest pose */
	UPROPERTY(EditAnywhere, Category = Phase)
	bool IgnoreRestCollisions;

	/** Control whether the particles will generate fluid density constraints when interacting with other fluid particles, note that fluids must also be enabled on the container */
	UPROPERTY(EditAnywhere, Category = Phase)
	bool Fluid;

	FFlexPhase();
};
