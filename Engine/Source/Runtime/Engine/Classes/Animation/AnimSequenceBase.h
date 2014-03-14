// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Abstract base class of animation sequence that can be played and evaluated to produce a pose.
 *
 */

#include "AnimationAsset.h"
#include "AnimSequenceBase.generated.h"

#define DEFAULT_SAMPLERATE			30.f
#define MINIMUM_ANIMATION_LENGTH	(1/DEFAULT_SAMPLERATE)

namespace EAnimEventTriggerOffsets
{
	enum Type
	{
		OffsetBefore,
		OffsetAfter,
		NoOffset
	};
}

ENGINE_API float GetTriggerTimeOffsetForType(EAnimEventTriggerOffsets::Type OffsetType);

/*
 * Triggers an animation notify.  Each AnimNotifyEvent contains an AnimNotify object
 * which has its Notify method called and passed to the animation.
 */
USTRUCT()
struct FAnimNotifyEvent
{
	GENERATED_USTRUCT_BODY()

	/** The user requested time for this notify */
	UPROPERTY()
	float DisplayTime;

	/** An offset from the DisplayTime to the actual time we will trigger the notify, as we cannot always trigger it exactly at the time the user wants */
	UPROPERTY()
	float TriggerTimeOffset;

	/** An offset similar to TriggerTimeOffset but used for the end scrub handle of a notify state's duration */
	UPROPERTY()
	float EndTriggerTimeOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=AnimNotifyEvent)
	float TriggerWeightThreshold;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=AnimNotifyEvent)
	FName NotifyName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=AnimNotifyEvent)
	class UAnimNotify * Notify;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=AnimNotifyEvent)
	class UAnimNotifyState * NotifyStateClass;

	UPROPERTY()
	float Duration;

#if WITH_EDITORONLY_DATA
	/** Color of Notify in editor */
	UPROPERTY()
	FColor NotifyColor;

	/** Visual position of notify in the vertical 'tracks' of the notify editor panel */
	UPROPERTY()
	int32 TrackIndex;

	/** If notify is selected in editor */
	UPROPERTY(Transient)
	bool bSelected;

#endif // WITH_EDITORONLY_DATA

	FAnimNotifyEvent()
		: DisplayTime(0)
		, TriggerTimeOffset(0)
		, EndTriggerTimeOffset(0)
		, TriggerWeightThreshold(ZERO_ANIMWEIGHT_THRESH)
#if WITH_EDITORONLY_DATA
		, Notify(NULL)
#endif // WITH_EDITORONLY_DATA
		, NotifyStateClass(NULL)
		, Duration(0)
#if WITH_EDITORONLY_DATA
		, TrackIndex(0)
		, bSelected(false)
#endif // WITH_EDITORONLY_DATA
	{
	}

	/** Updates trigger offset based on a combination of predicted offset and current offset */
	ENGINE_API void RefreshTriggerOffset(EAnimEventTriggerOffsets::Type PredictedOffsetType);

	/** Updates end trigger offset based on a combination of predicted offset and current offset */
	ENGINE_API void RefreshEndTriggerOffset(EAnimEventTriggerOffsets::Type PredictedOffsetType);

	/** Returns the actual trigger time for this notify. In some cases this may be different to the DisplayTime that has been set */
	ENGINE_API float GetTriggerTime() const;

	/** Returns the actual end time for a state notify. In some cases this may be different from DisplayTime + Duration */
	ENGINE_API float GetEndTriggerTime() const;

	/** Returns true if this is blueprint derived notifies **/
	bool IsBlueprintNotify() const
	{
		return Notify != NULL || NotifyStateClass != NULL;
	}
};

/**
 * Keyframe position data for one track.  Pos(i) occurs at Time(i).  Pos.Num() always equals Time.Num().
 */
