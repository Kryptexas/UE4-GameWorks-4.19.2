// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"

/////////////////////////////////////////////////////
// UAnimNotifyState

UAnimNotifyState::UAnimNotifyState(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(200, 200, 255, 255);
#endif // WITH_EDITORONLY_DATA
}


void UAnimNotifyState::NotifyBegin(class USkeletalMeshComponent * MeshComp, class UAnimSequenceBase * Animation)
{
	Received_NotifyBegin(MeshComp, Animation);
}

void UAnimNotifyState::NotifyTick(class USkeletalMeshComponent * MeshComp, class UAnimSequenceBase * Animation, float FrameDeltaTime)
{
	Received_NotifyTick(MeshComp, Animation, FrameDeltaTime);
}

void UAnimNotifyState::NotifyEnd(class USkeletalMeshComponent * MeshComp, class UAnimSequenceBase * Animation)
{
	Received_NotifyEnd(MeshComp, Animation);
}

