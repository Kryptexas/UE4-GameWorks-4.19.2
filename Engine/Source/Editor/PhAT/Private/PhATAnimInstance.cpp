// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "PhATAnimInstance.h"
#include "PhATAnimInstanceProxy.h"

/////////////////////////////////////////////////////
// UPhATAnimInstance
/////////////////////////////////////////////////////

UPhATAnimInstance::UPhATAnimInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bUseMultiThreadedAnimationUpdate = true;
}

FAnimInstanceProxy* UPhATAnimInstance::CreateAnimInstanceProxy()
{
	return new FPhATAnimInstanceProxy(this);
}

bool UPhATAnimInstance::IsPlayingAnimation() const
{
	const FPhATAnimInstanceProxy& AnimInstProxy = GetProxyOnGameThread<FPhATAnimInstanceProxy>();
	return AnimInstProxy.IsPlayingAnimation();
}

void UPhATAnimInstance::SetAnimationSequence(UAnimSequence* AnimSequence)
{
	FPhATAnimInstanceProxy& AnimInstProxy = GetProxyOnGameThread<FPhATAnimInstanceProxy>();
	return AnimInstProxy.SetAnimationSequence(AnimSequence);
}

void UPhATAnimInstance::SetPlayRate(float PlayRate)
{
	FPhATAnimInstanceProxy& AnimInstProxy = GetProxyOnGameThread<FPhATAnimInstanceProxy>();
	return AnimInstProxy.SetPlayRate(PlayRate);
}