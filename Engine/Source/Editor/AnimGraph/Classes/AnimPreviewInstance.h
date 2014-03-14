// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlueprintGraphDefinitions.h"
#include "AnimGraphNode_ModifyBone.h"
#include "AnimPreviewInstance.generated.h"

/** Enum to know how montage is being played */
UENUM()
enum EMontagePreviewType
{
	/** Playing montage in usual way. */
	EMPT_Normal, 
	/** Playing all sections. */
	EMPT_AllSections,
	EMPT_MAX,
};

/**
 * This Instance only contains one AnimationAsset, and produce poses
 * Used by Preview in AnimGraph, Playing single animation in Kismet2 and etc
 */

UCLASS(transient, noteditinlinenew)
class ANIMGRAPH_API UAnimPreviewInstance : public UAnimSingleNodeInstance
{
	GENERATED_UCLASS_BODY()

	/** Current Asset being played **/
	UPROPERTY(transient)
	FAnimNode_ModifyBone SingleBoneController;

	/** Shared parameters for previewing blendspace or animsequence **/
	UPROPERTY(transient)
	float SkeletalControlAlpha;

	/** Shared parameters for previewing blendspace or animsequence **/
	UPROPERTY(transient)
	TEnumAsByte<enum EMontagePreviewType> MontagePreviewType;

	UPROPERTY(transient)
	int32 MontagePreviewStartSectionIdx;

	// Begin UAnimInstance interface
	virtual void NativeInitializeAnimation() OVERRIDE;
	virtual bool NativeEvaluateAnimation(FPoseContext& Output) OVERRIDE;
	// End UAnimInstance interface

	/** Set SkeletalControl Alpha**/
	void SetSkeletalControlAlpha(float SkeletalControlAlpha);

	UAnimSequence* GetAnimSequence();

	// Begin UAnimSingleNodeInstance interface
	virtual void RestartMontage(UAnimMontage* Montage, FName FromSection = FName()) OVERRIDE;
	// End UAnimSingleNodeInstance interface

	/** Montage preview functions */
	void MontagePreview_JumpToStart();
	void MontagePreview_JumpToEnd();
	void MontagePreview_JumpToPreviewStart();
	void MontagePreview_Restart();
	void MontagePreview_PreviewNormal(int32 FromSectionIdx = INDEX_NONE);
	void MontagePreview_SetLoopNormal(bool bIsLooping, int32 PreferSectionIdx = INDEX_NONE);
	void MontagePreview_PreviewAllSections();
	void MontagePreview_SetLoopAllSections(bool bIsLooping);
	void MontagePreview_SetLoopAllSetupSections(bool bIsLooping);
	void MontagePreview_ResetSectionsOrder();
	void MontagePreview_SetLooping(bool bIsLooping);
	void MontagePreview_SetPlaying(bool bIsPlaying);
	void MontagePreview_SetReverse(bool bInReverse);
	void MontagePreview_StepForward();
	void MontagePreview_StepBackward();
	void MontagePreview_JumpToPosition(float NewPosition);
	int32 MontagePreview_FindFirstSectionAsInMontage(int32 AnySectionIdx);
	int32 MontagePreview_FindLastSection(int32 StartSectionIdx);
	float MontagePreview_CalculateStepLength();
	void MontagePreview_RemoveBlendOut();
	bool IsPlayingMontage() { return GetActiveMontageInstance() != NULL; }
};



