// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "LogVisualizer.h"
#include "LogVisualizerModule.h"
#include "LogVisualizerStyle.h"
#include "SDockTab.h"
#include "VisualLoggerRenderingActor.h"
#include "LogVisualizerSettings.h"
#if WITH_EDITOR
#include "Editor/EditorEngine.h"
#include "ISettingsModule.h"
#include "LevelEditor.h"
#endif // WITH_EDITOR

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

	UEditorExperimentalSettings::StaticClass()->GetDefaultObject<UEditorExperimentalSettings>()->OnSettingChanged().AddRaw(this, &FNewLogVisualizerModule::HandleExperimentalSettingChanged);
	
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if (SettingsModule != nullptr)
	{
		SettingsModule->RegisterSettings("Editor", "General", "VisualLogger",
			LOCTEXT("AIToolsSettingsName", "Visual Logger"),
			LOCTEXT("AIToolsSettingsDescription", "General settings for UE4 AI Tools."),
			ULogVisualizerSettings::StaticClass()->GetDefaultObject()
			);
	}
}

void FNewLogVisualizerModule::ShutdownModule()
{
	FGlobalTabmanager::Get()->UnregisterTabSpawner(VisualLoggerTabName);
	FVisualLoggerCommands::Unregister();
	IModularFeatures::Get().UnregisterModularFeature(VisualLoggerTabName, this);
	
	//UEditorExperimentalSettings::StaticClass()->GetDefaultObject<UEditorExperimentalSettings>()->OnSettingChanged().RemoveAll(this);

	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
	if (SettingsModule != nullptr)
	{
		SettingsModule->UnregisterSettings("Editor", "General", "VisualLogger");
	}

	FLogVisualizerStyle::Shutdown();
}

#if WITH_EDITOR

void FNewLogVisualizerModule::HandleExperimentalSettingChanged(FName PropertyName)
{
	if (PropertyName == TEXT("bVisualLogger"))
	{
		if (UEditorExperimentalSettings::StaticClass()->GetDefaultObject<UEditorExperimentalSettings>()->bVisualLogger == false)
		{
			ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
			if (SettingsModule != nullptr)
			{
				SettingsModule->UnregisterSettings("Editor", "General", "VisualLogger");
			}
		}
		else
		{
			ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
			if (SettingsModule != nullptr)
			{
				SettingsModule->RegisterSettings("Editor", "General", "VisualLogger",
					LOCTEXT("AIToolsSettingsName", "Visual Logger"),
					LOCTEXT("AIToolsSettingsDescription", "General settings for UE4 AI Tools."),
					ULogVisualizerSettings::StaticClass()->GetDefaultObject()
					);
			}
		}
	}
}
#endif

TSharedRef<SDockTab> FNewLogVisualizerModule::SpawnLogVisualizerTab(const FSpawnTabArgs& SpawnTabArgs)
{
	const TSharedRef<SDockTab> MajorTab = SNew(SDockTab)
		.TabRole(ETabRole::MajorTab).OnTabClosed(SDockTab::FOnTabClosedCallback::CreateRaw(this, &FNewLogVisualizerModule::OnTabClosed));

	TSharedPtr<SWidget> TabContent;

	TabContent = SNew(SVisualLogger, MajorTab, SpawnTabArgs.GetOwnerWindow());

	MajorTab->SetContent(TabContent.ToSharedRef());

	return MajorTab;
}

void FNewLogVisualizerModule::OnTabClosed(TSharedRef<SDockTab>)
{
	UWorld* World = NULL;
#if WITH_EDITOR
	UEditorEngine *EEngine = Cast<UEditorEngine>(GEngine);
	if (GIsEditor && EEngine != NULL)
	{
		// lets use PlayWorld during PIE/Simulate and regular world from editor otherwise, to draw debug information
		World = EEngine->PlayWorld != NULL ? EEngine->PlayWorld : EEngine->GetEditorWorldContext().World();

	}
	else 
#endif
	if (!GIsEditor)
	{

		World = GEngine->GetWorld();
	}

	if (World == NULL)
	{
		World = GWorld;
	}

	if (World)
	{
		for (TActorIterator<AVisualLoggerRenderingActor> It(World); It; ++It)
		{
			World->DestroyActor(*It);
		}
	}
}

IMPLEMENT_MODULE(FNewLogVisualizerModule, LogVisualizer);
#undef LOCTEXT_NAMESPACE
