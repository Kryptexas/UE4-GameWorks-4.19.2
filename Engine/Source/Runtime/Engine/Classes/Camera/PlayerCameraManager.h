// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "GameFramework/Actor.h"
#include "CameraTypes.h"
#include "CameraShake.h"
#include "PlayerCameraManager.generated.h"

class APawn;

UENUM()
enum EViewTargetBlendFunction
{
	/** Camera does a simple linear interpolation. */
	VTBlend_Linear,
	/** Camera has a slight ease in and ease out, but amount of ease cannot be tweaked. */
	VTBlend_Cubic,
	/** Camera immediately accelerates, but smoothly decelerates into the target.  Ease amount controlled by BlendExp. */
	VTBlend_EaseIn,
	/** Camera smoothly accelerates, but does not decelerate into the target.  Ease amount controlled by BlendExp. */
	VTBlend_EaseOut,
	/** Camera smoothly accelerates and decelerates.  Ease amount controlled by BlendExp. */
	VTBlend_EaseInOut,
	VTBlend_MAX,
};

/* Caching Camera, for optimization */
USTRUCT()
struct FCameraCacheEntry
{
	GENERATED_USTRUCT_BODY()

	/** Cached Time Stamp */
	UPROPERTY()
	float TimeStamp;

	/** cached Point of View */
	UPROPERTY()
	FMinimalViewInfo POV;

	FCameraCacheEntry()
		: TimeStamp(0.0f)
	{
	}
};

/**
 * View Target definition
 * A View Target is responsible for providing the Camera with an ideal Point of View (POV)
 */
USTRUCT()
struct ENGINE_API FTViewTarget
{
	GENERATED_USTRUCT_BODY()

	/** Target  AActor  used to compute ideal POV */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TViewTarget)
	class AActor* Target;

public:
	/** Point of View */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TViewTarget)
	struct FMinimalViewInfo POV;

protected:
	/** PlayerState (used to follow same player through pawn transitions, etc., when spectating) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TViewTarget)
	class APlayerState* PlayerState;

	//@TODO: All for GetNextViewablePlayer
	friend class APlayerController;
public:
	void SetNewTarget(AActor* NewTarget);

	APawn* GetTargetPawn() const;

	bool Equal(const FTViewTarget& OtherTarget) const;

	FTViewTarget()
		: Target(NULL)
		, PlayerState(NULL)
	{
	}

	/** Make sure ViewTarget is valid */
	void CheckViewTarget(APlayerController* OwningController);
};

/** A set of parameters to describe how to transition between viewtargets. */
USTRUCT()
struct FViewTargetTransitionParams
{
	GENERATED_USTRUCT_BODY()

	/** Total duration of blend to pending view target.  0 means no blending. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ViewTargetTransitionParams)
	float BlendTime;

	/** Function to apply to the blend parameter */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ViewTargetTransitionParams)
	TEnumAsByte<enum EViewTargetBlendFunction> BlendFunction;

	/** Exponent, used by certain blend functions to control the shape of the curve. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ViewTargetTransitionParams)
	float BlendExp;

	/** If true, lock outgoing viewtarget to last frame's camera position for the remainder of the blend.
	 *  This is useful if you plan to teleport the viewtarget, but want to keep the camera motion smooth. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ViewTargetTransitionParams)
	uint32 bLockOutgoing:1;

	FViewTargetTransitionParams()
		: BlendTime(0.f)
		, BlendFunction(VTBlend_Cubic)
		, BlendExp(2.f)
		, bLockOutgoing(false)
	{}

	float GetBlendAlpha(const float& TimePct) const
	{
		switch (BlendFunction)
		{
		case VTBlend_Linear: return FMath::Lerp(0.f, 1.f, TimePct); 
		case VTBlend_Cubic:	return FMath::CubicInterp(0.f, 0.f, 1.f, 0.f, TimePct); 
		case VTBlend_EaseInOut: return FMath::InterpEaseInOut(0.f, 1.f, TimePct, BlendExp); 
		case VTBlend_EaseIn: return FMath::Lerp(0.f, 1.f, FMath::Pow(TimePct, BlendExp)); 
		case VTBlend_EaseOut: return FMath::Lerp(0.f, 1.f, FMath::Pow(TimePct, (FMath::IsNearlyZero(BlendExp) ? 1.f : (1.f / BlendExp))));
		default:
			break;
		}

		return 1.f;
	}
};

/**
 *	Defines the point of view of a player in world space.
 */
