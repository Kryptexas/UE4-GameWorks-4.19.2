// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ILogVisualizer.h"

class FLogVisualizerModule : public IModuleInterface
{
public:
	// Begin IModuleInterface
	virtual void StartupModule() OVERRIDE;
	virtual void ShutdownModule() OVERRIDE;
	// End IModuleInterface

	/** Gets the debugger singleton or returns NULL */
	static ILogVisualizer* Get()
	{
		FLogVisualizerModule& VisLogModule = FModuleManager::Get().LoadModuleChecked<FLogVisualizerModule>("LogVisualizer");
		return VisLogModule.GetSingleton();
	}

private:
	virtual ILogVisualizer* GetSingleton() const 
	{ 
		return LogVisualizer; 
	}

	ILogVisualizer* LogVisualizer;
};
