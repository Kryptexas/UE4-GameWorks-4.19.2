// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Abstract base class of animation made of multiple sequences.
 *
 */

#include "Delegate.h"
#include "AnimCompositeBase.h"
#include "AnimMontage.generated.h"

/**
 * Section data for each track. Reference of data will be stored in the child class for the way they want
 * AnimComposite vs AnimMontage have different requirement for the actual data reference
 * This only contains composite section information. (vertical sequences)
 */
USTRUCT()
struct FCompositeSection
{
	GENERATED_USTRUCT_BODY()

	/** Section Name */
	UPROPERTY(EditAnywhere, Category=Section)
	FName SectionName;

	/** Start Time **/
	UPROPERTY()
	float StarTime_DEPRECATED;

	/** Start Time **/
	UPROPERTY(EditAnywhere, Category=Section)
	float StartTime;

	/** Should this animation loop. */
	UPROPERTY(EditAnywhere, Category=Section)
	FName NextSectionName;

	FCompositeSection()
		: SectionName(NAME_None)
		, StarTime_DEPRECATED(0.f)
		, StartTime(0.f)
		, NextSectionName(NAME_None)
	{
	}

};

/**
 * Each slot data referenced by Animation Slot 
 * contains slot name, and animation data 
 */
USTRUCT()
struct FSlotAnimationTrack
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category=Slot)
	FName SlotName;

	UPROPERTY(EditAnywhere, Category=Slot)
	FAnimTrack AnimTrack;

	FSlotAnimationTrack()
		: SlotName(NAME_None)
	{}

};

/** 
 * Special event handler for branching point
 * This is different from Notifies since they handle multiple calls in a frame
 * when you can modify positions when it gets called, and Tick will handle correctly
 * however that limits multi threading, so this is expensive
 * Use notifies if possible
 */
USTRUCT()
struct FBranchingPoint
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category=BranchingPoint)
	FName EventName;

	UPROPERTY(EditAnywhere, Category=BranchingPoint)
	float DisplayTime;

	/** An offset from the DisplayTime to the actual time we will trigger the notify, as we cannot always trigger it exactly at the time the user wants */
	UPROPERTY()
	float TriggerTimeOffset;

	/** Returns the time this branching point should be triggered */
	float GetTriggerTime() const { return DisplayTime + TriggerTimeOffset; }

	/** Updates trigger offset based on a combination of predicted offset and current offset */
	ENGINE_API void RefreshTriggerOffset(EAnimEventTriggerOffsets::Type PredictedOffsetType);
};

/**
 * Delegate for when Montage is completed, whether interrupted or finished
 * Weight of this montage is 0.f, so it stops contributing to output pose
 *
 * bInterrupted = true if it was not property finished
 */
DECLARE_DELEGATE_TwoParams( FOnMontageEnded, class UAnimMontage*, bool /*bInterrupted*/) 
/**
 * Delegate for when Montage started to blend out, whether interrupted or finished
 * DesiredWeight of this montage becomes 0.f, but this still contributes to the output pose
 * 
 * bInterrupted = true if it was not property finished
 */
DECLARE_DELEGATE_TwoParams( FOnMontageBlendingOutStarted, class UAnimMontage*, bool /*bInterrupted*/) 

USTRUCT()
struct FAnimMontageInstance
{
	GENERATED_USTRUCT_BODY()

	// Montage reference
	UPROPERTY()
	class UAnimMontage* Montage;

public: 
	UPROPERTY()
	float DesiredWeight;

	UPROPERTY()
	float Weight;

	UPROPERTY(transient)
	float BlendTime;

	// Blend Time multiplier to allow extending and narrowing blendtimes
	UPROPERTY(transient)
	float DefaultBlendTimeMultiplier;

	// list of next sections per section - index of array is section id
	UPROPERTY()
	TArray<int32> NextSections;

	// list of prev sections per section - index of array is section id
	UPROPERTY()
	TArray<int32> PrevSections;

	UPROPERTY()
	float Position;

	UPROPERTY()
	float PlayRate;

	UPROPERTY()
	bool bPlaying;

	// delegates
	FOnMontageEnded OnMontageEnded;
	FOnMontageBlendingOutStarted OnMontageBlendingOutStarted;

	// reference to AnimInstance
	TWeakObjectPtr<UAnimInstance> AnimInstance;

	// need to save if it's interrupted or not
	// this information is crucial for gameplay
	bool bInterrupted;

	// transient PreviousWeight - Weight of previous tick
	float PreviousWeight;

public:
	FAnimMontageInstance()
		: Montage(NULL)
		, DesiredWeight(0.f)
		, Weight(0.f)
		, BlendTime(0.f)
		, DefaultBlendTimeMultiplier(1.0f)
		, Position(0.f)
		, PlayRate(1.f)
		, bPlaying(false)		
		, AnimInstance(NULL)
		, bInterrupted(false)
		, PreviousWeight(0.f)
	{
	}