UCLASS(notplaceable, transient, BlueprintType, Blueprintable)
class ENGINE_API APlayerCameraManager : public AActor
{
	GENERATED_UCLASS_BODY()

	/** PlayerController owning this Camera actor  */
	UPROPERTY()
	class APlayerController* PCOwner;

private_subobject:
	/** Dummy component we can use to attach things to the camera. */
	DEPRECATED_FORGAME(4.6, "TransformComponent should not be accessed directly, please use GetTransformComponent() function instead. TransformComponent will soon be private and your code will not compile.")
	UPROPERTY(Category = PlayerCameraManager, VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USceneComponent* TransformComponent;
public:

	/** Camera Mode */
	UPROPERTY()
	FName CameraStyle;

	/** default FOV */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PlayerCameraManager)
	float DefaultFOV;

	/** value FOV is locked at */
	UPROPERTY()
	float LockedFOV;

	/** The default desired width (in world units) of the orthographic view (ignored in Perspective mode) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PlayerCameraManager)
	float DefaultOrthoWidth;

	/** value OrthoWidth is locked at */
	UPROPERTY()
	float LockedOrthoWidth;

	/** True when this camera should use an orthographic perspective instead of FOV */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PlayerCameraManager)
	bool bIsOrthographic;

	/** Default aspect ratio */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PlayerCameraManager)
	float DefaultAspectRatio;

	/** Color to fade to. */
	UPROPERTY()
	FColor FadeColor;

	/** Amount of fading to apply. */
	UPROPERTY()
	float FadeAmount;

	/** Allows control over scaling individual color channels in the final image. */
	UPROPERTY()
	FVector ColorScale;

	/** Desired color scale which ColorScale will interpolate to */
	UPROPERTY()
	FVector DesiredColorScale;

	/** Color scale value at start of interpolation */
	UPROPERTY()
	FVector OriginalColorScale;

	/** Total time for color scale interpolation to complete */
	UPROPERTY()
	float ColorScaleInterpDuration;

	/** Time at which interpolation started */
	UPROPERTY()
	float ColorScaleInterpStartTime;

	UPROPERTY()
	struct FCameraCacheEntry CameraCache;

	UPROPERTY()
	struct FCameraCacheEntry LastFrameCameraCache;

	/** Current ViewTarget */
	UPROPERTY()
	struct FTViewTarget ViewTarget;

	/** Pending view target for blending */
	UPROPERTY()
	struct FTViewTarget PendingViewTarget;

	/** Time left when blending to pending view target */
	UPROPERTY()
	float BlendTimeToGo;

	UPROPERTY()
	struct FViewTargetTransitionParams BlendParams;

	/** List of camera modifiers to apply during update of camera position/ rotation */
	UPROPERTY()
	TArray<class UCameraModifier*> ModifierList;

	/** Distance to place free camera from view target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Debug)
	float FreeCamDistance;

	/** Offset to Z free camera position */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Debug)
	FVector FreeCamOffset;

	/** camera fade management */
	UPROPERTY()
	FVector2D FadeAlpha;

	/** length of time for fade to occur over */
	UPROPERTY()
	float FadeTime;

	/** time remaining for fade to occur over */
	UPROPERTY()
	float FadeTimeRemaining;

protected:
	// "Lens" effects (e.g. blood, dirt on camera)
	/** CameraBlood emitter attached to this camera */
	UPROPERTY(transient)
	TArray<class AEmitterCameraLensEffectBase*> CameraLensEffects;

