// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlutilityPrivatePCH.h"
#include "BlutilityClasses.h"
#include "Blutility.generated.inl"

#include "AssetTypeActions_EditorUtilityBlueprint.h"
#include "AssetToolsModule.h"
#include "WorkspaceMenuStructureModule.h"

#include "BlutilityDetailsPanel.h"
#include "BlutilityShelf.h"

/////////////////////////////////////////////////////

namespace BlutilityModule
{
	static const FName BlutilityShelfApp = FName(TEXT("BlutilityShelfApp"));
}

/////////////////////////////////////////////////////
// FBlutilityModule 

// Blutility module implementation (private)
class FBlutilityModule : public IBlutilityModule
{
public:
	/** Asset type actions for MovieScene assets.  Cached here so that we can unregister it during shutdown. */
	TSharedPtr<FAssetTypeActions_EditorUtilityBlueprint> EditorBlueprintAssetTypeActions;

public:
	virtual void StartupModule() OVERRIDE
	{
		// Register the asset type
		EditorBlueprintAssetTypeActions = MakeShareable(new FAssetTypeActions_EditorUtilityBlueprint);
		FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get().RegisterAssetTypeActions(EditorBlueprintAssetTypeActions.ToSharedRef());
		
		// Register the details customizer
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomPropertyLayout(APlacedEditorUtilityBase::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FEditorUtilityInstanceDetails::MakeInstance));
		PropertyModule.RegisterCustomPropertyLayout(UGlobalEditorUtilityBase::StaticClass(), FOnGetDetailCustomizationInstance::CreateStatic(&FEditorUtilityInstanceDetails::MakeInstance));
		PropertyModule.NotifyCustomizationModuleChanged();

		if (GetDefault<UEditorExperimentalSettings>()->bEnableEditorUtilityBlueprints)
		{
			RegisterBlutilityShelfTabSpawner();
		}
		
		GetMutableDefault<UEditorExperimentalSettings>()->OnSettingChanged().AddRaw(this, &FBlutilityModule::HandleExperimentalSettingChanged);
	}

	virtual void ShutdownModule() OVERRIDE
	{
		if (!UObjectInitialized())
		{
			return;
		}

		UnregisterBlutilityShelfTabSpawner();

		GetMutableDefault<UEditorExperimentalSettings>()->OnSettingChanged().RemoveAll(this);

		// Only unregister if the asset tools module is loaded.  We don't want to forcibly load it during shutdown phase.
		check( EditorBlueprintAssetTypeActions.IsValid() );
		if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
		{
			FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get().UnregisterAssetTypeActions(EditorBlueprintAssetTypeActions.ToSharedRef());
		}
		EditorBlueprintAssetTypeActions.Reset();

		// Unregister the details customization
		if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
		{
			FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
			PropertyModule.UnregisterCustomPropertyLayout(APlacedEditorUtilityBase::StaticClass());
			PropertyModule.UnregisterCustomPropertyLayout(UGlobalEditorUtilityBase::StaticClass());
			PropertyModule.NotifyCustomizationModuleChanged();
		}
	}

protected:
	static TSharedRef<SDockTab> SpawnBlutilityShelfTab(const FSpawnTabArgs& Args)
	{
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			[
				SNew(SBlutilityShelf)
			];
	}

	void RegisterBlutilityShelfTabSpawner()
	{
		FGlobalTabmanager::Get()->RegisterNomadTabSpawner( BlutilityModule::BlutilityShelfApp, FOnSpawnTab::CreateStatic( &SpawnBlutilityShelfTab ) )
			.SetDisplayName(NSLOCTEXT("BlutilityShelf", "TabTitle", "Blutility Shelf"))
			.SetGroup( WorkspaceMenu::GetMenuStructure().GetToolsCategory() );
	}

	void UnregisterBlutilityShelfTabSpawner()
	{
		// Unregister our shelf tab spawner
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(BlutilityModule::BlutilityShelfApp);
	}

	void HandleExperimentalSettingChanged(FName PropertyName)
	{
		if (PropertyName == TEXT("bEnableEditorUtilityBlueprints"))
		{
			UEditorUtilityBlueprintFactory* EditorUtilityBlueprintFactory = Cast<UEditorUtilityBlueprintFactory>(UEditorUtilityBlueprintFactory::StaticClass()->GetDefaultObject());
			if (GetDefault<UEditorExperimentalSettings>()->bEnableEditorUtilityBlueprints)
			{
				RegisterBlutilityShelfTabSpawner();
				EditorUtilityBlueprintFactory->bCreateNew = true;
			}
			else
			{
				UnregisterBlutilityShelfTabSpawner();
				EditorUtilityBlueprintFactory->bCreateNew = false;
			}
		}
	}
};



IMPLEMENT_MODULE( FBlutilityModule, Blutility );
