// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "EngineAnimClasses.h"
#include "EngineAnimationNodeClasses.h"

/////////////////////////////////////////////////////
// FAnimNode_BlendSpaceEvaluator

FAnimNode_BlendSpaceEvaluator::FAnimNode_BlendSpaceEvaluator()
	: FAnimNode_BlendSpacePlayer()
	, NormalizedTime(0.f)
{
}

void FAnimNode_BlendSpaceEvaluator::Update(const FAnimationUpdateContext& Context)
{
	InternalTimeAccumulator = NormalizedTime;
	PlayRate = 0.f;

	FAnimNode_BlendSpacePlayer::Update(Context);
}

