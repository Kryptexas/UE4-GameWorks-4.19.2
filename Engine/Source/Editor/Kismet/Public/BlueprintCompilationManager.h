// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h" // for DLLEXPORT (KISMET_API)

class UBlueprint;

struct KISMET_API FBlueprintCompilationManager
{
	static void Initialize();
	static void Shutdown();

	static void FlushCompilationQueue();
	static void QueueForCompilation(UBlueprint* BPToCompile);
	static void NotifyBlueprintLoaded(UBlueprint* BPLoaded);

private:
	FBlueprintCompilationManager();
};

