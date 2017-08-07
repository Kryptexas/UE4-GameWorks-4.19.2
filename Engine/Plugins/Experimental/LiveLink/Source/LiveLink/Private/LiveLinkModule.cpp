// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ILiveLinkModule.h"

#include "IPluginManager.h"

#include "Editor.h"

#include "ModuleManager.h"
//#include "CoreDelegates.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"
#include "EditorStyleSet.h"
#include "SlateStyle.h"
#include "SlateTypes.h"
#include "SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"

#include "Features/IModularFeatures.h"
#include "LevelEditor.h"

#include "LiveLinkClient.h"
#include "LiveLinkClientPanel.h"
#include "LiveLinkClientCommands.h"

#include "LiveLinkRemapAssetActions.h"

/**
 * Implements the Messaging module.
 */

#define LOCTEXT_NAMESPACE "LiveLinkModule"

static const FName LiveLinkClientTabName(TEXT("LiveLink"));

#define IMAGE_PLUGIN_BRUSH( RelativePath, ... ) FSlateImageBrush( InPluginContent( RelativePath, ".png" ), __VA_ARGS__ )

FString InPluginContent(const FString& RelativePath, const ANSICHAR* Extension)
{
	static FString ContentDir = IPluginManager::Get().FindPlugin(TEXT("LiveLink"))->GetContentDir();
	return (ContentDir / RelativePath) + Extension;
}

class FLiveLinkModule
	: public ILiveLinkModule
{
public:
	FLiveLinkClient LiveLinkClient;

	TSharedPtr<FSlateStyleSet> StyleSet;

	TSharedPtr< class ISlateStyle > GetStyleSet() { return StyleSet; }

	// IModuleInterface interface

	virtual void StartupModule() override
	{
		IModularFeatures::Get().RegisterModularFeature(FLiveLinkClient::ModularFeatureName, &LiveLinkClient);

		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));

		FLiveLinkClient* ClientPtr = &LiveLinkClient;

		static FName LiveLinkStyle(TEXT("LiveLinkStyle"));
		StyleSet = MakeShareable(new FSlateStyleSet(LiveLinkStyle));

		FLiveLinkClientCommands::Register();

		TSharedPtr<FSlateStyleSet> StyleSetPtr = StyleSet;

		//Register our UI
		LevelEditorTabManagerChangedHandle = LevelEditorModule.OnTabManagerChanged().AddLambda([ClientPtr, StyleSetPtr]()
		{
			FLevelEditorModule& LocalLevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
			LocalLevelEditorModule.GetLevelEditorTabManager()->RegisterTabSpawner(LiveLinkClientTabName, FOnSpawnTab::CreateStatic(&FLiveLinkModule::SpawnLiveLinkTab, ClientPtr, StyleSetPtr))
				.SetGroup(WorkspaceMenu::GetMenuStructure().GetLevelEditorCategory())
				.SetDisplayName(LOCTEXT("LiveLinkTabTitle", "Live Link"))
				.SetTooltipText(LOCTEXT("SequenceRecorderTooltipText", "Open the Live Link streaming manager tab."))
				.SetIcon(FSlateIcon(StyleSetPtr->GetStyleSetName(), "LiveLinkClient.Common.Icon.Small"));
		});

		const FVector2D Icon16x16(16.0f, 16.0f);
		const FVector2D Icon20x20(20.0f, 20.0f);
		const FVector2D Icon40x40(40.0f, 40.0f);

		StyleSet->SetContentRoot(FPaths::EngineContentDir() / TEXT("Editor/Slate"));
		StyleSet->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

		StyleSet->Set("LiveLinkClient.Common.Icon", new IMAGE_PLUGIN_BRUSH(TEXT("LiveLink_40x"), Icon40x40));
		StyleSet->Set("LiveLinkClient.Common.Icon.Small", new IMAGE_PLUGIN_BRUSH(TEXT("LiveLink_16x"), Icon16x16));

		StyleSet->Set("LiveLinkClient.Common.AddSource", new IMAGE_PLUGIN_BRUSH(TEXT("icon_AddSource_40x"), Icon40x40));
		StyleSet->Set("LiveLinkClient.Common.RemoveSource", new IMAGE_PLUGIN_BRUSH(TEXT("icon_RemoveSource_40x"), Icon40x40));
		StyleSet->Set("LiveLinkClient.Common.RemoveAllSources", new IMAGE_PLUGIN_BRUSH(TEXT("icon_RemoveSource_40x"), Icon40x40));

		FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());

		RegisterAssetTypeAction(MakeShareable(new FLiveLinkRemapAssetActions()));
	}

	virtual void ShutdownModule() override
	{
		IModularFeatures::Get().UnregisterModularFeature(FLiveLinkClient::ModularFeatureName, &LiveLinkClient);

		if (FSlateApplication::IsInitialized())
		{
			FGlobalTabmanager::Get()->UnregisterTabSpawner(LiveLinkClientTabName);
		}

		if (FModuleManager::Get().IsModuleLoaded(TEXT("LevelEditor")))
		{
			FLevelEditorModule& LevelEditorModule = FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
			LevelEditorModule.OnTabManagerChanged().Remove(LevelEditorTabManagerChangedHandle);
		}

		FAssetToolsModule* AssetToolsModule = FModuleManager::GetModulePtr<FAssetToolsModule>("AssetTools");
		if (AssetToolsModule)
		{
			for (TSharedRef<IAssetTypeActions> RegisteredAssetTypeAction : RegisteredAssetTypeActions)
			{
				AssetToolsModule->Get().UnregisterAssetTypeActions(RegisteredAssetTypeAction);
			}
		}
	}

	virtual bool SupportsDynamicReloading() override
	{
		return false;
	}

	static TSharedRef<SDockTab> SpawnLiveLinkTab(const FSpawnTabArgs& SpawnTabArgs, FLiveLinkClient* Client, TSharedPtr<FSlateStyleSet> StyleSet)
	{
		const FSlateBrush* IconBrush = StyleSet->GetBrush("LiveLinkClient.Common.Icon.Small");

		const TSharedRef<SDockTab> MajorTab =
			SNew(SDockTab)
			.Icon(IconBrush)
			.TabRole(ETabRole::NomadTab);

		MajorTab->SetContent(SNew(SLiveLinkClientPanel, Client));

		return MajorTab;
	}

private:

	FDelegateHandle LevelEditorTabManagerChangedHandle;

	// Store all module registered asset type actions
	TArray<TSharedRef<IAssetTypeActions>> RegisteredAssetTypeActions;

	void RegisterAssetTypeAction(TSharedRef<IAssetTypeActions> AssetTypeAction)
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		RegisteredAssetTypeActions.Add(AssetTypeAction);
		AssetTools.RegisterAssetTypeActions(AssetTypeAction);
	}
	
};

IMPLEMENT_MODULE(FLiveLinkModule, LiveLink);

#undef LOCTEXT_NAMESPACE