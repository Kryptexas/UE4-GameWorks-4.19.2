// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleInterface.h"
#include "Toolkits/IToolkit.h"	// For EAssetEditorMode

DECLARE_LOG_CATEGORY_EXTERN(LogBehaviorTreeEditor, Log, All);

class IBehaviorTreeEditor;

/** DataTable Editor module */
class FBehaviorTreeEditorModule : public IModuleInterface,
	public IHasMenuExtensibility, public IHasToolBarExtensibility
{

public:
	// IModuleInterface
	virtual void StartupModule() OVERRIDE;
	virtual void ShutdownModule() OVERRIDE;

	/** Compile the specified script. */
	//virtual void CompileScript(class UNiagaraScript* ScriptToCompile);

	/** Creates an instance of Niagara editor.  Only virtual so that it can be called across the DLL boundary. */
	virtual TSharedRef<IBehaviorTreeEditor> CreateBehaviorTreeEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, class UBehaviorTree* Script );

	/** Gets the extensibility managers for outside entities to extend static mesh editor's menus and toolbars */
	virtual TSharedPtr<FExtensibilityManager> GetMenuExtensibilityManager() {return MenuExtensibilityManager;}
	virtual TSharedPtr<FExtensibilityManager> GetToolBarExtensibilityManager() {return ToolBarExtensibilityManager;}

	TSharedPtr<struct FClassBrowseHelper> GetClassCache() { return ClassCache; }

	/** Niagara Editor app identifier string */
	static const FName BehaviorTreeEditorAppIdentifier;

private:
	void HandleExperimentalSettingChanged(FName PropertyName);

	TSharedPtr<FExtensibilityManager> MenuExtensibilityManager;
	TSharedPtr<FExtensibilityManager> ToolBarExtensibilityManager;

	/** Asset type actions */
	TSharedPtr<class FAssetTypeActions_BehaviorTree> ItemDataAssetTypeActions;

	TSharedPtr<struct FClassBrowseHelper> ClassCache;
};


