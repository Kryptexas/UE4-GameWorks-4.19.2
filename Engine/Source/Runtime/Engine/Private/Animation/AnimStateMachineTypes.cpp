// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "AnimationRuntime.h"
#include "AnimationUtils.h"
#include "Animation/AnimStateMachineTypes.h"

/////////////////////////////////////////////////////
// UAnimStateMachineTypes

UAnimStateMachineTypes::UAnimStateMachineTypes(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

int32 FBakedAnimationStateMachine::FindStateIndex(FName StateName) const
{
	for (int32 Index = 0; Index < States.Num(); Index++)
	{
		if (States[Index].StateName == StateName)
		{
			return Index;
		}
	}

	return INDEX_NONE;
}