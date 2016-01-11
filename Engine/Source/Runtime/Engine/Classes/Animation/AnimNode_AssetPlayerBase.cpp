// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "AnimNode_AssetPlayerBase.h"

FAnimNode_AssetPlayerBase::FAnimNode_AssetPlayerBase()
	: bIgnoreForRelevancyTest(false)
	, BlendWeight(0.0f)
	, InternalTimeAccumulator(0.0f)
{

}

void FAnimNode_AssetPlayerBase::Update(const FAnimationUpdateContext& Context)
{
	// Cache the current weight and update the node
	BlendWeight = Context.GetFinalBlendWeight();
	UpdateAssetPlayer(Context);
}

float FAnimNode_AssetPlayerBase::GetCachedBlendWeight()
{
	return BlendWeight;
}

float FAnimNode_AssetPlayerBase::GetAccumulatedTime()
{
	return InternalTimeAccumulator;
}

void FAnimNode_AssetPlayerBase::SetAccumulatedTime(const float& NewTime)
{
	InternalTimeAccumulator = NewTime;
}

UAnimationAsset* FAnimNode_AssetPlayerBase::GetAnimAsset()
{
	return nullptr;
}

