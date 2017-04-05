// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h" // for DLLEXPORT (KISMET_API)

class UBlueprint;

struct KISMET_API FBlueprintCompilationManager
{
	static void Initialize();
	static void Shutdown();

	/**
	 * Compiles all blueprints that have been placed in the compilation queue. 
	 * ObjLoaded is a list of objects that need to be PostLoaded by the linker,
	 * when changing CDOs we will replace objects in this list. It is not a list
	 * of objects the compilation manager has loaded. The compilation manager
	 * will not load data while processing the compilation queue)
	 */
	static void FlushCompilationQueue(TArray<UObject*>* ObjLoaded = nullptr);

	/**
	 * Adds the blueprint to the compilation queue
	 */
	static void QueueForCompilation(UBlueprint* BPToCompile);
	
	/**
	 * Adds a newly loaded blueprint to the compilation queue
	 */
	static void NotifyBlueprintLoaded(UBlueprint* BPLoaded);

	/** Returns true when UBlueprint::GeneratedClass members are up to date */
	static bool IsGeneratedClassLayoutReady();
private:
	FBlueprintCompilationManager();
};

