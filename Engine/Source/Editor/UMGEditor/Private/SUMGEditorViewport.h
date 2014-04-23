// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PreviewScene.h"
#include "SEditorViewport.h"

/**
 * UMG Editor Preview viewport widget
 */
class SUMGEditorViewport : public SEditorViewport, public FGCObject
{
public:
	SLATE_BEGIN_ARGS( SUMGEditorViewport ){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TWeakPtr<IUMGEditor> UMGEditor, UGUIPage* ObjectToEdit);
	~SUMGEditorViewport();
	
	// FGCObject interface
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE;
	// End of FGCObject interface

	void RefreshViewport();

	void SetPreviewPage(UGUIPage* InPage);

	/** @return The editor viewport client */
	class FUMGEditorViewportClient& GetViewportClient();

	/** Set the parent tab of the viewport for determining visibility */
	void SetParentTab( TSharedRef<SDockTab> InParentTab ) { ParentTab = InParentTab; }

protected:
	/** SEditorViewport interface */
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() OVERRIDE;
	virtual EVisibility OnGetViewportContentVisibility() const OVERRIDE;
	virtual void BindCommands() OVERRIDE;
	virtual void OnFocusViewportToSelection() OVERRIDE;
	virtual TSharedPtr<SWidget> MakeViewportToolbar() OVERRIDE;
private:
	/** Determines the visibility of the viewport. */
	bool IsVisible() const;

	/** Callback for toggling the realtime preview flag. */
	void SetRealtimePreview();

	/** Callback for updating preview socket meshes if the gui page or socket has been modified. */
	void OnObjectPropertyChanged(UObject* ObjectBeingModified);
private:
	
	/** The parent tab where this viewport resides */
	TWeakPtr<SDockTab> ParentTab;

	/** Pointer back to the UMG editor tool that owns us */
	TWeakPtr<IUMGEditor> UMGEditorPtr;

	/** The scene for this viewport. */
	FPreviewScene PreviewScene;

	/** Level viewport client */
	TSharedPtr<class FUMGEditorViewportClient> EditorViewportClient;

	/** gui page being edited */
	UGUIPage* Page;

	/** The currently selected view mode. */
	EViewModeIndex CurrentViewMode;
};
