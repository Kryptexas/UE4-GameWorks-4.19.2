// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BaseToolkit.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"



class UNREALED_API FSimpleAssetEditor : public FAssetEditorToolkit
{
public:
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;


	/**
	 * Edits the specified asset object
	 *
	 * @param	Mode					Asset editing mode for this editor (standalone or world-centric)
	 * @param	InitToolkitHost			When Mode is WorldCentric, this is the level editor instance to spawn this editor within
	 * @param	ObjectToEdit			The object to edit
	 */
	void InitEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, const TArray<UObject*>& ObjectToEdit );

	/** Destructor */
	virtual ~FSimpleAssetEditor();

	/** IToolkit interface */
	virtual FName GetToolkitFName() const OVERRIDE;
	virtual FText GetBaseToolkitName() const OVERRIDE;
	virtual FText GetToolkitName() const OVERRIDE;
	virtual FString GetWorldCentricTabPrefix() const OVERRIDE;
	virtual FLinearColor GetWorldCentricTabColorScale() const OVERRIDE;
	
	/** Used to show or hide certain properties */
	void SetPropertyVisibilityDelegate(FIsPropertyVisible InVisibilityDelegate);

private:
	/** Create the properties tab and its content */
	TSharedRef<SDockTab> SpawnPropertiesTab( const FSpawnTabArgs& Args );

	/** Dockable tab for properties */
	TSharedPtr< SDockableTab > PropertiesTab;

	/** Details view */
	TSharedPtr< class IDetailsView > DetailsView;

	/** App Identifier. Technically, all simple editors are the same app, despite editing a variety of assets. */
	static const FName SimpleEditorAppIdentifier;

	/**	The tab ids for all the tabs used */
	static const FName PropertiesTabId;

public:
	static TSharedRef<FSimpleAssetEditor> CreateEditor( const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UObject* ObjectToEdit );

	static TSharedRef<FSimpleAssetEditor> CreateEditor( const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, const TArray<UObject*>& ObjectsToEdit );
};
