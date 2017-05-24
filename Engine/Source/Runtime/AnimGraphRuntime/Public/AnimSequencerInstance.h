// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/**
 *
 * This Instance only contains one AnimationAsset, and produce poses
 * Used by Preview in AnimGraph, Playing single animation in Kismet2 and etc
 */

#pragma once
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "AnimSequencerInstance.generated.h"

UCLASS(transient, NotBlueprintable)
class ANIMGRAPHRUNTIME_API UAnimSequencerInstance : public UAnimInstance
{
	GENERATED_UCLASS_BODY()

	/** 
	 * Called to bind a typed UAnimSequencerInstance to an existing skeletal mesh component 
	 * @return the current (or newly created) UAnimSequencerInstance
	 */
	template<typename InstanceClassType = UAnimSequencerInstance>
	static InstanceClassType* BindToSkeletalMeshComponent(USkeletalMeshComponent* InSkeletalMeshComponent)
	{
		// make sure to tick and refresh all the time when ticks
		// @TODO: this needs restoring post-binding
		InSkeletalMeshComponent->MeshComponentUpdateFlag = EMeshComponentUpdateFlag::AlwaysTickPoseAndRefreshBones;
#if WITH_EDITOR
		InSkeletalMeshComponent->SetUpdateAnimationInEditor(true);
#endif

		// we use sequence instance if it's using anim blueprint that matches. Otherwise, we create sequence player
		// this might need more check - i.e. making sure if it's same skeleton and so on, 
		// Ideally we could just call NeedToSpawnAnimScriptInstance call, which is protected now
		const bool bShouldUseSequenceInstance = ShouldUseSequenceInstancePlayer(InSkeletalMeshComponent);

		// if it should use sequence instance
		if (bShouldUseSequenceInstance)
		{
			// this has to wrap around with this because we don't want to reinitialize everytime they come here
			// SetAnimationMode will reinitiaize it even if it's same, so make sure we just call SetAnimationMode if not AnimationCustomMode
			if (InSkeletalMeshComponent->GetAnimationMode() != EAnimationMode::AnimationCustomMode)
			{
				InSkeletalMeshComponent->SetAnimationMode(EAnimationMode::AnimationCustomMode);
			}

			if (Cast<UAnimSequencerInstance>(InSkeletalMeshComponent->AnimScriptInstance) == nullptr || !InSkeletalMeshComponent->AnimScriptInstance->GetClass()->IsChildOf(InstanceClassType::StaticClass()))
			{
				InstanceClassType* SequencerInstance = NewObject<InstanceClassType>(InSkeletalMeshComponent, InstanceClassType::StaticClass());
				InSkeletalMeshComponent->AnimScriptInstance = SequencerInstance;
				InSkeletalMeshComponent->AnimScriptInstance->InitializeAnimation();
				return SequencerInstance;
			}
			else
			{
				return Cast<InstanceClassType>(InSkeletalMeshComponent->AnimScriptInstance);
			}
		}

		return nullptr;
	}

	/** Called to unbind a UAnimSequencerInstance to an existing skeletal mesh component */
	static void UnbindFromSkeletalMeshComponent(USkeletalMeshComponent* InSkeletalMeshComponent);

public:
	/** Update an animation sequence player in this instance */
	void UpdateAnimTrack(UAnimSequenceBase* InAnimSequence, int32 SequenceId, float InPosition, float Weight, bool bFireNotifies);

	/** Reset all nodes in this instance */
	void ResetNodes();

protected:
	// UAnimInstance interface
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

	/** Helper function for BindToSkeletalMeshComponent */
	static bool ShouldUseSequenceInstancePlayer(const USkeletalMeshComponent* SkeletalMeshComponent);
};
