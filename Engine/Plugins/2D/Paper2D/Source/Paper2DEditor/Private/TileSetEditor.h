// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Toolkits/AssetEditorToolkit.h"
#include "Toolkits/AssetEditorManager.h"
#include "SPaperEditorViewport.h"

//////////////////////////////////////////////////////////////////////////
// FTileSetEditor

class FTileSetEditor : public FAssetEditorToolkit, public FGCObject
{
public:
	FTileSetEditor();
	~FTileSetEditor();

	// IToolkit interface
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) override;
	// End of IToolkit interface

	// FAssetEditorToolkit
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FText GetToolkitName() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FString GetDocumentationLink() const override;
	// End of FAssetEditorToolkit

	// FSerializableObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	// End of FSerializableObject interface

public:
	void InitTileSetEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, class UPaperTileSet* InitTileSet);

	UPaperTileSet* GetTileSetBeingEdited() const { return TileSetBeingEdited; }

protected:
	TSharedRef<class SDockTab> SpawnTab_TextureView(const FSpawnTabArgs& Args);
	TSharedRef<class SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args);
	TSharedRef<class SDockTab> SpawnTab_SingleTileEditor(const FSpawnTabArgs& Args);

	void BindCommands();
	void ExtendMenu();
	void ExtendToolbar();

	void OnPropertyChanged(UObject* ObjectBeingModified, FPropertyChangedEvent& PropertyChangedEvent);

protected:
	UPaperTileSet* TileSetBeingEdited;

	TSharedPtr<class STileSetSelectorViewport> TileSetViewport;
	TSharedPtr<class FSingleTileEditorViewportClient> TileEditorViewportClient;

	FDelegateHandle OnPropertyChangedHandle;
};

//////////////////////////////////////////////////////////////////////////
// STileSetSelectorViewport

class STileSetSelectorViewport : public SPaperEditorViewport
{
public:
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnTileViewportSelectionChanged, const FIntPoint& /*TopLeft*/, const FIntPoint& /*Dimensions*/);

	SLATE_BEGIN_ARGS(STileSetSelectorViewport) {}
	SLATE_END_ARGS()

	~STileSetSelectorViewport();

	void Construct(const FArguments& InArgs, UPaperTileSet* InTileSet, class FEdModeTileMap* InTileMapEditor);

	void ChangeTileSet(UPaperTileSet* InTileSet);

	FOnTileViewportSelectionChanged& GetTileSelectionChanged()
	{
		return OnTileSelectionChanged;
	}

protected:
	// SPaperEditorViewport interface
	virtual FText GetTitleText() const override;
	// End of SPaperEditorViewport interface

private:
	void OnSelectionChanged(FMarqueeOperation Marquee, bool bIsPreview);
	void RefreshSelectionRectangle();

private:
	TWeakObjectPtr<class UPaperTileSet> TileSetPtr;
	TSharedPtr<class FTileSetEditorViewportClient> TypedViewportClient;
	class FEdModeTileMap* TileMapEditor;
	FIntPoint SelectionTopLeft;
	FIntPoint SelectionDimensions;

	FOnTileViewportSelectionChanged OnTileSelectionChanged;
};