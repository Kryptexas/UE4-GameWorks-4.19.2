// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LogVisualizerPCH.h"

IMPLEMENT_MODULE(FLogVisualizerModule, LogVisualizer);
DEFINE_LOG_CATEGORY(LogLogVisualizer);

void FLogVisualizerModule::StartupModule() 
{
	LogVisualizer = new FLogVisualizer();
}

void FLogVisualizerModule::ShutdownModule() 
{
	if (LogVisualizer != NULL)
	{
		delete LogVisualizer;
		LogVisualizer = NULL;
	}
}
