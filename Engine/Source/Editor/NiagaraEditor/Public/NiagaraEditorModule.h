// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleInterface.h"
#include "Toolkits/IToolkit.h"	// For EAssetEditorMode
#include "Toolkits/AssetEditorToolkit.h" // For FExtensibilityManager

class INiagaraEditor;

/** DataTable Editor module */
class FNiagaraEditorModule : public IModuleInterface,
	public IHasMenuExtensibility, public IHasToolBarExtensibility
{

public:
	// IModuleInterface
	virtual void StartupModule() OVERRIDE;
	virtual void ShutdownModule() OVERRIDE;

	/** Compile the specified script. */
	virtual void CompileScript(class UNiagaraScript* ScriptToCompile);

	/** Creates an instance of Niagara editor.  Only virtual so that it can be called across the DLL boundary. */
	virtual TSharedRef<INiagaraEditor> CreateNiagaraEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, class UNiagaraScript* Script );

	/** Gets the extensibility managers for outside entities to extend static mesh editor's menus and toolbars */
	virtual TSharedPtr<FExtensibilityManager> GetMenuExtensibilityManager() {return MenuExtensibilityManager;}
	virtual TSharedPtr<FExtensibilityManager> GetToolBarExtensibilityManager() {return ToolBarExtensibilityManager;}

	/** Niagara Editor app identifier string */
	static const FName NiagaraEditorAppIdentifier;

private:
	TSharedPtr<FExtensibilityManager> MenuExtensibilityManager;
	TSharedPtr<FExtensibilityManager> ToolBarExtensibilityManager;
};


