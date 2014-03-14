// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WorkflowCentricApplication.h"
#include "SPaperEditorViewport.h"

//////////////////////////////////////////////////////////////////////////
// FTileSetEditor

class FTileSetEditor : public FWorkflowCentricApplication, public FGCObject
{
public:
	// IToolkit interface
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;
	// End of IToolkit interface

	// FAssetEditorToolkit
	virtual FName GetToolkitFName() const OVERRIDE;
	virtual FText GetBaseToolkitName() const OVERRIDE;
	virtual FText GetToolkitName() const OVERRIDE;
	virtual FLinearColor GetWorldCentricTabColorScale() const OVERRIDE;
	virtual FString GetWorldCentricTabPrefix() const OVERRIDE;
	// End of FAssetEditorToolkit

	// FSerializableObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) OVERRIDE;
	// End of FSerializableObject interface

public:
	void InitTileSetEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, class UPaperTileSet* InitTileSet);

	UPaperTileSet* GetTileSetBeingEdited() const { return TileSetBeingEdited; }
protected:
	UPaperTileSet* TileSetBeingEdited;
};

//////////////////////////////////////////////////////////////////////////
// STileSetSelectorViewport

class STileSetSelectorViewport : public SPaperEditorViewport
{
public:
	SLATE_BEGIN_ARGS(STileSetSelectorViewport) {}
	SLATE_END_ARGS()

	~STileSetSelectorViewport();

	void Construct(const FArguments& InArgs, UPaperTileSet* InTileSet);

	void ChangeTileSet(UPaperTileSet* InTileSet);
protected:
	// SPaperEditorViewport interface
	virtual FString GetTitleText() const OVERRIDE;
	// End of SPaperEditorViewport interface

private:
	void OnSelectionChanged(FMarqueeOperation Marquee, bool bIsPreview);

private:
	TWeakObjectPtr<class UPaperTileSet> TileSetPtr;
	TSharedPtr<class FTileSetEditorViewportClient> TypedViewportClient;
};