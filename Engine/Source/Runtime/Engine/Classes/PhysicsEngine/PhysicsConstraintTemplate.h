// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

//~=============================================================================
// Complete constraint definition used by rigid body physics.
// 
// Defaults here will give you a ball and socket joint.
// Positions are in Physics scale.
// When adding stuff here, make sure to update URB_ConstraintSetup::CopyConstraintParamsFrom
//~=============================================================================

#pragma once 

#include "ConstraintInstance.h"
#include "PhysicsConstraintTemplate.generated.h"

/** Holds the constraint profile applicable for a given constraint*/
UCLASS()
class UPhysicsConstraintProfile : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY()
	FConstraintProfileProperties ProfileProperties;
};

USTRUCT()
struct FPhysicsConstraintProfileHandle
{
	GENERATED_BODY()
	
	UPROPERTY()
	UPhysicsConstraintProfile* Profile;

	UPROPERTY(EditAnywhere, Category=Constraint)
	FName ProfileName;
};

UCLASS(hidecategories=Object, MinimalAPI)
class UPhysicsConstraintTemplate : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Joint, meta=(ShowOnlyInnerProperties))
	FConstraintInstance DefaultInstance;

	/** Easily lookup from name to constraint profile */
	UPROPERTY(transient)
	TMap<FName, UPhysicsConstraintProfile*> ConstraintProfileNameMap;

	/** Handles to the constraint profiles applicable to this constraint*/
	UPROPERTY(EditAnywhere, Category = Joint)
	TArray<FPhysicsConstraintProfileHandle> ProfileHandles;

	/** Which constraint profile to use by default when creating the constraint instance */
	UPROPERTY(EditAnywhere, Category=Joint)
	FName DefaultProfileName;

	//~ Begin UObject Interface.
	virtual void Serialize(FArchive& Ar) override;
	//~ End UObject Interface.

	UPhysicsConstraintProfile* GetDefaultProfile() const
	{
		return ConstraintProfileNameMap.FindRef(DefaultProfileName);
	}

	UPhysicsConstraintProfile* GetProfile(FName ProfileName) const
	{
		return ConstraintProfileNameMap.FindRef(ProfileName);
	}

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	/** When no profile is selected, use these settings. Allows us to save whatever setting user had before switching to new profile for example */
	UPROPERTY()
	FConstraintProfileProperties NoProfileInstance;

	// Only needed for old content! Pre VER_UE4_ALL_PROPS_TO_CONSTRAINTINSTANCE
	void CopySetupPropsToInstance(FConstraintInstance* Instance);

#if WITH_EDITORONLY_DATA
	void UpdateInstanceFromProfile();
#endif

#if WITH_EDITORONLY_DATA
	void UpdateProfileBeingEdited();
#endif

	// Makes sure there are no duplicate names or null entries in the profiles array.
	void SanitizeProfileArray();

	// Updates the constraint profile map after array has been changed
	void UpdateConstraintProfileMap();

public:	//DEPRECATED
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	FName JointName_DEPRECATED;
	UPROPERTY()
	FName ConstraintBone1_DEPRECATED;
	UPROPERTY()
	FName ConstraintBone2_DEPRECATED;
	UPROPERTY()
	FVector Pos1_DEPRECATED;
	UPROPERTY()
	FVector PriAxis1_DEPRECATED;
	UPROPERTY()
	FVector SecAxis1_DEPRECATED;
	UPROPERTY()
	FVector Pos2_DEPRECATED;
	UPROPERTY()
	FVector PriAxis2_DEPRECATED;
	UPROPERTY()
	FVector SecAxis2_DEPRECATED;
	UPROPERTY()
	uint32 bEnableProjection_DEPRECATED : 1;
	UPROPERTY()
	float ProjectionLinearTolerance_DEPRECATED;
	UPROPERTY()
	float ProjectionAngularTolerance_DEPRECATED;
	UPROPERTY()
	TEnumAsByte<enum ELinearConstraintMotion> LinearXMotion_DEPRECATED;
	UPROPERTY()
	TEnumAsByte<enum ELinearConstraintMotion> LinearYMotion_DEPRECATED;
	UPROPERTY()
	TEnumAsByte<enum ELinearConstraintMotion> LinearZMotion_DEPRECATED;
	UPROPERTY()
	float LinearLimitSize_DEPRECATED;
	UPROPERTY()
	uint32 bLinearLimitSoft_DEPRECATED : 1;
	UPROPERTY()
	float LinearLimitStiffness_DEPRECATED;
	UPROPERTY()
	float LinearLimitDamping_DEPRECATED;
	UPROPERTY()
	uint32 bLinearBreakable_DEPRECATED : 1;
	UPROPERTY()
	float LinearBreakThreshold_DEPRECATED;
	UPROPERTY()
	TEnumAsByte<enum EAngularConstraintMotion> AngularSwing1Motion_DEPRECATED;
	UPROPERTY()
	TEnumAsByte<enum EAngularConstraintMotion> AngularSwing2Motion_DEPRECATED;
	UPROPERTY()
	TEnumAsByte<enum EAngularConstraintMotion> AngularTwistMotion_DEPRECATED;
	UPROPERTY()
	uint32 bSwingLimitSoft_DEPRECATED : 1;
	UPROPERTY()
	uint32 bTwistLimitSoft_DEPRECATED : 1;
	UPROPERTY()
	float Swing1LimitAngle_DEPRECATED;
	UPROPERTY()
	float Swing2LimitAngle_DEPRECATED;
	UPROPERTY()
	float TwistLimitAngle_DEPRECATED;
	UPROPERTY()
	float SwingLimitStiffness_DEPRECATED;
	UPROPERTY()
	float SwingLimitDamping_DEPRECATED;
	UPROPERTY()
	float TwistLimitStiffness_DEPRECATED;
	UPROPERTY()
	float TwistLimitDamping_DEPRECATED;
	UPROPERTY()
	uint32 bAngularBreakable_DEPRECATED : 1;
	UPROPERTY()
	float AngularBreakThreshold_DEPRECATED;
#endif
};



