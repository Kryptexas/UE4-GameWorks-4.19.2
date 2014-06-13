// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ScriptEditorPluginPrivatePCH.h"

DEFINE_LOG_CATEGORY(LogScriptEditorPlugin);

class FScriptEditorPlugin : public IScriptEditorPlugin
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE(FScriptEditorPlugin, ScriptEditorPlugin)


void FScriptEditorPlugin::StartupModule()
{
}

void FScriptEditorPlugin::ShutdownModule()
{
}
