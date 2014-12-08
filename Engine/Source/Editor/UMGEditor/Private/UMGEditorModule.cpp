// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UMGEditorPrivatePCH.h"

#include "UMGEditorModule.h"
#include "ModuleManager.h"

#include "AssetToolsModule.h"
#include "AssetTypeActions_WidgetBlueprint.h"
#include "KismetCompilerModule.h"
#include "WidgetBlueprintCompiler.h"

#include "Editor/Sequencer/Public/ISequencerModule.h"
#include "Animation/MarginTrackEditor.h"
#include "Animation/Sequencer2DTransformTrackEditor.h"
#include "IUMGModule.h"
#include "ComponentReregisterContext.h"
#include "WidgetComponent.h"
#include "DesignerCommands.h"
#include "WidgetBlueprint.h"

const FName UMGEditorAppIdentifier = FName(TEXT("UMGEditorApp"));

class FUMGEditorModule : public IUMGEditorModule
{
public:
	/** Constructor, set up console commands and variables **/
	FUMGEditorModule()
	{
	}

	/** Called right after the module DLL has been loaded and the module object has been created */
	virtual void StartupModule() override
	{
		FModuleManager::LoadModuleChecked<IUMGModule>("UMG");

		if (GIsEditor)
		{
			FDesignerCommands::Register();
		}

		MenuExtensibilityManager = MakeShareable(new FExtensibilityManager());
		ToolBarExtensibilityManager = MakeShareable(new FExtensibilityManager());

		// Register widget blueprint compiler we do this no matter what.
		IKismetCompilerInterface& KismetCompilerModule = FModuleManager::LoadModuleChecked<IKismetCompilerInterface>("KismetCompiler");
		FBlueprintCompileDelegate sd;
		sd.BindRaw(this, &FUMGEditorModule::CompileWidgetBlueprint);
		KismetCompilerModule.GetCompilers().Add(sd);

		// Register asset types
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		RegisterAssetTypeAction(AssetTools, MakeShareable(new FAssetTypeActions_WidgetBlueprint()));

		// Register with the sequencer module that we provide auto-key handlers.
		ISequencerModule& SequencerModule = FModuleManager::Get().LoadModuleChecked<ISequencerModule>("Sequencer");
		SequencerModule.RegisterTrackEditor(FOnCreateTrackEditor::CreateStatic(&FMarginTrackEditor::CreateTrackEditor));
		SequencerModule.RegisterTrackEditor(FOnCreateTrackEditor::CreateStatic(&F2DTransformTrackEditor::CreateTrackEditor));
	}

	/** Called before the module is unloaded, right before the module object is destroyed. */
	virtual void ShutdownModule() override
	{
		MenuExtensibilityManager.Reset();
		ToolBarExtensibilityManager.Reset();

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

	FReply CompileWidgetBlueprint(UBlueprint* Blueprint, const FKismetCompilerOptions& CompileOptions, FCompilerResultsLog& Results, TArray<UObject*>* ObjLoaded)
	{
		if ( UWidgetBlueprint* WidgetBlueprint = Cast<UWidgetBlueprint>(Blueprint) )
		{
			{
				TComponentReregisterContext<UWidgetComponent> ComponentReregisterContext;

				FWidgetBlueprintCompiler Compiler(WidgetBlueprint, Results, CompileOptions, ObjLoaded);
				Compiler.Compile();
				check(Compiler.NewClass);
			}

			if ( GIsEditor && GEditor )
			{
				GEditor->RedrawAllViewports(true);
			}

			return FReply::Handled();
		}

		return FReply::Unhandled();
	}

	/** Gets the extensibility managers for outside entities to extend gui page editor's menus and toolbars */
	virtual TSharedPtr<FExtensibilityManager> GetMenuExtensibilityManager() { return MenuExtensibilityManager; }
	virtual TSharedPtr<FExtensibilityManager> GetToolBarExtensibilityManager() { return ToolBarExtensibilityManager; }

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
};

IMPLEMENT_MODULE(FUMGEditorModule, UMGEditor);
