// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "UMGEditorModule.h"
#include "ModuleManager.h"
#include "UMGEditor.h"
#include "AssetToolsModule.h"
#include "UMGBlueprintEditorExtensionHook.h"
#include "AssetTypeActions_WidgetBlueprint.h"

#include "UMGEditor.generated.inl"

const FName UMGEditorAppIdentifier = FName(TEXT("UMGEditorApp"));

class FUMGEditorModule : public IUMGEditorModule
{
public:
	/** Constructor, set up console commands and variables **/
	FUMGEditorModule()
	{
	}

	/** Called right after the module DLL has been loaded and the module object has been created */
	virtual void StartupModule() OVERRIDE
	{
		FModuleManager::LoadModuleChecked<IUMGModule>("UMG");

		MenuExtensibilityManager = MakeShareable(new FExtensibilityManager());
		ToolBarExtensibilityManager = MakeShareable(new FExtensibilityManager());

		// Register asset types
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		RegisterAssetTypeAction(AssetTools, MakeShareable(new FAssetTypeActions_WidgetBlueprint()));
		
		FUMGBlueprintEditorExtensionHook::InstallHooks();
	}

	/** Called before the module is unloaded, right before the module object is destroyed. */
	virtual void ShutdownModule() OVERRIDE
	{
		MenuExtensibilityManager.Reset();
		ToolBarExtensibilityManager.Reset();

		if ( UObjectInitialized() )
		{
			FUMGBlueprintEditorExtensionHook::InstallHooks();
		}

		// Unregister all the asset types that we registered
		if ( FModuleManager::Get().IsModuleLoaded("AssetTools") )
		{
			IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
			for ( int32 Index = 0; Index < CreatedAssetTypeActions.Num(); ++Index )
			{
				AssetTools.UnregisterAssetTypeActions(CreatedAssetTypeActions[Index].ToSharedRef());
			}
		}
		CreatedAssetTypeActions.Empty();
	}

	virtual TSharedRef<IUMGEditor> CreateUMGEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UParticleSystem* ParticleSystem) OVERRIDE
	{
		TSharedRef<FUMGEditor> NewUMGEditor(new FUMGEditor());
		NewUMGEditor->Initialize(Mode, InitToolkitHost, ParticleSystem);
		UMGEditorToolkits.Add(&*NewUMGEditor);
		return NewUMGEditor;
	}

	virtual void UMGEditorClosed(FUMGEditor* UMGEditorInstance) OVERRIDE
	{
		UMGEditorToolkits.Remove(UMGEditorInstance);
	}

	virtual void RefreshUMGEditor(UParticleSystem* ParticleSystem) OVERRIDE
	{
		for (int32 Idx = 0; Idx < UMGEditorToolkits.Num(); ++Idx)
		{
			//if (UMGEditorToolkits[Idx]->GetParticleSystem() == ParticleSystem)
			//{
			//	UMGEditorToolkits[Idx]->ForceUpdate();
			//}
		}
	}

	virtual void ConvertModulesToSeeded(UParticleSystem* ParticleSystem) OVERRIDE
	{
		//FUMGEditor::ConvertAllModulesToSeeded( ParticleSystem );
	}

	/** Gets the extensibility managers for outside entities to extend gui page editor's menus and toolbars */
	virtual TSharedPtr<FExtensibilityManager> GetMenuExtensibilityManager() {return MenuExtensibilityManager;}
	virtual TSharedPtr<FExtensibilityManager> GetToolBarExtensibilityManager() {return ToolBarExtensibilityManager;}

private:
	void RegisterAssetTypeAction(IAssetTools& AssetTools, TSharedRef<IAssetTypeActions> Action)
	{
		AssetTools.RegisterAssetTypeActions(Action);
		CreatedAssetTypeActions.Add(Action);
	}

private:
	TSharedPtr<FExtensibilityManager> MenuExtensibilityManager;
	TSharedPtr<FExtensibilityManager> ToolBarExtensibilityManager;

	/** All created asset type actions.  Cached here so that we can unregister it during shutdown. */
	TArray< TSharedPtr<IAssetTypeActions> > CreatedAssetTypeActions;

	/** List of open UMGEditor toolkits */
	TArray<FUMGEditor*> UMGEditorToolkits;
};

IMPLEMENT_MODULE(FUMGEditorModule, UMGEditor);
