// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "PluginCreatorPrivatePCH.h"

#include "PluginCreatorPlugin.h"
#include "SPluginCreatorTabContent.h"
#include "SlateExtras.h"
#include "PluginCreatorStyle.h"
#include "PluginDescriptorDetails.h"
#include "MultiBoxExtender.h"
#include "IProjectManager.h"
#include "GameProjectUtils.h"

static const FName PCPluginTabName("PluginCreatorPlugin");

#define LOCTEXT_NAMESPACE "PluginCreatorPluginModule"

#include "PluginCreatorCommands.h"

DEFINE_LOG_CATEGORY(PluginCreatorPluginLog);

void FPluginCreatorModule::StartupModule()
{
	// Disallow this plugin in content only projects as they won't compile
	const FProjectDescriptor* CurrentProject = IProjectManager::Get().GetCurrentProject();
	if (CurrentProject == nullptr || CurrentProject->Modules.Num() == 0 || !GameProjectUtils::ProjectHasCodeFiles())
	{
		UE_LOG(PluginCreatorPluginLog, Warning, TEXT("Plugin Creator module disabled for content only projects"));
		return;
	}

	FPluginCreatorStyle::Initialize();
	FPluginCreatorStyle::ReloadTextures();

	FPluginCreatorCommands::Register();
	

	MyPluginCommands = MakeShareable(new FUICommandList);

	MyPluginCommands->MapAction(
		FPluginCreatorCommands::Get().OpenPluginCreator,
		FExecuteAction::CreateRaw(this, &FPluginCreatorModule::MyButtonClicked),
		FCanExecuteAction());

	TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
	MenuExtender->AddMenuExtension("FileProject", EExtensionHook::After, MyPluginCommands, FMenuExtensionDelegate::CreateRaw(this, &FPluginCreatorModule::AddMenuExtension));

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
		PCPluginTabName,
		FOnSpawnTab::CreateRaw(this, &FPluginCreatorModule::HandleSpawnPluginCreatorTab))
		.SetDisplayName(LOCTEXT("PluginCreatorTabTitle", "Plugin Creator"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout("PluginDescriptorObject", FOnGetDetailCustomizationInstance::CreateStatic(&FPluginDescriptorDetails::MakeInstance));
}

void FPluginCreatorModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FPluginCreatorCommands::Get().OpenPluginCreator);
}

TSharedRef<SDockTab> FPluginCreatorModule::HandleSpawnPluginCreatorTab(const FSpawnTabArgs& SpawnTabArgs)
{
	TSharedRef<SDockTab> ResultTab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab);

	TSharedRef<SWidget> TabContentWidget = SNew(SPluginCreatorTabContent, ResultTab);
	ResultTab->SetContent(TabContentWidget);

	return ResultTab;
}

void FPluginCreatorModule::MyButtonClicked() 
{
	FGlobalTabmanager::Get()->InvokeTab(PCPluginTabName);
}

void FPluginCreatorModule::ShutdownModule()
{
	FPluginCreatorStyle::Shutdown();

	FPluginCreatorCommands::Unregister();


	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(PCPluginTabName);

}

FString GetPluginCreatorRootPath()
{
	return FPaths::EnginePluginsDir() / TEXT("Editor/PluginCreator");
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPluginCreatorModule, PluginCreatorPlugin)