public:
	/////////////////////
	// Camera Modifiers
	/////////////////////
	
	/** Camera modifier for cone-driven screen shakes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, transient, Category = PlayerCameraManager)
	class UCameraModifier_CameraShake* CameraShakeCamMod;

protected:
	/** Class to use when instantiating screenshake modifier object.  Provided to support overrides. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PlayerCameraManager)
	TSubclassOf<class UCameraModifier_CameraShake>  CameraShakeCamModClass;

	/** By default camera modifiers are not applied to stock debug cameras. Setting to true will always apply modifiers. */
	UPROPERTY()
	bool bAlwaysApplyModifiers;
	
	////////////////////////
	// CameraAnim support
	////////////////////////
	
	/** Pool of anim instance objects available with which to play camera animations */
	UPROPERTY()
	class UCameraAnimInst* AnimInstPool[8];    /*MAX_ACTIVE_CAMERA_ANIMS @fixmeconst*/

	/** Parallel arrays */
	TArray<struct FPostProcessSettings> PostProcessBlendCache;
	TArray<float> PostProcessBlendCacheWeights;

	void AddCachedPPBlend(struct FPostProcessSettings& PPSettings, float BlendWeight);
	void ClearCachedPPBlends();

public:
	/** Array of anim instances that are currently playing and in-use */
	UPROPERTY()
	TArray<class UCameraAnimInst*> ActiveAnims;

	void GetCachedPostProcessBlends(TArray<struct FPostProcessSettings> const*& OutPPSettings, TArray<float> const*& OutBlendWeigthts) const;
	
protected:
	/** Array of anim instances that are not playing and available */
	UPROPERTY()
	TArray<class UCameraAnimInst*> FreeAnims;

	/** Internal.  Receives the output of individual camera animations. */
	UPROPERTY(transient)
	class ACameraActor* AnimCameraActor;

	/** true if FOV is locked to a constant value*/
	UPROPERTY()
	uint32 bLockedFOV:1;

	/** true if OrthoWidth is locked to a constant value*/
	UPROPERTY()
	uint32 bLockedOrthoWidth:1;

public:
	/** If we should apply FadeColor/FadeAmount to the screen. */
	UPROPERTY()
	uint32 bEnableFading:1;

	/** Apply fading of audio alongside the video */
	UPROPERTY()
	uint32 bFadeAudio:1;

	/** Turn on scaling of color channels in final image using ColorScale property. */
	UPROPERTY()
	uint32 bEnableColorScaling:1;

	/** Should interpolate color scale values */
	UPROPERTY()
	uint32 bEnableColorScaleInterp:1;

	/** True if clients are handling setting their own viewtarget and the server should not replicate it (e.g. during certain matinees) */
	UPROPERTY()
	uint32 bClientSimulatingViewTarget:1;

	/** if true, server will use camera positions replicated from the client instead of calculating locally. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=PlayerCameraManager)
	uint32 bUseClientSideCameraUpdates:1;

	/** If true, replicate the client side camera position but don't use it, and draw the positions on the server */
	UPROPERTY()
	uint32 bDebugClientSideCamera:1;

	/** if true, send a camera update to the server on next update */
	UPROPERTY(transient)
	uint32 bShouldSendClientSideCameraUpdate:1;

	// Whether we did a camera cut this frame. Automatically reset to false every frame.
	// This flag affects various things in the renderer (such as whether to use the occlusion queries from last frame, and motion blur)
	UPROPERTY(transient)
	uint32 bGameCameraCutThisFrame:1;

	/** Whether this camera's orientation should be updated by most recent HMD orientation or not. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PlayerCameraManager)
	uint32 bFollowHmdOrientation : 1;

	/** pitch limiting of camera facing direction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PlayerCameraManager)
	float ViewPitchMin;

	/** pitch limiting of camera facing direction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PlayerCameraManager)
	float ViewPitchMax;

	/** yaw limiting of camera facing direction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PlayerCameraManager)
	float ViewYawMin;

	/** yaw limiting of camera facing direction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PlayerCameraManager)
	float ViewYawMax;

	/** roll limiting of camera facing direction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PlayerCameraManager)
	float ViewRollMin;

	/** roll limiting of camera facing direction */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=PlayerCameraManager)
	float ViewRollMax;


	/** Blueprint hook to allow blueprints to implement custom cameras. Return true to use returned values, or false to ignore them. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic)
	virtual bool BlueprintUpdateCamera(AActor* CameraTarget, FVector& NewCameraLocation, FRotator& NewCameraRotation, float& NewCameraFOV);

	/** Returns the PlayerController that owns this camera. */
	UFUNCTION(BlueprintCallable, Category="Game|Player")
	virtual APlayerController* GetOwningPlayerController() const;

	static const int32 MAX_ACTIVE_CAMERA_ANIMS = 8;


	virtual void AssignViewTarget(AActor* NewTarget, FTViewTarget& VT, struct FViewTargetTransitionParams TransitionParams=FViewTargetTransitionParams());

	friend struct FTViewTarget;
