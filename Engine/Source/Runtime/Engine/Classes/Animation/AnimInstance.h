// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimMontage.h"
#include "AnimSequence.h"
#include "AnimationAsset.h"
#include "AnimInstance.generated.h"

struct FBoneContainer;
class  USkeletalMesh;

/** Enum for controlling which reference frame a controller is applied in. */
UENUM()
enum EBoneControlSpace
{
	/** Set absolute position of bone in world space. */
	BCS_WorldSpace UMETA( DisplayName = "World Space" ),
	/** Set position of bone in SkeletalMeshComponent's reference frame. */
	BCS_ComponentSpace UMETA( DisplayName = "Component Space" ),
	/** Set position of bone relative to parent bone. */
	BCS_ParentBoneSpace UMETA( DisplayName = "Parent Bone Space" ),
	/** Set position of bone in its own reference frame. */
	BCS_BoneSpace UMETA( DisplayName = "Bone Space" ),
	BCS_MAX,
};

USTRUCT()
struct FA2Pose
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FTransform> Bones;

	FA2Pose() {}
};

/** component space poses **/
USTRUCT()
struct ENGINE_API FA2CSPose : public FA2Pose
{
	GENERATED_USTRUCT_BODY()

private:
	/** Pointer to current BoneContainer. */
	const struct FBoneContainer * BoneContainer;

	/** once evaluated to be mesh space, this flag will be set **/
	UPROPERTY()
	TArray<uint8> ComponentSpaceFlags;

public:
	FA2CSPose()
		: BoneContainer(NULL)
	{
	}

	/** constructor - needs LocalPoses **/
	void AllocateLocalPoses(const FBoneContainer & InBoneContainer, const FA2Pose & LocalPose);

	/** constructor - needs LocalPoses **/
	void AllocateLocalPoses(const FBoneContainer & InBoneContainer, const FTransformArrayA2 & LocalBones);

	/** Returns if this struct is valid */
	bool IsValid() const;

	/** Get parent bone index for given bone index. */
	int32 GetParentBoneIndex(const int32 & BoneIndex) const;

	/** Returns local transform for the boneindex **/
	FTransform GetLocalSpaceTransform(int32 BoneIndex);

	/**
	 * Do not access Bones array directly but via this 
	 * This will fill up gradually mesh space bases 
	 */
	FTransform GetComponentSpaceTransform(int32 BoneIndex);

	/** convert to local poses **/
	void ConvertToLocalPoses(FA2Pose & LocalPoses) const;

	/** 
	 * Set a bunch of Component Space Bone Transforms.
	 * Do this safely by insuring that Parents are already in Component Space,
	 * and any Component Space children are converted back to Local Space before hand.
	 */
	void SafeSetCSBoneTransforms(const TArray<struct FBoneTransform> & BoneTransforms);

	/** 
	 * Blends Component Space transforms to MeshPose in Local Space. 
	 * Used by SkelControls to apply their transforms.
	 *
	 * The tricky bit is that SkelControls deliver their transforms in Component Space,
	 * But the blending is done in Local Space. Also we need to refresh any Children they have
	 * that has been previously converted to Component Space.
	 */
	void LocalBlendCSBoneTransforms(const TArray<struct FBoneTransform> & BoneTransforms, float Alpha);

private:
	/** Calculate all transform till parent **/
	void CalculateComponentSpaceTransform(int32 Index);
	void SetComponentSpaceTransform(int32 Index, const FTransform & NewTransform);

	/**
	 * Convert Bone to Local Space.
	 */
	void ConvertBoneToLocalSpace(int32 BoneIndex);


	void SetLocalSpaceTransform(int32 Index, const FTransform& NewTransform);

	/** this is not really best way to protect SetComponentSpaceTransform, but we'd like to make sure
	 that isn't called by anywhere else */
	friend class FAnimationRuntime;
};

USTRUCT(BlueprintType)
struct FBoneTransform
{
	GENERATED_USTRUCT_BODY()

	/** @todo anim: should be Skeleton bone index in the future, but right now it's Mesh BoneIndex **/
	UPROPERTY()
	int32 BoneIndex;

	/** Transform to apply **/
	UPROPERTY()
	FTransform Transform;

	FBoneTransform() 
		: BoneIndex(INDEX_NONE)
	{}

