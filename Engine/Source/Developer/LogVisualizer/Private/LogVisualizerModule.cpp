// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "LogVisualizerPCH.h"
#include "LogVisualizer.h"

IMPLEMENT_MODULE(FLogVisualizerModule, LogVisualizer);
DEFINE_LOG_CATEGORY(LogLogVisualizer);

void FLogVisualizerModule::StartupModule() 
{
#if ENABLE_VISUAL_LOG
	LogVisualizer = new FLogVisualizer();
#endif
}

void FLogVisualizerModule::ShutdownModule() 
{
	if (LogVisualizer != NULL)
	{
		delete LogVisualizer;
		LogVisualizer = NULL;
	}
}
