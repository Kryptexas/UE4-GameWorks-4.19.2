// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ICollisionAnalyzer.h"

#include "Slate.h"

class FCollisionAnalyzerModule : public IModuleInterface
{
public:
	// Begin IModuleInterface
	virtual void StartupModule() OVERRIDE;
	virtual void ShutdownModule() OVERRIDE;
	// End IModuleInterface

	/** Gets the debugger singleton or returns NULL */
	static ICollisionAnalyzer* Get()
	{
		FCollisionAnalyzerModule& DebuggerModule = FModuleManager::Get().LoadModuleChecked<FCollisionAnalyzerModule>("CollisionAnalyzer");
		return DebuggerModule.GetSingleton();
	}

private:
	virtual ICollisionAnalyzer* GetSingleton() const 
	{ 
		return CollisionAnalyzer; 
	}

	/** Spawns the Collision Analyzer tab in an SDockTab */
	TSharedRef<class SDockTab> SpawnCollisionAnalyzerTab(const FSpawnTabArgs& Args);

	ICollisionAnalyzer* CollisionAnalyzer;
};
