// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Abstract base class of animation sequence that can be played and evaluated to produce a pose.
 *
 */

#include "AnimationAsset.h"
#include "SmartName.h"
#include "Skeleton.h"
#include "Animation/AnimCurveTypes.h"
#include "AnimSequenceBase.generated.h"

#define DEFAULT_SAMPLERATE			30.f
#define MINIMUM_ANIMATION_LENGTH	(1/DEFAULT_SAMPLERATE)

UENUM()
enum ETypeAdvanceAnim
{
	ETAA_Default,
	ETAA_Finished,
	ETAA_Looped
};

UCLASS(abstract, MinimalAPI, BlueprintType)
class UAnimSequenceBase : public UAnimationAsset
{
	GENERATED_UCLASS_BODY()

	/** Animation notifies, sorted by time (earliest notification first). */
	UPROPERTY()
	TArray<struct FAnimNotifyEvent> Notifies;

	/** Length (in seconds) of this AnimSequence if played back with a speed of 1.0. */
	UPROPERTY(Category=Length, AssetRegistrySearchable, VisibleAnywhere)
	float SequenceLength;

	/** Number for tweaking playback rate of this animation globally. */
	UPROPERTY(EditAnywhere, Category=Animation)
	float RateScale;
	
	/**
	 * Raw uncompressed float curve data 
	 */
	UPROPERTY()
	struct FRawCurveTracks RawCurveData;

#if WITH_EDITORONLY_DATA
	// if you change Notifies array, this will need to be rebuilt
	UPROPERTY()
	TArray<FAnimNotifyTrack> AnimNotifyTracks;
#endif // WITH_EDITORONLY_DATA

	// Begin UObject interface
	virtual void PostLoad() override;
	// End of UObject interface

	/** Returns the total play length of the montage, if played back with a speed of 1.0. */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	virtual float GetPlayLength();

	/** Sort the Notifies array by time, earliest first. */
	ENGINE_API void SortNotifies();	

	/** 
	 * Retrieves AnimNotifies given a StartTime and a DeltaTime.
	 * Time will be advanced and support looping if bAllowLooping is true.
	 * Supports playing backwards (DeltaTime<0).
	 * Returns notifies between StartTime (exclusive) and StartTime+DeltaTime (inclusive)
	 */
	void GetAnimNotifies(const float& StartTime, const float& DeltaTime, const bool bAllowLooping, TArray<const FAnimNotifyEvent *>& OutActiveNotifies) const;

	/** 
	 * Retrieves AnimNotifies between two time positions. ]PreviousPosition, CurrentPosition]
	 * Between PreviousPosition (exclusive) and CurrentPosition (inclusive).
	 * Supports playing backwards (CurrentPosition<PreviousPosition).
	 * Only supports contiguous range, does NOT support looping and wrapping over.
	 */
	ENGINE_API void GetAnimNotifiesFromDeltaPositions(const float& PreviousPosition, const float & CurrentPosition, TArray<const FAnimNotifyEvent *>& OutActiveNotifies) const;

	/** Evaluate curve data to Instance at the time of CurrentTime **/
	ENGINE_API virtual void EvaluateCurveData(FBlendedCurve& OutCurve, float CurrentTime) const;

	/**
	 * return true if this is valid additive animation
	 * false otherwise
	 */
	virtual bool IsValidAdditive() const { return false; }

#if WITH_EDITOR
	/** Return Number of Frames **/
	virtual int32 GetNumberOfFrames() const;

	/** Get the frame number for the provided time */
	virtual int32 GetFrameAtTime(const float Time) const;

	/** Get the time at the given frame */
	virtual float GetTimeAtFrame(const int32 Frame) const;

	// update anim notify track cache
	ENGINE_API void UpdateAnimNotifyTrackCache();
	
	// @todo document
	ENGINE_API void InitializeNotifyTrack();

	/** Fix up any notifies that are positioned beyond the end of the sequence */
	ENGINE_API void ClampNotifiesAtEndOfSequence();

	/** Calculates what (if any) offset should be applied to the trigger time of a notify given its display time */ 
	virtual EAnimEventTriggerOffsets::Type CalculateOffsetForNotify(float NotifyDisplayTime) const;

	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
	
	// Get a pointer to the data for a given Anim Notify
	ENGINE_API uint8* FindNotifyPropertyData(int32 NotifyIndex, UArrayProperty*& ArrayProperty);

#endif	//WITH_EDITORONLY_DATA

	// Begin UAnimationAsset interface
	virtual void TickAssetPlayerInstance(const FAnimTickRecord& Instance, class UAnimInstance* InstanceOwner, FAnimAssetTickContext& Context) const override;
	// this is used in editor only when used for transition getter
	// this doesn't mean max time. In Sequence, this is SequenceLength,
	// but for BlendSpace CurrentTime is normalized [0,1], so this is 1
	virtual float GetMaxCurrentTime() override { return SequenceLength; }
	// End of UAnimationAsset interface

	/**
	* Get Bone Transform of the Time given, relative to Parent for all RequiredBones
	* This returns different transform based on additive or not. Or what kind of additive.
	*
	* @param	OutPose				Pose object to fill
	* @param	OutCurve			Curves to fill
	* @param	ExtractionContext	Extraction Context (position, looping, root motion, etc.)
	*/
	ENGINE_API virtual void GetAnimationPose(struct FCompactPose& OutPose, FBlendedCurve& OutCurve, const FAnimExtractContext& ExtractionContext) const PURE_VIRTUAL(UAnimSequenceBase::GetAnimationPose, );
	
	virtual void OnAssetPlayerTickedInternal(FAnimAssetTickContext &Context, const float PreviousTime, const float MoveDelta, const FAnimTickRecord &Instance, class UAnimInstance* InstanceOwner) const;

	virtual bool HasRootMotion() const { return false; }

	virtual void Serialize(FArchive& Ar) override;

#if WITH_EDITOR
private:
	DECLARE_MULTICAST_DELEGATE( FOnNotifyChangedMulticaster );
	FOnNotifyChangedMulticaster OnNotifyChanged;

public:
	typedef FOnNotifyChangedMulticaster::FDelegate FOnNotifyChanged;

	/** Registers a delegate to be called after notification has changed*/
	ENGINE_API void RegisterOnNotifyChanged(const FOnNotifyChanged& Delegate);
	ENGINE_API void UnregisterOnNotifyChanged(void* Unregister);
	ENGINE_API virtual bool IsValidToPlay() const { return true; }

#endif

protected:
	template <typename DataType>
	void VerifyCurveNames(USkeleton* Skeleton, const FName& NameContainer, TArray<DataType>& CurveList);
};