public:
	/** Returns current ViewTarget */
	AActor* GetViewTarget() const;

	/** Returns the pawn that is being viewed (if any) */
	APawn* GetViewTargetPawn() const;

	// Begin AActor Interface
	virtual bool ShouldTickIfViewportsOnly() const override;
	virtual void PostInitializeComponents() override;
	virtual void Destroyed() override;
	virtual void DisplayDebug(class UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) override;
	virtual void ApplyWorldOffset(const FVector& InOffset, bool bWorldShift) override;
	// End AActor Interface

	/** Static.  Plays an in-world camera shake that affects all nearby players, with distance-based attenuation.
	 * @param InWorld - World context
	 * @param Shake - Camera shake asset to use
	 * @param Epicenter - location to place the effect in world space
	 * @param InnerRadius - Cameras inside this radius are ignored
	 * @param OuterRadius - Cameras outside of InnerRadius and inside this are effected
	 * @param Falloff - Affects falloff of effect as it nears OuterRadius
	 * @param bOrientShakeTowardsEpicenter - Changes the rotation of shake to point towards epicenter instead of forward
	 */
	static void PlayWorldCameraShake(UWorld* InWorld, TSubclassOf<UCameraShake> Shake, FVector Epicenter, float InnerRadius, float OuterRadius, float Falloff, bool bOrientShakeTowardsEpicenter = false );

protected:
	/** Internal.  Returns intensity scalar in the range [0..1] for a shake originating at Epicenter. */
	static float CalcRadialShakeScale(class APlayerCameraManager* Cam,FVector Epicenter,float InnerRadius,float OuterRadius,float Falloff);
public:

	/**
	 * Performs camera update.
	 * Called once per frame after all actors have been ticked.
	 * Non-local players replicate the POV if bUseClientSideCameraUpdates is true
	 */
	virtual void UpdateCamera(float DeltaTime);

	/** Internal. Creates and initializes a new camera modifier of the specified class, returns the object ref. */
	virtual class UCameraModifier* CreateCameraModifier(TSubclassOf<class UCameraModifier> ModifierClass);
	
	
	/**
	 * Apply modifiers on Camera.
	 * @param	DeltaTime	Time is seconds since last update
	 * @param	InOutPOV		Point of View
	 */
	virtual void ApplyCameraModifiers(float DeltaTime, FMinimalViewInfo& InOutPOV);
	
	/**
	 * Initialize Camera for associated PlayerController
	 * @param	PC	PlayerController attached to this Camera.
	 */
	virtual void InitializeFor(class APlayerController* PC);
	
	/** Returns camera's current FOV angle */
	virtual float GetFOVAngle() const;
	
	/** Lock FOV to a specific value. */
	virtual void SetFOV(float NewFOV);

	/** Remove lock from FOV */
	virtual void UnlockFOV();

	/** Returns true if this camera is using an orthographic perspective */
	virtual bool IsOrthographic() const;

	/** Returns the current orthographic width for the camera. Only used if IsOrthographic returns true */
	virtual float GetOrthoWidth() const;

	/** Sets the current orthographic width for the camera. Only used if IsOrthographic returns true */
	virtual void SetOrthoWidth(float OrthoWidth);

	/** Remove lock from OrthoWidth */
	virtual void UnlockOrthoWidth();

	/**
	 * Master function to retrieve Camera's actual view point.
	 * do not call this directly, call PlayerController::GetPlayerViewPoint() instead.
	 *
	 * @param	OutCamLoc	Camera Location
	 * @param	OutCamRot	Camera Rotation
	 */
	void GetCameraViewPoint(FVector& OutCamLoc, FRotator& OutCamRot) const;
	
	// @todo document
	UFUNCTION(BlueprintCallable, Category = "Camera")
	FRotator GetCameraRotation() const;

	// @todo document
	UFUNCTION(BlueprintCallable, Category = "Camera")
	FVector GetCameraLocation() const;
	
	/** Sets the new desired color scale and enables interpolation. */
	virtual void SetDesiredColorScale(FVector NewColorScale, float InterpTime);
	
