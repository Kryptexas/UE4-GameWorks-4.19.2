// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Toolkits/AssetEditorToolkit.h"


/*-----------------------------------------------------------------------------
   FFontEditor
-----------------------------------------------------------------------------*/

class FFontEditor : public IFontEditor, public FGCObject, public FNotifyHook, public FEditorUndoClient
{
public:
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;

	/** Destructor */
	virtual ~FFontEditor();

	/** Edits the specified Font object */
	void InitFontEditor(const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UObject* ObjectToEdit);

	/** IFontEditor interface */
	virtual UFont* GetFont() const OVERRIDE;
	virtual void SetSelectedPage(int32 PageIdx) OVERRIDE;
	
	/** IToolkit interface */
	virtual FName GetToolkitFName() const OVERRIDE;
	virtual FText GetBaseToolkitName() const OVERRIDE;
	virtual FString GetWorldCentricTabPrefix() const OVERRIDE;
	virtual FLinearColor GetWorldCentricTabColorScale() const OVERRIDE;

	/** FGCObject interface */
	virtual void AddReferencedObjects(FReferenceCollector& Collector) OVERRIDE;

	/** Called to determine if the user should be prompted for a new file if one is missing during an asset reload*/
	virtual bool ShouldPromptForNewFilesOnReload(const UObject& object) const OVERRIDE;

protected:
	/** Called when the preview text changes */
	void OnPreviewTextChanged(const FText& Text);
	
	// Begin FEditorUndoClient Interface
	/** Handles any post undo cleanup of the GUI so that we don't have stale data being displayed. */
	virtual void PostUndo(bool bSuccess) OVERRIDE;
	virtual void PostRedo(bool bSuccess) OVERRIDE { PostUndo(bSuccess); }
	// End of FEditorUndoClient

private:
	/** FNotifyHook interface */
	virtual void NotifyPostChange( const FPropertyChangedEvent& PropertyChangedEvent, class FEditPropertyChain* PropertyThatChanged) OVERRIDE;

	/** Creates all internal widgets for the tabs to point at */
	void CreateInternalWidgets();

	/** Builds the toolbar widget for the font editor */
	void ExtendToolbar();
	
	/**	Binds our UI commands to delegates */
	void BindCommands();

	/** Toolbar command methods */
	void OnUpdate();
	bool OnUpdateEnabled() const;
	void OnUpdateAll();
	void OnExport();
	bool OnExportEnabled() const;
	void OnExportAll();
	void OnBackgroundColor();
	bool OnBackgroundColorEnabled() const;
	void OnForegroundColor();
	bool OnForegroundColorEnabled() const;
	void OnPostReimport(UObject* InObject, bool bSuccess);

	/** Common method for replacing a font page with a new texture */
	bool ImportPage(int32 PageNum, const TCHAR* FileName);

	/**	Spawns the viewport tab */
	TSharedRef<SDockTab> SpawnTab_Viewport( const FSpawnTabArgs& Args );

	/**	Spawns the preview tab */
	TSharedRef<SDockTab> SpawnTab_Preview( const FSpawnTabArgs& Args );

	/**	Spawns the properties tab */
	TSharedRef<SDockTab> SpawnTab_Properties( const FSpawnTabArgs& Args );

	/**	Spawns the page properties tab */
	TSharedRef<SDockTab> SpawnTab_PageProperties( const FSpawnTabArgs& Args );

	/**	Caches the specified tab for later retrieval */
	void AddToSpawnedToolPanels( const FName& TabIdentifier, const TSharedRef<SDockTab>& SpawnedTab );

	/** Callback when an object is reimported, handles steps needed to keep the editor up-to-date. */
	void OnObjectReimported(UObject* InObject);

private:
	/** The font asset being inspected */
	UFont* Font;

	/** List of open tool panels; used to ensure only one exists at any one time */
	TMap< FName, TWeakPtr<SDockTab> > SpawnedToolPanels;

	/** Viewport */
	TSharedPtr<SFontEditorViewport> FontViewport;

	/** Preview tab */
	TSharedPtr<SVerticalBox> FontPreview;

	/** Properties tab */
	TSharedPtr<class IDetailsView> FontProperties;

	/** Page properties tab */
	TSharedPtr<class IDetailsView> FontPageProperties;

	/** Preview viewport widget */
	TSharedPtr<SFontEditorViewport> FontPreviewWidget;

	/** Preview text */
	TSharedPtr<SEditableTextBox> FontPreviewText;

	/** The last path exported to or from */
	static FString LastPath;
	
	/** The exporter to use for all font page exporting */
	UTextureExporterTGA* TGAExporter;

	/** The factory to create updated pages with */
	UTextureFactory* Factory;

	/**	The tab ids for the font editor */
	static const FName ViewportTabId;
	static const FName PreviewTabId;
	static const FName PropertiesTabId;
	static const FName PagePropertiesTabId;
};
