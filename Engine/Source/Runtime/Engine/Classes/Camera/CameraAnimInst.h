// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/**
 *	CameraAnim: defines a pre-packaged animation to be played on a camera.
 */

#pragma once
#include "CameraAnimInst.generated.h"

UCLASS(notplaceable)
class UCameraAnimInst : public UObject
{
	GENERATED_UCLASS_BODY()

	/** which CameraAnim this is an instance of */
	UPROPERTY()
	class UCameraAnim* CamAnim;

protected:
	/** the UInterpGroupInst used to do the interpolation */
	UPROPERTY(instanced)
	TSubobjectPtr<class UInterpGroupInst> InterpGroupInst;

public:
	/** Current time for the animation */
	UPROPERTY(transient)
	float CurTime;

protected:
	/** True if the animation should loop, false otherwise. */
	UPROPERTY(transient)
	uint32 bLooping:1;

public:
	/** True if the animation has finished, false otherwise. */
	UPROPERTY(transient)
	uint32 bFinished:1;

	/** True if it's ok for the system to auto-release this instance upon completion. */
	UPROPERTY(transient)
	uint32 bAutoReleaseWhenFinished:1;

protected:
	/** Time to interpolate in from zero, for smooth starts. */
	UPROPERTY()
	float BlendInTime;

	/** Time to interpolate out to zero, for smooth finishes. */
	UPROPERTY()
	float BlendOutTime;

	/** True if currently blending in. */
	UPROPERTY(transient)
	uint32 bBlendingIn:1;

	/** True if currently blending out. */
	UPROPERTY(transient)
	uint32 bBlendingOut:1;

	/** Current time for the blend-in.  I.e. how long we have been blending. */
	UPROPERTY(transient)
	float CurBlendInTime;

	/** Current time for the blend-out.  I.e. how long we have been blending. */
	UPROPERTY(transient)
	float CurBlendOutTime;

public:
	/** Multiplier for playback rate.  1.0 = normal. */
	UPROPERTY()
	float PlayRate;

	/** "Intensity" scalar.  This is the scale at which the anim was first played.  */
	UPROPERTY()
	float BasePlayScale;

	/** A supplemental scale factor, allowing external systems to scale this anim as necessary.  This is reset to 1.f each frame. */
	UPROPERTY()
	float TransientScaleModifier;

	/* Number in range [0..1], controlling how much this influence this instance should have. */
	UPROPERTY()
	float CurrentBlendWeight;

protected:
	/** How much longer to play the anim, if a specific duration is desired.  Has no effect if 0.  */
	UPROPERTY(transient)
	float RemainingTime;

public:
	/** cached movement track from the currently playing anim so we don't have to go find it every frame */
	UPROPERTY(transient)
	class UInterpTrackMove* MoveTrack;

	UPROPERTY(transient)
	class UInterpTrackInstMove* MoveInst;

	UPROPERTY()
	TEnumAsByte<enum ECameraAnimPlaySpace> PlaySpace;

	/** The user-defined space for CAPS_UserDefined */
	UPROPERTY(transient)
	FMatrix UserPlaySpaceMatrix;

	/** Camera Anim debug variable to trace back to previous location **/
	UPROPERTY(transient)
	FVector LastCameraLoc;


	/**
	 * Starts this instance playing the specified CameraAnim.
	 *
	 * CamAnim:		The animation that should play on this instance.
	 * CamActor:	The  AActor  that will be modified by this animation.
	 * InRate:		How fast to play the animation.  1.f is normal.
	 * InScale:		How intense to play the animation.  1.f is normal.
	 * InBlendInTime:	Time over which to linearly ramp in.
	 * InBlendInTime:	Time over which to linearly ramp out.
	 * bInLoop:			Whether or not to loop the animation.
	 * bRandomStartTime:	Whether or not to choose a random time to start playing.  Only really makes sense for bLoop = true;
	 * Duration:	optional specific playtime for this animation.  This is total time, including blends.
	 */
	void Play(class UCameraAnim* Anim, class AActor* CamActor, float InRate, float InScale, float InBlendInTime, float InBlendOutTime, bool bInLoop, bool bRandomStartTime, float Duration = 0.f);
	
	/** Updates this active instance with new parameters. */
	void Update(float NewRate, float NewScale, float NewBlendInTime, float NewBlendOutTime, float NewDuration = 0.f);
	
	/** advances the animation by the specified time - updates any modified interp properties, moves the group actor, etc */
	void AdvanceAnim(float DeltaTime, bool bJump);
	
	/** Stops this instance playing whatever animation it is playing. */
	void Stop(bool bImmediate = false);
	
	/** Applies given scaling factor to the playing animation for the next update only. */
	void ApplyTransientScaling(float Scalar);
	
	/** Sets this anim to play in an alternate playspace */
	void SetPlaySpace(ECameraAnimPlaySpace NewSpace, FRotator UserPlaySpace = FRotator::ZeroRotator);
};



