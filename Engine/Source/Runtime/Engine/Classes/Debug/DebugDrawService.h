// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "DebugDrawService.generated.h"

/** 
 * 
 */
DECLARE_DELEGATE_TwoParams(FDebugDrawDelegate, class UCanvas*, class APlayerController*);

UCLASS(config=Engine)
class ENGINE_API UDebugDrawService : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	//void Register

	static void Register(const TCHAR* Name, const FDebugDrawDelegate& NewDelegate);
	static void Unregister(const FDebugDrawDelegate& NewDelegate);
	static void Draw(const FEngineShowFlags Flags, class UCanvas* Canvas);
	static void Draw(const FEngineShowFlags Flags, class FViewport* Viewport, FSceneView* View, FCanvas* Canvas);

private:
	static TArray<TArray<FDebugDrawDelegate> > Delegates;
	static FEngineShowFlags ObservedFlags;
};