protected:
	// @todo document
	virtual void DoUpdateCamera(float DeltaTime);
	
	/** Apply audio fading */
	virtual void ApplyAudioFade();
	
	/**
	 * Blend 2 viewtargets.
	 *
	 * @param	A		Source view target
	 * @paramn	B		destination view target
	 * @param	Alpha	Alpha, % of blend from A to B.
	 */
	FPOV BlendViewTargets(const FTViewTarget& A,const FTViewTarget& B, float Alpha);
	
public:
	/** Cache update results */
	void FillCameraCache(const FMinimalViewInfo& NewInfo);
	
protected:
	/**
	 * Query ViewTarget and outputs Point Of View.
	 *
	 * @param	OutVT		ViewTarget to use.
	 * @param	DeltaTime	Delta Time since last camera update (in seconds).
	 */
	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime);

	/** Update any attached camera lens effects **/
	virtual void UpdateCameraLensEffects( const FTViewTarget& OutVT );

public:
	/** Set a new ViewTarget with optional transition time */
	void SetViewTarget(class AActor* NewViewTarget, FViewTargetTransitionParams TransitionParams = FViewTargetTransitionParams());
	
	/** Give each modifier a chance to change view rotation/deltarot.  Base implementation also limits rotation components using LimitViewPitch, LimitViewYaw, and LimitViewRoll. */
	virtual void ProcessViewRotation(float DeltaTime, FRotator& OutViewRotation, FRotator& OutDeltaRot);

	/** Finds the first instance of a lens effect of the given class, using linear search. */
	virtual class AEmitterCameraLensEffectBase* FindCameraLensEffect(TSubclassOf<class AEmitterCameraLensEffectBase> LensEffectEmitterClass);
	
	/** Initiates a camera lens effect of the given class on this camera. */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	virtual AEmitterCameraLensEffectBase* AddCameraLensEffect(TSubclassOf<class AEmitterCameraLensEffectBase> LensEffectEmitterClass);
	
	/** Removes this particular lens effect from the camera. */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	virtual void RemoveCameraLensEffect(class AEmitterCameraLensEffectBase* Emitter);
	
	/** Removes all Camera Lens Effects. */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	virtual void ClearCameraLensEffects();

	//
	// Camera Shakes.
	//

	/** Play a camera shake */
	virtual void PlayCameraShake(TSubclassOf<class UCameraShake> Shake, float Scale, enum ECameraAnimPlaySpace::Type PlaySpace=ECameraAnimPlaySpace::CameraLocal, FRotator UserPlaySpaceRot = FRotator::ZeroRotator);
	
	/** Stop playing a camera shake. */
	virtual void StopCameraShake(TSubclassOf<class UCameraShake> Shake);

	// @todo document
	virtual void ClearAllCameraShakes();

	//
	//  CameraAnim support.
	//
	
	/**
	 * Play the indicated CameraAnim on this camera.
	 * 
	 * @param Anim The animation that should play on this instance.
	 * @param Rate				How fast to play the animation. 1.0 is normal.
	 * @param Scale				How "intense" to play the animation. 1.0 is normal.
	 * @param BlendInTime		Time to linearly ramp in.
	 * @param BlendOutTime		Time to linearly ramp out.
	 * @param bLoop				True to loop the animation if it hits the end.
	 * @param bRandomStartTime	Whether or not to choose a random time to start playing. Useful with bLoop=true and a duration to randomize things like shakes.
	 * @param Duration			Optional total playtime for this animation, including blends. 0 means to use animations natural duration, or infinite if looping.
	 * @param PlaySpace			Which space to play the animation in.
	 * @param UserPlaySpaceRot  Custom play space, used when PlaySpace is UserDefined.
	 * @return The CameraAnim instance, which can be stored to manipulate/stop the anim after the fact.
	 */
	UFUNCTION(BlueprintCallable, Category="Camera Animation")
	virtual class UCameraAnimInst* PlayCameraAnim(class UCameraAnim* Anim, float Rate=1.f, float Scale=1.f, float BlendInTime=0.f, float BlendOutTime=0.f, bool bLoop=false, bool bRandomStartTime=false, float Duration=0.f, ECameraAnimPlaySpace::Type PlaySpace=ECameraAnimPlaySpace::CameraLocal, FRotator UserPlaySpaceRot=FRotator::ZeroRotator);
	
	/**
	 * Stop playing all instances of the indicated CameraAnim.
	 * @param bImmediate	True to stop it right now and ignore blend out, false to let it blend out as indicated.
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Animation")
	virtual void StopAllInstancesOfCameraAnim(class UCameraAnim* Anim, bool bImmediate = false);
	
	/** 
	 * Stops the given CameraAnimInst from playing.  The given pointer should be considered invalid after this. 
	 * @param bImmediate	True to stop it right now and ignore blend out, false to let it blend out as indicated.
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Animation")
	virtual void StopCameraAnimInst(class UCameraAnimInst* AnimInst, bool bImmediate = false);
	
	/**
	 * Stop playing all CameraAnims on this CameraManager.
	 * @param bImmediate	True to stop it right now and ignore blend out, false to let it blend out as indicated.
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera Animation")
	virtual void StopAllCameraAnims(bool bImmediate = false);

	/** Returns first existing instance of the specified camera anim, or NULL if none exists. */
	UCameraAnimInst* FindInstanceOfCameraAnim(class UCameraAnim const* Anim) const;

