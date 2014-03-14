// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Toolkits/BaseToolkit.h"

//////////////////////////////////////////////////////////////////////////
// FTileMapEdModeToolkit

class FTileMapEdModeToolkit : public FModeToolkit
{
public:
	// IToolkit interface
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;
	virtual FName GetToolkitFName() const OVERRIDE;
	virtual FText GetBaseToolkitName() const OVERRIDE;
	virtual FText GetToolkitName() const OVERRIDE;
	virtual class FEdMode* GetEditorMode() const OVERRIDE;
	virtual TSharedPtr<class SWidget> GetInlineContent() const OVERRIDE;
	// End of IToolkit interface

	// FModeToolkit interface
	virtual void Init(const TSharedPtr<class IToolkitHost>& InitToolkitHost) OVERRIDE;
	// End of FModeToolkit interface

protected:
	void OnChangeTileSet(UObject* NewAsset);
	UObject* GetCurrentTileSet() const;

	void BindCommands();

	EVisibility GetTileSetSelectorVisibility() const;

	void OnSelectTool(ETileMapEditorTool::Type NewTool);
	bool IsToolSelected(ETileMapEditorTool::Type QueryTool) const;

	void OnSelectLayerPaintingMode(ETileMapLayerPaintingMode::Type NewMode);
	bool IsLayerPaintingModeSelected(ETileMapLayerPaintingMode::Type PaintingMode) const;

	TSharedRef<SWidget> BuildToolBar() const;
private:
	TWeakObjectPtr<class UPaperTileSet> CurrentTileSetPtr;

	// All of the inline content for this toolkit
	TSharedPtr<class SWidget> MyWidget;

	// The tile set selector palette
	TSharedPtr<class STileSetSelectorViewport> TileSetPalette;

	// Command list for binding functions for the toolbar.
	TSharedPtr<FUICommandList> UICommandList;
};