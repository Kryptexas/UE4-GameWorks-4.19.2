// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IDataTableEditor.h"
#include "Toolkits/AssetEditorToolkit.h"


/** Viewer/editor for a DataTable */
class FDataTableEditor :
	public IDataTableEditor
{

public:

	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;

	/**
	 * Edits the specified table
	 *
	 * @param	Mode					Asset editing mode for this editor (standalone or world-centric)
	 * @param	InitToolkitHost			When Mode is WorldCentric, this is the level editor instance to spawn this editor within
	 * @param	Table					The table to edit
	 */
	void InitDataTableEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UDataTable* Table );

	/** Destructor */
	virtual ~FDataTableEditor();

	/* Create the uniform grid panel */
	TSharedPtr<SUniformGridPanel> CreateGridPanel();

	/** IToolkit interface */
	virtual FName GetToolkitFName() const OVERRIDE;
	virtual FText GetBaseToolkitName() const OVERRIDE;
	virtual FString GetWorldCentricTabPrefix() const OVERRIDE;
	virtual FLinearColor GetWorldCentricTabColorScale() const OVERRIDE;

	/* Called when DataTable was reloaded */
	virtual void OnDataTableReloaded() OVERRIDE;


private:

	/**	Spawns the tab with the data table inside */
	TSharedRef<SDockTab> SpawnTab_DataTable( const FSpawnTabArgs& Args );

private:

	/**Border surrounding the GridPanel */
	TSharedPtr<SBorder>	 GridPanelOwner;

	/* The DataTable that is active in the editor */
	UDataTable*	DataTable;

	/**	The tab id for the data table tab */
	static const FName DataTableTabId;
};
