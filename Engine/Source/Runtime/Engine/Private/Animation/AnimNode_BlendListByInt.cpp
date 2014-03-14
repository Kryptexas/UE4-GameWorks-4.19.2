// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "AnimationRuntime.h"

/////////////////////////////////////////////////////
// FAnimNode_BlendListByInt

int32 FAnimNode_BlendListByInt::GetActiveChildIndex()
{
	const int NumPoses = BlendPose.Num();
	return FMath::Clamp<int32>(ActiveChildIndex, 0, NumPoses - 1);
}
