// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "LiveLinkInstance.h"

void FLiveLinkInstanceProxy::Initialize(UAnimInstance* InAnimInstance)
{
	FAnimInstanceProxy::Initialize(InAnimInstance);

	// initialize node manually 
	FAnimationInitializeContext InitContext(this);
	PoseNode.Initialize_AnyThread(InitContext);
}

bool FLiveLinkInstanceProxy::Evaluate(FPoseContext& Output)
{
	PoseNode.Evaluate_AnyThread(Output);

	return true;
}

void FLiveLinkInstanceProxy::UpdateAnimationNode(float DeltaSeconds)
{
	UpdateCounter.Increment();

	FAnimationUpdateContext UpdateContext(this, DeltaSeconds);
	PoseNode.Update_AnyThread(UpdateContext);
}

ULiveLinkInstance::ULiveLinkInstance(const FObjectInitializer& Initializer)
	: Super(Initializer)
{

}

FAnimInstanceProxy* ULiveLinkInstance::CreateAnimInstanceProxy()
{
	return new FLiveLinkInstanceProxy(this);
}