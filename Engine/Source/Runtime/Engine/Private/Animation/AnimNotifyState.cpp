// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "EngineAnimClasses.h"

/////////////////////////////////////////////////////
// UAnimNotifyState

UAnimNotifyState::UAnimNotifyState(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(200, 200, 255, 255);
#endif // WITH_EDITORONLY_DATA
}


void UAnimNotifyState::NotifyBegin(class USkeletalMeshComponent * MeshComp, class UAnimSequence * AnimSeq)
{
	Received_NotifyBegin(MeshComp, AnimSeq);
}

void UAnimNotifyState::NotifyTick(class USkeletalMeshComponent * MeshComp, class UAnimSequence * AnimSeq, float FrameDeltaTime)
{
	Received_NotifyTick(MeshComp, AnimSeq, FrameDeltaTime);
}

void UAnimNotifyState::NotifyEnd(class USkeletalMeshComponent * MeshComp, class UAnimSequence * AnimSeq)
{
	Received_NotifyEnd(MeshComp, AnimSeq);
}

