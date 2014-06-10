// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayDebuggingTypes.generated.h"

UENUM()
namespace EAIDebugDrawDataView
{
	enum Type
	{
		Empty,
		OverHead,
		Basic,
		BehaviorTree,
		EQS,
		Perception,
		GameView1,
		GameView2,
		GameView3,
		GameView4,
		GameView5,
		NavMesh,
		EditorDebugAIFlag,
		MAX UMETA(Hidden)
	};
}

struct GAMEPLAYDEBUGGER_API FGameplayDebuggerSettings
{
	static void SetFlag(uint8 Flag)
	{
		DebuggerShowFlags |= (1 << Flag);
	}

	static void ClearFlag(uint8 Flag)
	{
		DebuggerShowFlags &= ~(1 << Flag);
	}

	static bool CheckFlag(uint8 Flag)
	{
		return (DebuggerShowFlags & (1 << Flag)) != 0;
	}

	static uint32 DebuggerShowFlags;
};

