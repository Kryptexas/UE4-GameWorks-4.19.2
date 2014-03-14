// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "InterpTrackAnimControl.generated.h"

/** Structure used for holding information for one animation played on the Anim Control track. */
USTRUCT()
struct FAnimControlTrackKey
{
	GENERATED_USTRUCT_BODY()

	/** Position in the Matinee sequence to start playing this animation. */
	UPROPERTY()
	float StartTime;

	/** Animation Sequence to play */
	UPROPERTY()
	class UAnimSequence* AnimSeq;

	/** Time to start playing AnimSequence at. */
	UPROPERTY()
	float AnimStartOffset;

	/** Time to end playing the AnimSequence at. */
	UPROPERTY()
	float AnimEndOffset;

	/** Playback speed of this animation. */
	UPROPERTY()
	float AnimPlayRate;

	/** Should this animation loop. */
	UPROPERTY()
	uint32 bLooping:1;

	/** Whether to play the animation in reverse or not. */
	UPROPERTY()
	uint32 bReverse:1;


	FAnimControlTrackKey()
		: StartTime(0)
		, AnimSeq(NULL)
		, AnimStartOffset(0)
		, AnimEndOffset(0)
		, AnimPlayRate(0)
		, bLooping(false)
		, bReverse(false)
	{
	}

};

UCLASS(HeaderGroup=Interpolation, MinimalAPI, meta=( DisplayName = "Anim Track" ) )
class UInterpTrackAnimControl : public UInterpTrackFloatBase
{
	GENERATED_UCLASS_BODY()
	
	/** 
	 *	Name of slot to use when playing animation. Passed to Actor. 
	 *	When multiple tracks use the same slot name, they are each given a different ChannelIndex when SetAnimPosition is called. 
	 */
	UPROPERTY(EditAnywhere, Category=InterpTrackAnimControl)
	FName SlotName;

	/** Track of different animations to play and when to start playing them. */
	UPROPERTY()
	TArray<struct FAnimControlTrackKey> AnimSeqs;

	/**  Skip all anim notifiers **/
	UPROPERTY(EditAnywhere, Category=InterpTrackAnimControl)
	uint32 bSkipAnimNotifiers:1;



	// Begin UObject interface.
	virtual void PostLoad();
	// End UObject interface.

	// Begin UInterpTrack interface.
	virtual int32 GetNumKeyframes() const OVERRIDE;
	virtual void GetTimeRange(float& StartTime, float& EndTime) const OVERRIDE;
	virtual float GetTrackEndTime() const OVERRIDE;
	virtual float GetKeyframeTime(int32 KeyIndex) const OVERRIDE;
	virtual int32 GetKeyframeIndex( float KeyTime ) const OVERRIDE;
	virtual int32 AddKeyframe(float Time, UInterpTrackInst* TrInst, EInterpCurveMode InitInterpMode) OVERRIDE;
	virtual int32 SetKeyframeTime(int32 KeyIndex, float NewKeyTime, bool bUpdateOrder=true) OVERRIDE;
	virtual void RemoveKeyframe(int32 KeyIndex) OVERRIDE;
	virtual int32 DuplicateKeyframe(int32 KeyIndex, float NewKeyTime, UInterpTrack* ToTrack = NULL) OVERRIDE;
	virtual bool GetClosestSnapPosition(float InPosition, TArray<int32> &IgnoreKeys, float& OutPosition) OVERRIDE;
	virtual FColor GetKeyframeColor(int32 KeyIndex) const OVERRIDE;
	virtual void PreviewUpdateTrack(float NewPosition, class UInterpTrackInst* TrInst) OVERRIDE;
	virtual void UpdateTrack(float NewPosition, UInterpTrackInst* TrInst, bool bJump) OVERRIDE;
	virtual const FString	GetEdHelperClassName() const OVERRIDE;
	virtual const FString	GetSlateHelperClassName() const OVERRIDE;
	virtual class UTexture2D* GetTrackIcon() const OVERRIDE;
	virtual void DrawTrack( FCanvas* Canvas, UInterpGroup* Group, const FInterpTrackDrawParams& Params ) OVERRIDE;
	// End UInterpTrack interface.

	// Begin FInterpEdInputInterface Interface
	virtual void BeginDrag(FInterpEdInputData &InputData) OVERRIDE;
	virtual void EndDrag(FInterpEdInputData &InputData) OVERRIDE;
	virtual EMouseCursor::Type GetMouseCursor(FInterpEdInputData &InputData) OVERRIDE;
	virtual void ObjectDragged(FInterpEdInputData& InputData) OVERRIDE;
	// End FInterpEdInputInterface Interface

	/** 
	 * Calculates the reversed time for a sequence key, if the key has bReverse set.
	 *
	 * @param SeqKey		Key that is reveresed and we are trying to find a position for.
	 * @param Seq			Anim sequence the key represents.  If NULL, the function will lookup the sequence.
	 * @param InPosition	Timeline position that we are currently at.
	 *
	 * @return Returns the position in the specified seq key. 
	 */
	float ConditionallyReversePosition(FAnimControlTrackKey &SeqKey, UAnimSequence* Seq, float InPosition);

	
	/** 
	 * Find the animation name and position for the given point in the track timeline. 
	 * @return true if it needs the animation to advance timer (from Previous to Current Time for Root Motion)
	 */
	bool GetAnimForTime(float InTime, UAnimSequence** OutAnimSequencePtr, float& OutPosition, bool& bOutLooping);

	/** Get the strength that the animation from this track should be blended in with at the give time. */
	float GetWeightForTime(float InTime);

	/** 
	 *	Utility to split the animation we are currently over into two pieces at the current position. 
	 *	InPosition is position in the entire Matinee sequence.
	 *	Returns the index of the newly created key.
	 */
	ENGINE_API int32 SplitKeyAtPosition(float InPosition);

	/**
	 * Crops the key at the position specified, by deleting the area of the key before or after the position.
	 *
	 * @param InPosition				Position to use as a crop point.
	 * @param bCutAreaBeforePosition	Whether we should delete the area of the key before the position specified or after.
	 *
	 * @return Returns the index of the key that was cropped.
	 */
	ENGINE_API int32 CropKeyAtPosition(float InPosition, bool bCutAreaBeforePosition);


	/** Calculate the index of this Track within its Slot (for when multiple tracks are using same slot). */
	int32 CalcChannelIndex();
};