	FBoneTransform( int32 InBoneIndex, const FTransform & InTransform) 
		: BoneIndex(InBoneIndex)
		, Transform(InTransform)
	{}
};

USTRUCT(BlueprintType)
struct FPerBoneBlendWeight
{
	GENERATED_USTRUCT_BODY()
	UPROPERTY()
	int32 SourceIndex; // source index of the buffer
	UPROPERTY()
	float BlendWeight; // how much blend weight

	FPerBoneBlendWeight()
		: SourceIndex(0)
		, BlendWeight(0.0f)
	{
	}
};

USTRUCT(BlueprintType)
struct FPerBoneBlendWeights
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FPerBoneBlendWeight> BoneBlendWeights;


	FPerBoneBlendWeights() {}

};

UCLASS(transient, Blueprintable, hideCategories=AnimInstance, DependsOn(UAnimStateMachineTypes), BlueprintType)
class ENGINE_API UAnimInstance : public UObject
{
	GENERATED_UCLASS_BODY()

	/** DeltaTime **/
	UPROPERTY()
	float DeltaTime_DEPRECATED;

	/** This is used to extract animation. If Mesh exists, this will be overwritten by Mesh->Skeleton */
	UPROPERTY(transient)
	class USkeleton* CurrentSkeleton;

	// The list of animation assets which are going to be evaluated this frame and need to be ticked (ungrouped)
	UPROPERTY(transient)
	TArray<FAnimTickRecord> UngroupedActivePlayers;

	// The set of tick groups for this anim instance
	UPROPERTY(transient)
	TArray<FAnimGroupInstance> SyncGroups;

	/** Array indicating active vertex anims (by reference) generated by anim instance. */
	UPROPERTY(transient)
	TArray<struct FActiveVertexAnim> VertexAnims;

public:

	// @todo document
	void MakeSequenceTickRecord(FAnimTickRecord& TickRecord, class UAnimSequenceBase* Sequence, bool bLooping, float PlayRate, float FinalBlendWeight, float& CurrentTime) const;
	void MakeBlendSpaceTickRecord(FAnimTickRecord& TickRecord, class UBlendSpaceBase* BlendSpace, const FVector& BlendInput, TArray<FBlendSampleData>& BlendSampleDataCache, FBlendFilter& BlendFilter, bool bLooping, float PlayRate, float FinalBlendWeight, float& CurrentTime) const;

	void SequenceAdvanceImmediate(class UAnimSequenceBase* Sequence, bool bLooping, float PlayRate, float DeltaSeconds, /*inout*/ float& CurrentTime);

	// @todo document
	void BlendSpaceAdvanceImmediate(class UBlendSpaceBase* BlendSpace, const FVector& BlendInput, TArray<FBlendSampleData> & BlendSampleDataCache, FBlendFilter & BlendFilter, bool bLooping, float PlayRate, float DeltaSeconds, /*inout*/ float& CurrentTime);

	// Creates an uninitialized tick record in the list for the correct group or the ungrouped array.  If the group is valid, OutSyncGroupPtr will point to the group.
	FAnimTickRecord& CreateUninitializedTickRecord(int32 GroupIndex, FAnimGroupInstance*& OutSyncGroupPtr);

	void SequenceEvaluatePose(class UAnimSequenceBase* Sequence, struct FA2Pose& Pose, const FAnimExtractContext & ExtractionContext);

	void BlendSequences(const struct FA2Pose& Pose1, const struct FA2Pose& Pose2, float Alpha, struct FA2Pose& Blended);

	static void CopyPose(const struct FA2Pose& Source, struct FA2Pose& Destination);

	void ApplyAdditiveSequence(const struct FA2Pose& BasePose, const struct FA2Pose& AdditivePose, float Alpha, struct FA2Pose& Blended);

	void BlendSpaceEvaluatePose(class UBlendSpaceBase* BlendSpace, TArray<FBlendSampleData>& BlendSampleDataCache, struct FA2Pose& Pose, bool bIsLooping);

	// skeletal control related functions
	void BlendRotationOffset(const struct FA2Pose& BasePose/* local space base pose */, struct FA2Pose& RotationOffsetPose/* mesh space rotation only additive **/, float Alpha/*0 means no additive, 1 means whole additive */, struct FA2Pose& Pose /** local space blended pose **/);

