// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "IFlexEditorModule.h"

#include "FlexEditorPluginBridge.h"

#include "AssetTypeActions_FlexContainer.h"
#include "AssetTypeActions_FlexFluidSurface.h"

#include "ContentBrowserExtensions/ContentBrowserExtensions.h"


class FFlexEditorModule : public IFlexEditorModule
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TSharedPtr<IAssetTypeActions>	FlexContainerAssetActions;
	TSharedPtr<IAssetTypeActions>	FlexFluidAssetActions;
};

IMPLEMENT_MODULE( FFlexEditorModule, FlexEditor )



void FFlexEditorModule::StartupModule()
{
	GFlexEditorPluginBridge = new FFlexEditorPluginBridge();

	// Register asset types
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	FlexContainerAssetActions = MakeShareable(new FAssetTypeActions_FlexContainer);
	FlexFluidAssetActions = MakeShareable(new FAssetTypeActions_FlexFluidSurface);

	AssetTools.RegisterAssetTypeActions(FlexContainerAssetActions.ToSharedRef());
	AssetTools.RegisterAssetTypeActions(FlexFluidAssetActions.ToSharedRef());

	if (!IsRunningCommandlet())
	{
		FFlexContentBrowserExtensions::InstallHooks();
	}
}


void FFlexEditorModule::ShutdownModule()
{
	if (UObjectInitialized())
	{
		FFlexContentBrowserExtensions::RemoveHooks();
	}

	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		if (FlexContainerAssetActions.IsValid())
		{
			AssetTools.UnregisterAssetTypeActions(FlexContainerAssetActions.ToSharedRef());
		}

		if (FlexFluidAssetActions.IsValid())
		{
			AssetTools.UnregisterAssetTypeActions(FlexFluidAssetActions.ToSharedRef());
		}
	}

	delete GFlexEditorPluginBridge;
	GFlexEditorPluginBridge = nullptr;
}