	FAnimMontageInstance(UAnimInstance * InAnimInstance)
		: Montage(NULL)
		, DesiredWeight(0.f)
		, Weight(0.f)
		, BlendTime(0.f)
		, DefaultBlendTimeMultiplier(1.0f)
		, Position(0.f)
		, PlayRate(1.f)
		, bPlaying(false)		
		, AnimInstance(InAnimInstance)
		, bInterrupted(false)
		, PreviousWeight(0.f)		
	{
	}

	bool ChangeNextSection(FName SectionName, FName NextSectionName);

	// montage instance interfaces
	void Play(float InPlayRate = 1.f);
	void Stop(float BlendOut, bool bInterrupt=true);
	void Pause();
	void Initialize(class UAnimMontage * InMontage);

	bool ChangePositionToSection(FName SectionName, bool bEndOfSection = false);

	bool IsValid() const { return (Montage!=NULL); }
	bool IsPlaying() const { return IsValid() && bPlaying; }

	void Terminate();
	/**
	 * Montage Tick happens in 2 phases
	 *
	 * first is to update weight of current montage only
	 * this will make sure that all nodes will get up-to-date weight information
	 * when update comes in for them
	 *
	 * second is normal tick. This tick has to happen later when all node ticks
	 * to accumulate and update curve data/notifies/branching points
	 */
	void UpdateWeight(float DeltaTime);
	/** Simulate is same as Advance, but without calling any events or touching any of the instance data. So it performs a simulation of advancing the timeline. 
	 * @fixme laurent can we make Advance use that, so we don't have 2 code paths which risk getting out of sync? */
	bool SimulateAdvance(float DeltaTime, float & InOutPosition, struct FRootMotionMovementParams & OutRootMotionParams) const;
	void Advance(float DeltaTime, struct FRootMotionMovementParams & OutRootMotionParams);

	FName GetCurrentSection() const;
	// reference has to be managed manually
	void AddReferencedObjects( FReferenceCollector& Collector );
private:
	/** Delegate function handlers
	 */
	void HandleEvents(float PreviousTrackPos, float CurrentTrackPos, const FBranchingPoint * BranchingPoint);
	void TriggerEventHandler(FName EventName);
	void RefreshNextPrevSections();
};

UCLASS(config=Engine, hidecategories=(UObject, Length), MinimalAPI, BlueprintType)
class UAnimMontage : public UAnimCompositeBase
{
	GENERATED_UCLASS_BODY()

	/** Default blend in time. */
	UPROPERTY(EditAnywhere, Category=Montage)
	float BlendInTime;

	/** Default blend out time. */
	UPROPERTY(EditAnywhere, Category=Montage)
	float BlendOutTime;

	// composite section. 
	UPROPERTY()
	TArray<FCompositeSection> CompositeSections;
	
	// slot data, each slot contains anim track
	UPROPERTY()
	TArray<struct FSlotAnimationTrack> SlotAnimTracks;

	// slot data, each slot contains anim track
	UPROPERTY()
	TArray<struct FBranchingPoint> BranchingPoints;

	/** If this is on, it will allow extracting root motion translation **/
	UPROPERTY(EditAnywhere, Category=RootMotion)
	bool bEnableRootMotionTranslation;

	/** If this is on, it will allow extracting root motion rotation **/
	UPROPERTY(EditAnywhere, Category=RootMotion)
	bool bEnableRootMotionRotation;

	/** Root Bone will be locked to that position when extracting root motion. **/
	UPROPERTY(EditAnywhere, Category=RootMotion)
	TEnumAsByte<ERootMotionRootLock::Type> RootMotionRootLock;

#if WITH_EDITORONLY_DATA
	/** Preview Base pose for additive BlendSpace **/
	UPROPERTY(EditAnywhere, Category=AdditiveSettings)
	UAnimSequence* PreviewBasePose;
#endif // WITH_EDITORONLY_DATA

	/** return true if valid slot */
	bool IsValidSlot(FName InSlotName) const;

public:
	// Begin UObject Interface
	virtual void PostLoad() OVERRIDE;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	// End UObject Interface

	// Begin AnimSequenceBase Interface
	virtual bool IsValidAdditive() const OVERRIDE;
#if WITH_EDITOR
	virtual EAnimEventTriggerOffsets::Type CalculateOffsetForNotify(float NotifyDisplayTime) const OVERRIDE;
#endif // WITH_EDITOR
	// End AnimSequenceBase Interface

#if WITH_EDITOR
	// Begin UAnimationAsset interface
	virtual bool GetAllAnimationSequencesReferred(TArray<UAnimSequence*>& AnimationSequences) OVERRIDE;
	virtual void ReplaceReferredAnimations(const TMap<UAnimSequence*, UAnimSequence*>& ReplacementMap) OVERRIDE;
	// End UAnimationAsset interface

