// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ICurveTableEditor.h"
#include "Toolkits/AssetEditorToolkit.h"


/** Viewer/editor for a CurveTable */
class FCurveTableEditor :
	public ICurveTableEditor
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
	void InitCurveTableEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UCurveTable* Table );

	/** Destructor */
	virtual ~FCurveTableEditor();

	/** IToolkit interface */
	virtual FName GetToolkitFName() const OVERRIDE;
	virtual FText GetBaseToolkitName() const OVERRIDE;
	virtual FString GetWorldCentricTabPrefix() const OVERRIDE;
	virtual FLinearColor GetWorldCentricTabColorScale() const OVERRIDE;

	/**	Spawns the tab with the curve table inside */
	TSharedRef<SDockTab> SpawnTab_CurveTable( const FSpawnTabArgs& Args );

private:

	/**	The tab id for the curve table tab */
	static const FName CurveTableTabId;
};