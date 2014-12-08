// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "LogVisualizerPCH.h"
#include "Engine/GameInstance.h"
#include "Debug/DebugDrawService.h"
#include "GameFramework/HUD.h"
#if WITH_EDITOR
#include "Editor/EditorEngine.h"
#endif // WITH_EDITOR
#include "LogVisualizerDebugActor.h"

ALogVisualizerDebugActor::ALogVisualizerDebugActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}
