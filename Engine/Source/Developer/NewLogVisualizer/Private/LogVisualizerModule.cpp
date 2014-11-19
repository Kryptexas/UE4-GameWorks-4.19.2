// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "LogVisualizer.h"
#include "LogVisualizerModule.h"
#include "LogVisualizerStyle.h"
#include "SDockTab.h"

#define LOCTEXT_NAMESPACE "FLogVisualizerModule"

static const FName VisualLoggerTabName("VisualLogger");

//DEFINE_LOG_CATEGORY(LogLogVisualizer);

void FNewLogVisualizerModule::StartupModule()
{
	FLogVisualizerStyle::Initialize();

	FVisualLoggerCommands::Register();
	IModularFeatures::Get().RegisterModularFeature(VisualLoggerTabName, this);

	// This is still experimental in the editor, so it'll be invoked specifically in FMainMenu if the experimental settings flag is set.
	// When no longer experimental, switch to the nomad spawner registration below
	FGlobalTabmanager::Get()->RegisterTabSpawner(VisualLoggerTabName, FOnSpawnTab::CreateRaw(this, &FNewLogVisualizerModule::SpawnLogVisualizerTab));
}

void FNewLogVisualizerModule::ShutdownModule()
{
	FGlobalTabmanager::Get()->UnregisterTabSpawner(VisualLoggerTabName);
	FVisualLoggerCommands::Unregister();
	IModularFeatures::Get().UnregisterModularFeature(VisualLoggerTabName, this);

	FLogVisualizerStyle::Shutdown();
}

TSharedRef<SDockTab> FNewLogVisualizerModule::SpawnLogVisualizerTab(const FSpawnTabArgs& SpawnTabArgs)
{
	const TSharedRef<SDockTab> MajorTab = SNew(SDockTab)
		.TabRole(ETabRole::MajorTab);

	TSharedPtr<SWidget> TabContent;

	TabContent = SNew(SVisualLogger, MajorTab, SpawnTabArgs.GetOwnerWindow());

	MajorTab->SetContent(TabContent.ToSharedRef());

	return MajorTab;
}

IMPLEMENT_MODULE(FNewLogVisualizerModule, LogVisualizer);
#undef LOCTEXT_NAMESPACE
