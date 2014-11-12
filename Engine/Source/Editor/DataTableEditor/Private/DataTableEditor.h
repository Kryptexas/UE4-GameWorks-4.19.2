// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IDataTableEditor.h"
#include "Toolkits/AssetEditorToolkit.h"

class UDataTable;

/** Viewer/editor for a DataTable */
class FDataTableEditor :
	public IDataTableEditor
{

public:

	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;

	/**
	 * Edits the specified table
	 *
	 * @param	Mode					Asset editing mode for this editor (standalone or world-centric)
	 * @param	InitToolkitHost			When Mode is WorldCentric, this is the level editor instance to spawn this editor within
	 * @param	Table					The table to edit
	 */
	void InitDataTableEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UDataTable* Table );

	/** Constructor */
	FDataTableEditor();

	/** Destructor */
	virtual ~FDataTableEditor();

	/* Create the uniform grid panel */
	TSharedPtr<SUniformGridPanel> CreateGridPanel();

	/** IToolkit interface */
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;

	/* Called when DataTable was reloaded */
	virtual void OnDataTableReloaded() override;


private:

	/** Called when an object is reimported */
	void OnPostReimport(UObject* InObject, bool bSuccess);

	void ReloadVisibleData();

	void OnSearchTextChanged(const FText& SearchText);

	TSharedRef<SVerticalBox> CreateContentBox();

	/**	Spawns the tab with the data table inside */
	TSharedRef<SDockTab> SpawnTab_DataTable( const FSpawnTabArgs& Args );

private:

	/** Cached table data */
	TArray<TArray<FString> > CachedDataTable;

	/** Visibility of data table rows */
	TArray<bool> RowsVisibility;

	/** Search box */
	TSharedPtr<SWidget> SearchBox;

	/** Scroll containing data table */
	TSharedPtr<SScrollBox> ScrollBoxWidget;

	/**Border surrounding the GridPanel */
	TSharedPtr<SBorder>	 GridPanelOwner;

	/* The DataTable that is active in the editor */
	UDataTable*	DataTable;

	/**	The tab id for the data table tab */
	static const FName DataTableTabId;
};
