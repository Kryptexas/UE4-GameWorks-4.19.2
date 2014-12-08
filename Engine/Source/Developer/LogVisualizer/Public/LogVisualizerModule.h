// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ILogVisualizer.h"

class FLogVisualizerModule : public IModuleInterface
{
public:
	// Begin IModuleInterface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
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