	// slotnode interfaces
	bool NeedToTickChildren(FName SlotNodeName, float SlotNodeWeight);
	float GetSlotWeight(FName SlotNodeName);
	void SlotEvaluatePose(FName SlotNodeName, const struct FA2Pose & SourcePose, struct FA2Pose & BlendedPose, float SlotNodeWeight);

	// slot node run-time functions
	void RegisterSlotNode(FName SlotNodeName);
	void UpdateSlotNodeWeight(FName SlotNodeName, float Weight);
	// if it doesn't tick, it will keep old weight, so we'll have to clear it in the beginning of tick
	void ClearSlotNodeWeights();
	bool IsActiveSlotNode(FName SlotNodeName) const;

protected:
	// kismet event functions

	/** Returns the owning actor of this AnimInstance */
	UFUNCTION(BlueprintPure, Category="Animation")
	class AActor* GetOwningActor();

	UFUNCTION(BlueprintPure, Category="Animation")
	virtual class APawn* TryGetPawnOwner();

	// Returns the skeletal mesh component that has created this AnimInstance
	UFUNCTION(BlueprintPure, Category="Animation")
	class USkeletalMeshComponent* GetOwningComponent();

public:

	/** Executed when the Animation is initialized */
	UFUNCTION(BlueprintImplementableEvent)
	virtual void BlueprintInitializeAnimation();

	/** Executed when the Animation is updated */
	UFUNCTION(BlueprintImplementableEvent)
	virtual void BlueprintUpdateAnimation(float DeltaTimeX);

	bool CanTransitionSignature() const;
	
	UFUNCTION()
	void AnimNotify_Sound(UAnimNotify* Notify);

public:

	/** Play normal animation asset on the slot node. You can only play one asset (whether montage or animsequence) at a time. */
	UFUNCTION(BlueprintCallable, Category="Animation", meta=(BlendInTime="0.25", BlendOutTime="0.25", InPlayRate="1.0"))
	float PlaySlotAnimation(UAnimSequenceBase* Asset, FName SlotNodeName, float BlendInTime, float BlendOutTime, float InPlayRate);

	/** Stops currently playing slot animation */
	UFUNCTION(BlueprintCallable, Category="Animation", meta=(InBlendOutTime="0.25"))
	void StopSlotAnimation(float InBlendOutTime);

	/** Return true if it's playing the slot animation */
	UFUNCTION(BlueprintCallable, Category="Animation")
	bool IsPlayingSlotAnimation(UAnimSequenceBase* Asset, FName SlotNodeName ); 

	/** Makes a montage jump to a named section. */
	UFUNCTION(BlueprintCallable, Category="Animation")
	void Montage_JumpToSection(FName SectionName);

	/** Makes a montage jump to the end of a named section. */
	UFUNCTION(BlueprintCallable, Category="Animation")
	void Montage_JumpToSectionsEnd(FName SectionName);

	/** Changes the next section in the montage to a different one. */
	UFUNCTION(BlueprintCallable, Category="Animation")
	void Montage_SetNextSection(FName SectionNameToChange, FName NextSection);

	/** Plays an animation montage. Returns the length of the animation montage in seconds. Returns 0.f if failed to play. */
	UFUNCTION(BlueprintCallable, Category="Animation", meta=(InPlayRate="1.0"))
	float Montage_Play(UAnimMontage* MontageToPlay, float InPlayRate);

	/** Stops the animation montage. */
	UFUNCTION(BlueprintCallable, Category="Animation", meta=(InBlendOutTime="0.25"))
	void Montage_Stop(float InBlendOutTime);

	/** Returns true if the animation montage is active. */
	UFUNCTION(BlueprintCallable, Category="Animation")
	bool Montage_IsActive(UAnimMontage* Montage);  

	/** Returns true if the animation montage is currently playing. */
	UFUNCTION(BlueprintCallable, Category="Animation")
	bool Montage_IsPlaying(UAnimMontage* Montage); 

	/** Returns the name of the current animation montage section. */
	UFUNCTION(BlueprintCallable, Category="Animation")
	FName Montage_GetCurrentSection();

	/** Returns the value of a named curve. */
	UFUNCTION(BlueprintPure, Category="Animation", meta=(BlueprintProtected = "true"))
	float GetCurveValue(FName CurveName);

