// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "IPluginManager.h"
#include "ModuleManager.h"

class FPluginBrowserModule : public IPluginsEditor
{
public:
	TMap<IPlugin*, bool> PendingEnablePlugins;

	/** Accessor for the module interface */
	static FPluginBrowserModule& Get()
	{
		return FModuleManager::Get().GetModuleChecked<FPluginBrowserModule>(TEXT("PluginBrowser"));
	}

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** ID name for the plugins editor major tab */
	static const FName PluginsEditorTabName;
};
