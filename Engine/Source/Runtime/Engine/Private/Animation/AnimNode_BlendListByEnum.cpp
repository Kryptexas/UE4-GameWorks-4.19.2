// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "AnimationRuntime.h"

/////////////////////////////////////////////////////
// FAnimNode_BlendListByEnum

int32 FAnimNode_BlendListByEnum::GetActiveChildIndex()
{
	if (EnumToPoseIndex.IsValidIndex(ActiveEnumValue))
	{
		return EnumToPoseIndex[ActiveEnumValue];
	}
	else
	{
		return 0;
	}
}