USTRUCT()
struct FAnimNotifyTrack
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FName TrackName;

	UPROPERTY()
	FLinearColor TrackColor;

	TArray<FAnimNotifyEvent*> Notifies;

	FAnimNotifyTrack()
		: TrackName(TEXT(""))
		, TrackColor(FLinearColor::White)
	{
	}

	FAnimNotifyTrack(FName InTrackName, FLinearColor InTrackColor)
		: TrackName(InTrackName)
		, TrackColor(InTrackColor)
	{
	}
};

/**
 * Float curve data for one track
 */

USTRUCT()
struct FAnimCurveBase
{
	GENERATED_USTRUCT_BODY()

	/** Name of curve. - in the future, we might need TMap, right now TMap doesn't support property serialization, but in the future we can change this to TMap*/
	UPROPERTY()
	FName		CurveName;

	FAnimCurveBase(){}

	FAnimCurveBase(FName InName)
		: CurveName(InName)
	{	
	}
};

/** native enum for curve types **/
enum EAnimCurveFlags
{
	// Used as morph target curve
	ACF_DrivesMorphTarget	= 0x00000001,
	// Used as triggering event
	ACF_TriggerEvent		= 0x00000002,
	// Is editable in Sequence Editor
	ACF_Editable			= 0x00000004,
	// Used as a material curve
	ACF_DrivesMaterial		= 0x00000008,

	// default flag when created
	ACF_DefaultCurve		= ACF_TriggerEvent | ACF_Editable,
	// curves created from Morph Target
	ACF_MorphTargetCurve	= ACF_DrivesMorphTarget
};

USTRUCT()
struct FFloatCurve : public FAnimCurveBase
{
	GENERATED_USTRUCT_BODY()

	/** Curve data for float. */
	UPROPERTY()
	FRichCurve	FloatCurve;

	FFloatCurve(){}

	FFloatCurve(FName InName, int32 InCurveTypeFlags)
		: FAnimCurveBase(InName)
		, CurveTypeFlags(InCurveTypeFlags)
	{
	}

	/**
	 * Set InFlag to bValue
	 */
	ENGINE_API void SetCurveTypeFlag(EAnimCurveFlags InFlag, bool bValue);
	/**
	 * Return true if InFlag is set, false otherwise 
	 */
	ENGINE_API bool GetCurveTypeFlag(EAnimCurveFlags InFlag) const;

	/**
	 * Set CurveTypeFlags to NewCurveTypeFlags
	 * This just overwrites CurveTypeFlags
	 */
	void SetCurveTypeFlags(int32 NewCurveTypeFlags);
	/** 
	 * returns CurveTypeFlags
	 */
	int32 GetCurveTypeFlags() const;

private:

	/** Curve Type Flags */
	UPROPERTY()
	int32		CurveTypeFlags;
};

/**
 * Raw Curve data for serialization
 */
USTRUCT()
struct FRawCurveTracks
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FFloatCurve>		FloatCurves;

	/**
	 * Evaluate curve data at the time CurrentTime, and add to Instance
	 */
	void EvaluateCurveData(class UAnimInstance* Instance, float CurrentTime, float BlendWeight ) const;
	/**
	 * Find curve data based on name, you give pointer
	 */
	ENGINE_API FFloatCurve * GetCurveData(FName InCurveName);
	/**
	 * Add new curve named InCurveName and return true if success
	 * bVectorInterpCurve == true, then it will create FVectorCuve, otherwise, FFloatCurve
	 */
	ENGINE_API bool AddCurveData(FName InCurveName, int32 CurveFlags = ACF_DefaultCurve);
	/*
	 * Delete curve data 
	 */
	ENGINE_API bool DeleteCurveData(FName InCurveName);
};

UENUM()
enum ETypeAdvanceAnim
{
	ETAA_Default,
	ETAA_Finished,
	ETAA_Looped
};

UCLASS(abstract, MinimalAPI, dependson=UCurveBase, BlueprintType)
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
	FRawCurveTracks RawCurveData;