	/** Returns the length (in seconds) of an animation AnimAsset. */
	UFUNCTION(BlueprintPure, Category="Animation", meta=(BlueprintInternalUseOnly = "true"))
	static float GetAnimAssetPlayerLength(class UAnimationAsset* AnimAsset);

	//** Returns how far through the animation AnimAsset we are (as a proportion between 0.0 and 1.0). */
	UFUNCTION(BlueprintPure, Category="Animation", meta=(BlueprintInternalUseOnly = "true"))
	static float GetAnimAssetPlayerTimeFraction(class UAnimationAsset* AnimAsset, float CurrentTime);

	/** Returns how long until the end of the animation AnimAsset (in seconds). */
	UFUNCTION(BlueprintPure, Category="Animation", meta=(BlueprintInternalUseOnly = "true"))
	static float GetAnimAssetPlayerTimeFromEnd(class UAnimationAsset* AnimAsset, float CurrentTime);

	/** Returns how long until the end of the animation AnimAsset we are (as a proportion between 0.0 and 1.0). */
	UFUNCTION(BlueprintPure, Category="Animation", meta=(BlueprintInternalUseOnly = "true"))
	static float GetAnimAssetPlayerTimeFromEndFraction(class UAnimationAsset* AnimAsset, float CurrentTime);

	/** Returns the weight of a state in a state machine. */
	UFUNCTION(BlueprintPure, Category="Animation", meta=(BlueprintInternalUseOnly = "true"))
	float GetStateWeight(int32 MachineIndex, int32 StateIndex);

	/** Returns (in seconds) the time a state machine has been active. */
	UFUNCTION(BlueprintPure, Category="Animation", meta=(BlueprintInternalUseOnly = "true"))
	float GetCurrentStateElapsedTime(int32 MachineIndex);

	/** Sets a morph target to a certain weight. */
	UFUNCTION(BlueprintCallable, Category="Animation")
	void SetMorphTarget(FName MorphTargetName, float Value);

	/** Clears the current morph targets. */
	UFUNCTION(BlueprintCallable, Category="Animation")
	void ClearMorphTargets();

	/** 
	 * Returns degree of the angle betwee velocity and Rotation forward vector
	 * The range of return will be from [-180, 180], and this can be used to feed blendspace directional value
	 */
	UFUNCTION(BlueprintCallable, Category="Animation")
	float CalculateDirection(const FVector & Velocity, const FRotator & BaseRotation);

	//--- AI communication start ---//
	/** locks indicated AI resources of animated pawn */
	UFUNCTION(BlueprintCallable, Category="Animation", BlueprintAuthorityOnly)
	void LockAIResources(bool bLockMovement, bool LockAILogic);

	/** unlocks indicated AI resources of animated pawn. Will unlock only animation-locked resources */
	UFUNCTION(BlueprintCallable, Category="Animation", BlueprintAuthorityOnly)
	void UnlockAIResources(bool bUnlockMovement, bool UnlockAILogic);
	//--- AI communication end ---//

public:
	// Root node of animation graph
	struct FAnimNode_Base* RootNode;

public:
	// Begin UObject Interface
	virtual void Serialize(FArchive& Ar);
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	// End UObject Interface


	//@TODO: Better comments
	virtual void EvaluateAnimation(struct FPoseContext& Output);

	void InitializeAnimation();
	void UpdateAnimation(float DeltaSeconds);

	// Native initialization override point
	virtual void NativeInitializeAnimation();

	// Native update override point
	virtual void NativeUpdateAnimation(float DeltaSeconds);

	// Native evaluate override point.
	// @return true if this function is implemented, false otherwise.
	// Note: the node graph will not be evaluated if this function returns true
	virtual bool NativeEvaluateAnimation(FPoseContext& Output);
public:

	/** Temporary array of bone indices required this frame. Should be subset of Skeleton and Mesh's RequiredBones */
	FBoneContainer RequiredBones;

	/** Animation Notifies that has been triggered in the latest tick **/
	TArray<const struct FAnimNotifyEvent *> AnimNotifies;

	/** Currently Active AnimNotifyState */
	TArray<const struct FAnimNotifyEvent *> ActiveAnimNotifyState;

	/** AnimMontage instances that are running currently 
	 * - only one is primarily active, but the other ones are blending out 
	 */
	TArray<struct FAnimMontageInstance*> MontageInstances;