protected:
	/** Gets specified temporary CameraActor ready to update the specified Anim. */
	void InitTempCameraActor(class ACameraActor* CamActor, class UCameraAnimInst const* AnimInstToInitFor) const;
	void ApplyAnimToCamera(class ACameraActor const* AnimatedCamActor, class UCameraAnimInst const* AnimInst, FMinimalViewInfo& InOutPOV);

	/** Returns an available CameraAnimInst, or NULL if no more are available. */
	UCameraAnimInst* AllocCameraAnimInst();

	/** Returns an available CameraAnimInst, or NULL if no more are available. */
	void ReleaseCameraAnimInst(UCameraAnimInst* Inst);

protected:
	/** Limit the player's view pitch.  */
	virtual void LimitViewPitch( FRotator& ViewRotation, float InViewPitchMin, float InViewPitchMax );

	/** Limit the player's view roll. */
	virtual void LimitViewRoll(FRotator& ViewRotation, float InViewRollMin, float InViewRollMax);

	/** Limit the player's view yaw. */
	virtual void LimitViewYaw(FRotator& ViewRotation, float InViewYawMin, float InViewYawMax);

	/**
	 * Query ViewTarget and updates the view target Point Of View.
	 * This is called from UpdateViewTarget under normal circumstances (target is not a camera actor and no debug cameras are active)
	 * Note: This function will eventually be replaced, but it currently provides a way for subclasses to override behavior without copy-pasting the special case code
	 *
	 * @param	OutVT		ViewTarget to use.
	 * @param	DeltaTime	Delta Time since last camera update (in seconds).
	 */
	virtual void UpdateViewTargetInternal(FTViewTarget& OutVT, float DeltaTime);

private:
	// Buried to prevent use; use GetCameraLocation instead
	FRotator GetActorRotation() const { return Super::GetActorRotation(); }

	// Buried to prevent use; use GetCameraRotation instead
	FVector GetActorLocation() const { return Super::GetActorLocation(); }

public:
	/** Returns TransformComponent subobject **/
	class USceneComponent* GetTransformComponent() const;
};