	/** Calculates what (if any) offset should be applied to the trigger time of a branch point given its display time */
	ENGINE_API EAnimEventTriggerOffsets::Type CalculateOffsetForBranchingPoint(float BranchingPointDisplayTime) const;
#endif

	/** Find First BranchingPoint between ]StartTrackPos,EndTrackPos] 
	 * Has to be contiguous range, does not handle looping and wrapping over. **/
	const FBranchingPoint * FindFirstBranchingPoint(float StartTrackPos, float EndTrackPos);

	/** Get FCompositeSection with InSectionName */
	FCompositeSection& GetAnimCompositeSection(int32 SectionIndex);
	const FCompositeSection& GetAnimCompositeSection(int32 SectionIndex) const;

	// @todo document
	ENGINE_API void GetSectionStartAndEndTime(int32 SectionIndex, float& OutStartTime, float& OutEndTime) const;
	
	// @todo document
	ENGINE_API float GetSectionLength(int32 SectionIndex) const;
	
	/** Get SectionIndex from SectionName */
	ENGINE_API int32 GetSectionIndex(FName InSectionName) const;
	
	/** Get SectionName from SectionIndex in TArray */
	ENGINE_API FName GetSectionName(int32 SectionIndex) const;

	/** @return true if valid section */
	ENGINE_API bool IsValidSectionName(FName InSectionName) const;

	// @todo document
	ENGINE_API bool IsValidSectionIndex(int32 SectionIndex) const;

	/** Return Section Index from Position */
	ENGINE_API int32 GetSectionIndexFromPosition(float Position) const;
	
	/** Get Section Index from CurrentTime with PosWithinCompositeSection */
	int32 GetAnimCompositeSectionIndexFromPos(float CurrentTime, float & PosWithinCompositeSection) const;

	/** Return time left to end of section from given position. -1.f if not a valid position */
	ENGINE_API float GetSectionTimeLeftFromPos(float Position);

	/** Utility function to calculate Animation Pos from Section, PosWithinCompositeSection */
	float CalculatePos(FCompositeSection &Section, float PosWithinCompositeSection) const;
	
	/** Prototype function to get animation data - this will need rework */
	const FAnimTrack* GetAnimationData(FName SlotName) const;

	/** Extract RootMotion Transform from a contiguous Track position range.
	 * *CONTIGUOUS* means that if playing forward StartTractPosition < EndTrackPosition.
	 * No wrapping over if looping. No jumping across different sections.
	 * So the AnimMontage has to break the update into contiguous pieces to handle those cases.
	 *
	 * This does handle Montage playing backwards (StartTrackPosition > EndTrackPosition).
	 *
	 * It will break down the range into steps if needed to handle looping animations, or different animations.
	 * These steps will be processed sequentially, and output the RootMotion transform in component space.
	 */
	FTransform ExtractRootMotionFromTrackRange(float StartTrackPosition, float EndTrackPosition) const;

private:
	/** 
	 * Utility function to check if CurrentTime is between FirstIndex and SecondIndex of CompositeSections
	 * return true if it is
	 */
	bool IsWithinPos(int32 FirstIndex, int32 SecondIndex, float CurrentTime) const;

	/** Calculates a trigger offset based on the supplied time taking into account only the montages sections */
	EAnimEventTriggerOffsets::Type CalculateOffsetFromSections(float Time) const;

#if WITH_EDITOR
public:
	// UAnimSequenceBase Interface
	virtual void EvaluateCurveData(class UAnimInstance* Instance, float CurrentTime, float BlendWeight) const OVERRIDE;
	// End of UAnimSequenceBase Interface

	/**
	 * Add Composite section with InSectionName
	 * returns index of added item 
	 * returns INDEX_NONE if failed. - i.e. InSectionName is not unique
	 */
	ENGINE_API int32 AddAnimCompositeSection(FName InSectionName, float StartPos);
	
	/** 
	 * Delete Composite section with InSectionName
	 * return true if success, false otherwise
	 */
	ENGINE_API bool DeleteAnimCompositeSection(int32 SectionIndex);

private:
	/** Sort CompositeSections in the order of StartPos */
	void SortAnimCompositeSectionByPos();

#endif	//WITH_EDITOR

	/** Sort CompositeSections in the order of StartPos */
	void SortAnimBranchingPointByTime();

	// @remove me: temporary variable to do sort while property window changed
	// this should be fixed when we have tool to do so.
	bool bAnimBranchingPointNeedsSort;
};
