// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

struct FBlueprintCompilationManagerImpl;

struct KISMET_API FBlueprintCompilationManager
{
	static void Initialize();
	static void Shutdown();
	static FBlueprintCompilationManagerImpl* DebugAccessImpl();

	static void QueueForCompilation(UBlueprint* BPToCompile);
	static void NotifyBlueprintLoaded(UBlueprint* BPLoaded);
};

