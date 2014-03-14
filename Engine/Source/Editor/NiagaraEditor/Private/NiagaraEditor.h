// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "INiagaraEditor.h"
#include "Toolkits/AssetEditorToolkit.h"


/** Viewer/editor for a DataTable */
class FNiagaraEditor : public INiagaraEditor
{

public:

	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;

	/** Edits the specified Niagara Script */
	void InitNiagaraEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UNiagaraScript* Script );

	/** Destructor */
	virtual ~FNiagaraEditor();

	// Begin IToolkit interface
	virtual FName GetToolkitFName() const OVERRIDE;
	virtual FText GetBaseToolkitName() const OVERRIDE;
	virtual FString GetWorldCentricTabPrefix() const OVERRIDE;
	virtual FLinearColor GetWorldCentricTabColorScale() const OVERRIDE;
	// End IToolkit interface

	// Delegates
	FReply OnCompileClicked();
	void DeleteSelectedNodes();
	bool CanDeleteNodes() const;

private:
	/** Create widget for graph editing */
	TSharedRef<class SGraphEditor> CreateGraphEditorWidget(UEdGraph* InGraph);

	/** Spawns the tab with the update graph inside */
	TSharedRef<SDockTab> SpawnTab_UpdateGraph(const FSpawnTabArgs& Args);

	/** Builds the toolbar widget */
	void ExtendToolbar();
private:

	/* The Script being edited */
	UNiagaraScript*				Script;
	/** Source of script being edited */
	UNiagaraScriptSource*		Source;
	/** */
	TWeakPtr<SGraphEditor>		UpdateGraphEditorPtr;

	/** The command list for this editor */
	TSharedPtr<FUICommandList> GraphEditorCommands;

	/**	Graph editor tab */
	static const FName UpdateGraphTabId;
};
