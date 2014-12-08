// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "CameraModifier.generated.h"

UCLASS()
class ENGINE_API UCameraModifier : public UObject
{
	GENERATED_UCLASS_BODY()

protected:
	/* Interface class for all camera modifiers applied to
		a player camera actor. Sub classes responsible for
		implementing declared functions.
	*/
	
	/* Do not apply this modifier */
	UPROPERTY()
	uint32 bDisabled:1;

public:
	/* This modifier is still being applied and will disable itself */
	UPROPERTY()
	uint32 bPendingDisable:1;

	/** Camera this object is attached to */
	UPROPERTY()
	class APlayerCameraManager* CameraOwner;

protected:
	/**
	 * Priority of this modifier - determines where it is added in the modifier list.
	 * 0 = highest priority, 255 = lowest 
	 */
	UPROPERTY()
	uint8 Priority;

	/** This modifier can only be used exclusively - no modifiers of same priority allowed */
	UPROPERTY()
	uint32 bExclusive:1;

	/** When blending in, alpha proceeds from 0 to 1 over this time */
	UPROPERTY()
	float AlphaInTime;

	/** When blending out, alpha proceeds from 1 to 0 over this time */
	UPROPERTY()
	float AlphaOutTime;

	/** Current blend alpha */
	UPROPERTY(transient)
	float Alpha;

	/** Desired alpha we are interpolating towards. */
	UPROPERTY(transient)
	float TargetAlpha;

public:
	//debug
	UPROPERTY(EditAnywhere, Category=Debug)
	uint32 bDebug:1;


protected:
	virtual float GetTargetAlpha(class APlayerCameraManager* Camera);
public:
	virtual void GetDebugText(TArray<FString>& Lines) {}

	/** Allow anything to happen right after creation */
	virtual void Init( APlayerCameraManager* Camera );
	
	/**
	 * Directly modifies variables in the camera actor
	 *
	 * @param	Camera		reference to camera actor we are modifying
	 * @param	DeltaTime	Change in time since last update
	 * @param	InOutPOV		current Point of View, to be updated.
	 * @return	bool		true if should STOP looping the chain, false otherwise
	 */
	virtual bool ModifyCamera(APlayerCameraManager* Camera, float DeltaTime, struct FMinimalViewInfo& InOutPOV);
	
	/** Accessor function to check if modifier is inactive */
	virtual bool IsDisabled() const;
	
	/**
	 * Camera modifier evaluates itself vs the given camera's modifier list
	 * and decides whether to add itself or not. Handles adding by priority and avoiding 
	 * adding the same modifier twice.
	 *
	 * @param	Camera - reference to camera actor we want add this modifier to
	 * @return	bool   - true if modifier added to camera's modifier list, false otherwise
	 */
	virtual bool AddCameraModifier( APlayerCameraManager* Camera );
	
	/**
	 * Camera modifier removes itself from given camera's modifier list
	 *
	 * @param	Camera	- reference to camara actor we want to remove this modifier from
	 * @return	bool	- true if modifier removed successfully, false otherwise
	 */
	virtual bool RemoveCameraModifier( APlayerCameraManager* Camera );
	
	/** 
	 *  Accessor functions for changing disable flag
	 *  
	 *  @param  bImmediate  - true to disable with no blend out, false (default) to allow blend out
	 */
	virtual void DisableModifier(bool bImmediate = false);
	virtual void EnableModifier();
	virtual void ToggleModifier();
	
	/**
	 * Allow this modifier a chance to change view rotation and deltarot
	 * Default just returns ViewRotation unchanged
	 * @return	bool - true if should stop looping modifiers to adjust rotation, false otherwise
	 */
	virtual bool ProcessViewRotation(class AActor* ViewTarget, float DeltaTime, FRotator& OutViewRotation, FRotator& OutDeltaRot);

	/**
	 * Responsible for updating alpha blend value.
	 *
	 * @param	Camera		- Camera that is being updated
	 * @param	DeltaTime	- Amount of time since last update
	 */
	virtual void UpdateAlpha( APlayerCameraManager* Camera, float DeltaTime );

	/** Return a world context */
	UWorld* GetWorld() const;
		
};