	/** Curve Values that are added to trigger in event**/
	TMap<FName, float>	EventCurves;
	/** Morph Target Curves that will be used for SkeletalMeshComponent **/
	TMap<FName, float>	MorphTargetCurves;
	/** Material Curves that will be used for SkeletalMeshComponent **/
	TMap<FName, float>	MaterialParameterCurves;
	/** Material parameters that we had been changing and now need to clear */
	TArray<FName> MaterialParamatersToClear;

	TMap<FName, float> ActiveSlotWeights;

#if WITH_EDITORONLY_DATA
	// Maximum playback position ever reached (only used when debugging in Persona)
	double LifeTimer;

	// Current scrubbing playback position (only used when debugging in Persona)
	double CurrentLifeTimerScrubPosition;
#endif

public:
	/** 
	 * Recalculate Required Bones [RequiredBones]
	 * Is called when bRequiredBonesUpToDate = false
	 */
	void RecalcRequiredBones();

	/** When RequiredBones mapping has changed, AnimNodes need to update their bones caches. */
	UPROPERTY(Transient)
	bool bBoneCachesInvalidated;

	/** Increment Context Counter, used by SavedCachePose to traverse tree once. */
	void IncrementContextCounter();

	/** Get current Context Counter, used by SavedCachePose to traverse tree once. */
	int16 GetContextCounter() const;

	// @todo document
	inline USkeletalMeshComponent* GetSkelMeshComponent() const { return CastChecked<USkeletalMeshComponent>(GetOuter()); }

	virtual class UWorld* GetWorld() const OVERRIDE;

	/** Add anim notifier **/
	void AddAnimNotifies(const TArray<const FAnimNotifyEvent*>& NewNotifies, const float InstanceWeight);

	/** Queues an Anim Notify from the shared list on our generated class */
	void AddAnimNotifyFromGeneratedClass(int32 NotifyIndex);

	/** Trigger AnimNotifies **/
	void TriggerAnimNotifies(float DeltaSeconds);

	/** Add curve float data **/
	void AddCurveValue(const FName & CurveName, float Value, int32 CurveTypeFlags);

	/* native cpp montage interface functions **/
	void Montage_SetEndDelegate(FOnMontageEnded & OnMontageEnded);
	void Montage_SetBlendingOutDelegate(FOnMontageBlendingOutStarted & OnMontageBlendingOut);
	FOnMontageBlendingOutStarted* Montage_GetBlendingOutDelegate();

public:
	/** Get Current Montage Position */
	float Montage_GetPosition(UAnimMontage* Montage);
	/** Has Montage been stopped? */
	bool Montage_GetIsStopped(UAnimMontage* Montage);

	/** Set position. */
	void Montage_SetPosition(UAnimMontage* Montage, float NewPosition);

	/** Get PlayRate */
	float Montage_GetPlayRate(UAnimMontage* Montage);

	/** Set PlayRate */
	void Montage_SetPlayRate(UAnimMontage* Montage, float NewPlayRate);

	/** Get next sectionID for given section ID */
	int32 Montage_GetNextSectionID(UAnimMontage* Montage, int32 CurrentSectionID);

	/** Get Current Active Montage in this AnimInstance **/
	UAnimMontage * GetCurrentActiveMontage();

protected:
	/** Update weight of montages  **/
	virtual void Montage_UpdateWeight(float DeltaSeconds);
	/** Advance montages **/
	virtual void Montage_Advance(float DeltaSeconds);
	/** Stop all montages that are active **/
	void StopAllMontages(float BlendOut);
	/** Get Currently active montage instace **/
	FAnimMontageInstance* GetActiveMontageInstance();
	/** Called by blueprint functions that modify the montages current position. */
	void OnMontagePositionChanged(FAnimMontageInstance* MontageInstance, FName ToSectionName);

#if WITH_EDITORONLY_DATA
	// Returns true if a snapshot is being played back and the remainder of Update should be skipped.
	bool UpdateSnapshotAndSkipRemainingUpdate();
#endif

	// Root Motion
public:
	/** Get current RootMotion FAnimMontageInstance if any. NULL otherwise. */
	FAnimMontageInstance * GetRootMotionMontageInstance() const;

private:
	/** Active Root Motion Montage Instance, if any. */
	struct FAnimMontageInstance * RootMotionMontageInstance;

	/** Update RootMotionMontageInstance */
	void UpdateRootMotionMontageInstance();
};