#if WITH_EDITORONLY_DATA
	// if you change Notifies array, this will need to be rebuilt
	UPROPERTY()
	TArray<FAnimNotifyTrack> AnimNotifyTracks;
#endif // WITH_EDITORONLY_DATA

	// Begin UObject interface
	virtual void PostLoad() OVERRIDE;
	// End of UObject interface

	/** Sort the Notifies array by time, earliest first. */
	ENGINE_API void SortNotifies();	

	/** 
	 * Retrieves AnimNotifies given a StartTime and a DeltaTime.
	 * Time will be advanced and support looping if bAllowLooping is true.
	 * Supports playing backwards (DeltaTime<0).
	 * Returns notifies between StartTime (exclusive) and StartTime+DeltaTime (inclusive)
	 */
	void GetAnimNotifies(const float & StartTime, const float & DeltaTime, const float bAllowLooping, TArray<const FAnimNotifyEvent *> & OutActiveNotifies) const;

	/** 
	 * Retrieves AnimNotifies between two time positions. ]PreviousPosition, CurrentPosition]
	 * Between PreviousPosition (exclusive) and CurrentPosition (inclusive).
	 * Supports playing backwards (CurrentPosition<PreviousPosition).
	 * Only supports contiguous range, does NOT support looping and wrapping over.
	 */
	void GetAnimNotifiesFromDeltaPositions(const float & PreviousPosition, const float & CurrentPosition, TArray<const FAnimNotifyEvent *> & OutActiveNotifies) const;

	/** Evaluate curve data to Instance at the time of CurrentTime **/
	ENGINE_API virtual void EvaluateCurveData(class UAnimInstance* Instance, float CurrentTime, float BlendWeight ) const;

	/**
	 * return true if this is valid additive animation
	 * false otherwise
	 */
	virtual bool IsValidAdditive() const { return false; }

#if WITH_EDITOR
	/** Return Number of Frames **/
	virtual int32 GetNumberOfFrames();

	// update anim notify track cache
	ENGINE_API void UpdateAnimNotifyTrackCache();
	
	// @todo document
	ENGINE_API void InitializeNotifyTrack();

	/** Fix up any notifies that are positioned beyond the end of the sequence */
	ENGINE_API void ClampNotifiesAtEndOfSequence();

	/** Calculates what (if any) offset should be applied to the trigger time of a notify given its display time */ 
	virtual EAnimEventTriggerOffsets::Type CalculateOffsetForNotify(float NotifyDisplayTime) const;

	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const OVERRIDE;

#endif	//WITH_EDITORONLY_DATA

	/**
	 * Update Morph Target Data type to new Curve Type
	 */
	ENGINE_API virtual void UpgradeMorphTargetCurves();

	// Begin UAnimationAsset interface
	virtual void TickAssetPlayerInstance(const FAnimTickRecord& Instance, class UAnimInstance* InstanceOwner, FAnimAssetTickContext& Context) const OVERRIDE;
	// this is used in editor only when used for transition getter
	// this doesn't mean max time. In Sequence, this is SequenceLength,
	// but for BlendSpace CurrentTime is normalized [0,1], so this is 1
	virtual float GetMaxCurrentTime() OVERRIDE { return SequenceLength; }
	// End of UAnimationAsset interface

protected:
	virtual void ExtractRootTrack(float Pos, FTransform & RootTransform, const FBoneContainer * RequiredBones) const;

#if WITH_EDITOR
private:
	DECLARE_MULTICAST_DELEGATE( FOnNotifyChangedMulticaster );
	FOnNotifyChangedMulticaster OnNotifyChanged;

public:
	typedef FOnNotifyChangedMulticaster::FDelegate FOnNotifyChanged;

	/** Registers a delegate to be called after notification has changed*/
	ENGINE_API void RegisterOnNotifyChanged(const FOnNotifyChanged& Delegate);
	ENGINE_API void UnregisterOnNotifyChanged(void * Unregister);
#endif
};
