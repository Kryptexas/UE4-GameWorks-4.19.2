// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * 
 * Camera modifier that provides support for code-based oscillating camera shakes.
 */

#pragma once
#include "CameraModifier_CameraShake.generated.h"

USTRUCT()
struct FCameraShakeInstance
{
	GENERATED_USTRUCT_BODY()

	/** source shake */
	UPROPERTY()
	class UCameraShake* SourceShake;

	/** Used to identify shakes when single instances are desired */
	UPROPERTY()
	FName SourceShakeName;

	/** <0.f means play infinitely. */
	UPROPERTY()
	float OscillatorTimeRemaining;

	/** blend vars */
	UPROPERTY()
	uint32 bBlendingIn:1;

	UPROPERTY()
	float CurrentBlendInTime;

	UPROPERTY()
	uint32 bBlendingOut:1;

	UPROPERTY()
	float CurrentBlendOutTime;

	/** Current offsets. */
	UPROPERTY()
	FVector LocSinOffset;

	UPROPERTY()
	FVector RotSinOffset;

	UPROPERTY()
	float FOVSinOffset;

	UPROPERTY()
	float Scale;

	UPROPERTY()
	class UCameraAnimInst* AnimInst;

	/** What space to play the shake in before applying to the camera.  Affects Anim and Oscillation both. */
	UPROPERTY()
	TEnumAsByte<enum ECameraAnimPlaySpace> PlaySpace;

	/** Matrix defining the playspace, used when PlaySpace == CAPS_UserDefined */
	UPROPERTY()
	FMatrix UserPlaySpaceMatrix;


	FCameraShakeInstance()
		: SourceShake(NULL)
		, OscillatorTimeRemaining(0)
		, bBlendingIn(false)
		, CurrentBlendInTime(0)
		, bBlendingOut(false)
		, CurrentBlendOutTime(0)
		, LocSinOffset(ForceInit)
		, RotSinOffset(ForceInit)
		, FOVSinOffset(0)
		, Scale(0)
		, AnimInst(NULL)
		, PlaySpace(0)
		, UserPlaySpaceMatrix(ForceInit)
	{
	}

};

UCLASS(dependson=UCameraShake, config=Camera)
class UCameraModifier_CameraShake : public UCameraModifier
{
	GENERATED_UCLASS_BODY()

	/** Active CameraShakes array */
	UPROPERTY()
	TArray<struct FCameraShakeInstance> ActiveShakes;

protected:
	/** Scalar applied to all camera shakes in splitscreen. Normally used to dampen, since shakes feel more intense in a smaller viewport. */
	UPROPERTY(EditAnywhere, Category=CameraModifier_CameraShake)
	float SplitScreenShakeScale;

public:

protected:
	/** For situational scaling of individual shakes. */
	virtual float GetShakeScale(FCameraShakeInstance const& ShakeInst) const;
public:
	float InitializeOffset( const FFOscillator& Param );
	virtual void ReinitShake(int32 ActiveShakeIdx, float Scale);
	
	/** Initialize camera shake structure */
	virtual FCameraShakeInstance InitializeShake(TSubclassOf<class UCameraShake> NewShake, float Scale, ECameraAnimPlaySpace PlaySpace, FRotator UserPlaySpaceRot = FRotator::ZeroRotator);
	
	/** Add a new screen shake to the list */
	virtual void AddCameraShake(TSubclassOf<class UCameraShake> NewShake, float Scale, ECameraAnimPlaySpace PlaySpace=CAPS_CameraLocal, FRotator UserPlaySpaceRot = FRotator::ZeroRotator);
	
	virtual void RemoveCameraShake(TSubclassOf<class UCameraShake> Shake);
	virtual void RemoveAllCameraShakes();
	
	/** Update a CameraShake */
	virtual void UpdateCameraShake(float DeltaTime, FCameraShakeInstance& Shake, struct FMinimalViewInfo& InOutPOV);
	
	// Begin UCameraModifer Interface
	virtual bool ModifyCamera(APlayerCameraManager* Camera, float DeltaTime, struct FMinimalViewInfo& InOutPOV) OVERRIDE;
	// End UCameraModifer Interface
};



