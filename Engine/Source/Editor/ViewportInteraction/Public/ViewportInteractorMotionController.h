// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ViewportInteractor.h"
#include "HeadMountedDisplayTypes.h"	// For EHMDDeviceType::Type
#include "ViewportInteractorMotionController.generated.h"

/**
 * Represents the interactor in the world
 */
UCLASS()
class VIEWPORTINTERACTION_API UViewportInteractorMotionController : public UViewportInteractor
{
	GENERATED_UCLASS_BODY()

public:

	virtual ~UViewportInteractorMotionController();

	// ViewportInteractorInterface overrides
	virtual void Shutdown() override;
	virtual void Tick( const float DeltaTime ) override;

	/** @return Returns the type of HMD we're dealing with */
	EHMDDeviceType::Type GetHMDDeviceType() const;

	// ViewportInteractor
	virtual bool GetTransformAndForwardVector( FTransform& OutHandTransform, FVector& OutForwardVector );

	/** Starts haptic feedback for physical motion controller */
	virtual void PlayHapticEffect( const float Strength ) override;
	
	/** Stops playing any haptic effects that have been going for a while.  Called every frame. */
	void StopOldHapticEffects();
	
	/** Get the side of the controller */
	EControllerHand GetControllerSide() const;

	/** Get the motioncontroller component of this interactor */
	class UMotionControllerComponent* GetMotionControllerComponent() const;
	
	/** Gets the trigger value */
	float GetSelectAndMoveTriggerValue() const;

protected:

	// ViewportInteractor
	virtual void HandleInputAxis( FViewportActionKeyInput& Action, const FKey Key, const float Delta, const float DeltaTime, bool& bOutWasHandled ) override;

	/** Polls input for the motion controllers transforms */
	virtual void PollInput() override;

	/** Check if the trigger is lightly pressed */
	bool IsTriggerAtLeastLightlyPressed() const;

	/** Get the real time the trigger was lightly pressed */
	double GetRealTimeTriggerWasLightlyPressed() const;

	/** Set that the trigger is at least lighty pressed */
	void SetTriggerAtLeastLightlyPressed( const bool bInTriggerAtLeastLightlyPressed );

	/** Set that the trigger has been released since last press */
	void SetTriggerBeenReleasedSinceLastPress( const bool bInTriggerBeenReleasedSinceLastPress );

protected:
	
	/** Motion controller component which handles late-frame transform updates of all parented sub-components */
	UPROPERTY()
	class UMotionControllerComponent* MotionControllerComponent;

	//
	// Graphics
	//

	/** Mesh for this hand */
	UPROPERTY()
	class UStaticMeshComponent* HandMeshComponent;

	/** Mesh for this hand's laser pointer */
	UPROPERTY()
	class UStaticMeshComponent* LaserPointerMeshComponent;

	/** MID for laser pointer material (opaque parts) */
	UPROPERTY()
	class UMaterialInstanceDynamic* LaserPointerMID;

	/** MID for laser pointer material (translucent parts) */
	UPROPERTY()
	class UMaterialInstanceDynamic* TranslucentLaserPointerMID;

	/** Hover impact indicator mesh */
	UPROPERTY()
	class UStaticMeshComponent* HoverMeshComponent;

	/** Hover point light */
	UPROPERTY()
	class UPointLightComponent* HoverPointLightComponent;

	/** MID for hand mesh */
	UPROPERTY()
	class UMaterialInstanceDynamic* HandMeshMID;

	/** Right or left hand */
	EControllerHand ControllerHandSide;

	/** True if this hand has a motion controller (or both!) */
	bool bHaveMotionController;

	// Special key action names for motion controllers
	static const FName TrackpadPositionX;
	static const FName TrackpadPositionY;
	static const FName TriggerAxis;
	static const FName MotionController_Left_FullyPressedTriggerAxis;
	static const FName MotionController_Right_FullyPressedTriggerAxis;
	static const FName MotionController_Left_LightlyPressedTriggerAxis;
	static const FName MotionController_Right_LightlyPressedTriggerAxis;

private:

	//
	// Trigger axis state
	//

	/** True if trigger is fully pressed right now (or within some small threshold) */
	bool bIsTriggerFullyPressed;

	/** True if the trigger is currently pulled far enough that we consider it in a "half pressed" state */
	bool bIsTriggerAtLeastLightlyPressed;

	/** Real time that the trigger became lightly pressed.  If the trigger remains lightly pressed for a reasonably 
	long duration before being pressed fully, we'll continue to treat it as a light press in some cases */
	double RealTimeTriggerWasLightlyPressed;

	/** True if trigger has been fully released since the last press */
	bool bHasTriggerBeenReleasedSinceLastPress;

	/** Current trigger pressed amount for 'select and move' (0.0 - 1.0) */
	float SelectAndMoveTriggerValue;

	//
	// Haptic feedback
	//

	/** The last real time we played a haptic effect.  This is used to turn off haptic effects shortly after they're triggered. */
	double LastHapticTime;

};