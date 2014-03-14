// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

//////////////////////////////////////////////////////////////////////////
// FGlobalBlutilityDialog

class FGlobalBlutilityDialog : public FAssetEditorToolkit, public FGCObject
{
public:
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;
	TSharedRef<SDockTab> SpawnTab_DetailsPanel( const FSpawnTabArgs& SpawnTabArgs );

	/**
	 * Edits the specified sound class object
	 *
	 * @param	Mode					Asset editing mode for this editor (standalone or world-centric)
	 * @param	InitToolkitHost			When Mode is WorldCentric, this is the level editor instance to spawn this editor within
	 * @param	ObjectToEdit			The sound class to edit
	 */
	void InitBlutilityDialog(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UObject* ObjectToEdit);

	/**
	 * Updates the editor's property window to contain sound class properties if any are selected.
	 */
	void UpdatePropertyWindow( const TArray<UObject*>& SelectedObjects );

	virtual ~FGlobalBlutilityDialog();

	/** FGCObject interface */
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE;

	/** IToolkit interface */
	virtual FName GetToolkitFName() const OVERRIDE;
	virtual FText GetBaseToolkitName() const OVERRIDE;
	virtual FString GetWorldCentricTabPrefix() const OVERRIDE;

	/** @return Returns the color and opacity to use for the color that appears behind the tab text for this toolkit's tab in world-centric mode. */
	virtual FLinearColor GetWorldCentricTabColorScale() const OVERRIDE;

private:

	/** Creates all internal widgets for the tabs to point at */
	void CreateInternalWidgets();

private:
	/** List of open tool panels; used to ensure only one exists at any one time */
	TMap< FName, TWeakPtr<class SDockableTab> > SpawnedToolPanels;

	/** Property View */
	TSharedPtr<class IDetailsView> DetailsView;

	TWeakObjectPtr<UGlobalEditorUtilityBase> BlutilityInstance;
};