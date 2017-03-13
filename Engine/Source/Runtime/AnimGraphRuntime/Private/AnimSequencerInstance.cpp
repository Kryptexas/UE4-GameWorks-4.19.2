// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UAnimSequencerInstance.cpp: Single Node Tree Instance 
	Only plays one animation at a time. 
=============================================================================*/ 

#include "AnimSequencerInstance.h"
#include "AnimSequencerInstanceProxy.h"
#include "Components/SkeletalMeshComponent.h"

/////////////////////////////////////////////////////
// UAnimSequencerInstance
/////////////////////////////////////////////////////

UAnimSequencerInstance::UAnimSequencerInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bUseMultiThreadedAnimationUpdate = false;
}

FAnimInstanceProxy* UAnimSequencerInstance::CreateAnimInstanceProxy()
{
	return new FAnimSequencerInstanceProxy(this);
}

bool UAnimSequencerInstance::ShouldUseSequenceInstancePlayer(const USkeletalMeshComponent* SkeletalMeshComponent)
{
	const USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->SkeletalMesh;
	// create proper anim instance to animate
	UAnimInstance* AnimInstance = SkeletalMeshComponent->GetAnimInstance();

	return (AnimInstance == nullptr || SkeletalMeshComponent->GetAnimationMode() != EAnimationMode::AnimationBlueprint ||
		AnimInstance->GetClass() != SkeletalMeshComponent->AnimClass || !SkeletalMesh->Skeleton->IsCompatible(AnimInstance->CurrentSkeleton));
}

void UAnimSequencerInstance::UnbindFromSkeletalMeshComponent(USkeletalMeshComponent* InSkeletalMeshComponent)
{
#if WITH_EDITOR
	InSkeletalMeshComponent->SetUpdateAnimationInEditor(false);
#endif

	if (InSkeletalMeshComponent->GetAnimationMode() == EAnimationMode::Type::AnimationCustomMode)
	{
		UAnimSequencerInstance* SequencerInstance = Cast<UAnimSequencerInstance>(InSkeletalMeshComponent->GetAnimInstance());
		if (SequencerInstance)
		{
			InSkeletalMeshComponent->AnimScriptInstance = nullptr;
		}
	}
	else if (InSkeletalMeshComponent->GetAnimationMode() == EAnimationMode::Type::AnimationBlueprint)
	{
		UAnimInstance* AnimInstance = InSkeletalMeshComponent->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->Montage_Stop(0.0f);
			AnimInstance->UpdateAnimation(0.0f, false);
		}

		// Update space bases to reset it back to ref pose
		InSkeletalMeshComponent->RefreshBoneTransforms();
		InSkeletalMeshComponent->RefreshSlaveComponents();
		InSkeletalMeshComponent->UpdateComponentToWorld();
	}
}

void UAnimSequencerInstance::UpdateAnimTrack(UAnimSequenceBase* InAnimSequence, int32 SequenceId, float InPosition, float Weight, bool bFireNotifies)
{
	GetProxyOnGameThread<FAnimSequencerInstanceProxy>().UpdateAnimTrack(InAnimSequence, SequenceId, InPosition, Weight, bFireNotifies);
}

void UAnimSequencerInstance::ResetNodes()
{
	GetProxyOnGameThread<FAnimSequencerInstanceProxy>().ResetNodes();
}