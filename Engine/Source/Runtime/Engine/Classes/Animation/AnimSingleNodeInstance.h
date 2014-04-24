// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 *
 * This Instance only contains one AnimationAsset, and produce poses
 * Used by Preview in AnimGraph, Playing single animation in Kismet2 and etc
 */

#pragma once
#include "AnimSingleNodeInstance.generated.h"

DECLARE_DYNAMIC_DELEGATE(FPostEvaluateAnimEvent);

UCLASS(transient)
class ENGINE_API UAnimSingleNodeInstance : public UAnimInstance
{
	GENERATED_UCLASS_BODY()

	/** Current Asset being played **/
	UPROPERTY(transient)
	class UAnimationAsset* CurrentAsset;

	UPROPERTY(transient)
	class UVertexAnimation* CurrentVertexAnim;

	/** Random cached values to play each asset **/
	UPROPERTY(transient)
	FVector BlendSpaceInput;

	/** Random cached values to play each asset **/
	UPROPERTY(transient)
	TArray<FBlendSampleData> BlendSampleData;

	/** Random cached values to play each asset **/
	UPROPERTY(transient)
	FBlendFilter BlendFilter;

	/** Shared parameters for previewing blendspace or animsequence **/
	UPROPERTY(transient)
	float CurrentTime;

	UPROPERTY(transient)
	float PlayRate;
	 
	UPROPERTY(Transient)
	FPostEvaluateAnimEvent PostEvaluateAnimEvent;

	UPROPERTY(transient)
	uint32 bLooping:1;

	UPROPERTY(transient)
	uint32 bPlaying:1;

	UPROPERTY(transient)
	uint32 bReverse:1;

	// Begin UAnimInstance interface
	virtual void NativeInitializeAnimation() OVERRIDE;
	virtual void NativeUpdateAnimation(float DeltaTimeX) OVERRIDE;
	virtual bool NativeEvaluateAnimation(FPoseContext& Output) OVERRIDE;
protected:
	virtual void Montage_Advance(float DeltaTime) OVERRIDE;
	void InternalBlendSpaceEvaluatePose(class UBlendSpaceBase* BlendSpace, TArray<FBlendSampleData>& BlendSampleDataCache, struct FA2Pose& Pose, bool bIsLooping);
	// End UAnimInstance interface
public:

	UFUNCTION(BlueprintCallable, Category="Animation")
	void SetLooping(bool bIsLooping);
	UFUNCTION(BlueprintCallable, Category="Animation")
	void SetPlayRate(float InPlayRate);
	UFUNCTION(BlueprintCallable, Category="Animation")
	void SetReverse(bool bInReverse);
	UFUNCTION(BlueprintCallable, Category="Animation")
	void SetPosition(float InPosition, bool bFireNotifies=true);
	UFUNCTION(BlueprintCallable, Category="Animation")
	void SetBlendSpaceInput(const FVector & InBlendInput);
	UFUNCTION(BlueprintCallable, Category="Animation")
	void SetPlaying(bool bIsPlaying);
	UFUNCTION(BlueprintCallable, Category="Animation")
	float GetLength();
	/* For AnimSequence specific **/
	UFUNCTION(BlueprintCallable, Category="Animation")
	void PlayAnim(bool bIsLooping=false, float InPlayRate=1.f, float InStartPosition=0.f);
	UFUNCTION(BlueprintCallable, Category="Animation")
	void StopAnim();
	/** Set New Asset - calls InitializeAnimation, for now we need MeshComponent **/
	UFUNCTION(BlueprintCallable, Category="Animation")
	virtual void SetAnimationAsset(UAnimationAsset* NewAsset, bool bIsLooping=true, float InPlayRate=1.f);
	/** Set new vertex animation */
	UFUNCTION(BlueprintCallable, Category="Animation")
	void SetVertexAnimation(UVertexAnimation* NewVertexAnim, bool bIsLooping=true, float InPlayRate=1.f);

public:
	/** AnimSequence specific **/
	void StepForward();
	void StepBackward();

	/** custom evaluate pose **/
	virtual void RestartMontage(UAnimMontage * Montage, FName FromSection = FName());
	void SetMontageLoop(UAnimMontage* Montage, bool bIsLooping, FName StartingSection = FName());

#if WITH_EDITORONLY_DATA
	float PreviewPoseCurrentTime;
#endif
};



