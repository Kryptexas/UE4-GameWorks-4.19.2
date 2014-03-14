// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Toolkits/AssetEditorToolkit.h"
#include "Toolkits/AssetEditorManager.h"

//////////////////////////////////////////////////////////////////////////
// FFlipbookEditor

class FFlipbookEditor : public FAssetEditorToolkit, public FGCObject
{
public:
	// IToolkit interface
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;
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
	void InitFlipbookEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, class UPaperFlipbook* InitSprite);

	UPaperFlipbook* GetFlipbookBeingEdited() const { return FlipbookBeingEdited; }
protected:
	UPaperFlipbook* FlipbookBeingEdited;
	TSharedPtr<class SFlipbookEditorViewport> ViewportPtr;
	float PlayTime;

protected:
	void BindCommands();
	void ExtendMenu();
	void ExtendToolbar();

	float GetPlayTime() const { return PlayTime; }

	TSharedRef<SDockTab> SpawnTab_Viewport(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnTab_Details(const FSpawnTabArgs& Args);
};