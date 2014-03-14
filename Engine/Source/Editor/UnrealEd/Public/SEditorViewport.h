// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once

class UNREALED_API SEditorViewport : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SEditorViewport) {}
	SLATE_END_ARGS()
	
	SEditorViewport();
	virtual ~SEditorViewport();

	void Construct( const FArguments& InArgs );

	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent ) OVERRIDE;
	virtual bool SupportsKeyboardFocus() const OVERRIDE;
	virtual FReply OnKeyboardFocusReceived( const FGeometry& MyGeometry, const FKeyboardFocusEvent& InKeyboardFocusEvent ) OVERRIDE;

	/**
	 * @return True if the viewport is being updated in realtime
	 */
	bool IsRealtime() const;

	/**
	 * @return true if the specified coordinate system the active one active
	 */
	virtual bool IsCoordSystemActive( ECoordSystem CoordSystem ) const;
	
	/** @return The viewport command list */
	const TSharedPtr<FUICommandList> GetCommandList() const { return CommandList; }

	TSharedPtr<FEditorViewportClient> GetViewportClient() const { return Client; }

protected:
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() = 0;
	virtual TSharedPtr<SWidget> MakeViewportToolbar() = 0;
	virtual void BindCommands();
	virtual const FSlateBrush* OnGetViewportBorderBrush() const { return NULL; }
	virtual FSlateColor OnGetViewportBorderColorAndOpacity() const { return FLinearColor::Black; }
	
	/**
	 * @return The visibility of widgets in the viewport (e.g, menus).  Note this is not the visibility of the scene rendered in the viewport                                                              
	 */
	virtual EVisibility OnGetViewportContentVisibility() const;

	/** UI Command delegate bindings */
	void OnToggleRealtime();
	void OnToggleStats();
	void OnToggleFPS();

	/**
	 * Changes the exposure setting for this viewport
	 *
	 * @param ID	The ID of the new exposure setting
	 */
	void ChangeExposureSetting( int32 ID );

	/**
	 * Checks if an exposure setting is selected
	 * 
	 * @param ID	The ID of the exposure setting to check
	 * @return		true if the exposure setting is checked
	 */
	bool IsExposureSettingSelected( int32 ID ) const;
	

	virtual void OnScreenCapture();
	virtual void OnScreenCaptureForProjectThumbnail();
	virtual bool DoesAllowScreenCapture() { return true; }
	
	/**
	 * Changes the snapping grid size
	 */
	virtual void OnIncrementPositionGridSize() {};
	virtual void OnDecrementPositionGridSize() {};
	virtual void OnIncrementRotationGridSize() {};
	virtual void OnDecrementRotationGridSize() {};

	/**
	 * @return true if the specified widget mode is active
	 */
	virtual bool IsWidgetModeActive( FWidget::EWidgetMode Mode ) const;

	/**
	 * @return true if the translate/rotate mode widget is visible 
	 */
	virtual bool IsTranslateRotateModeVisible() const;

	/**
	 * Moves between widget modes
	 */
	virtual void OnCycleWidgetMode();

	/**
	 * Cycles between world and local coordinate systems
	 */
	virtual void OnCycleCoordinateSystem();

	/**
	 * Called when the user wants to focus the viewport to the current selection
	 */
	virtual void OnFocusViewportToSelection(){}
protected:
	TSharedPtr<SOverlay> ViewportOverlay;
	/** Viewport that renders the scene provided by the viewport client */
	TSharedPtr<class FSceneViewport> SceneViewport;
	/** Widget where the scene viewport is drawn in */
	TSharedPtr<SViewport> ViewportWidget;
	/** The client responsible for setting up the scene */
	TSharedPtr<FEditorViewportClient> Client;
	/** Commandlist used in the viewport (Maps commands to viewport specific actions) */
	TSharedPtr<FUICommandList> CommandList;
};