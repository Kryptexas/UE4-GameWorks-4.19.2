// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "ILevelViewport.h"
#include "Editor/UnrealEd/Public/SEditorViewport.h"
	
/**
 * Encapsulates an SViewport and an SLevelViewportToolBar
 */
class SLevelViewport : public SEditorViewport, public ILevelViewport
{
public:
	SLATE_BEGIN_ARGS( SLevelViewport )
		: _ViewportType( LVT_Perspective )
		, _Realtime( false )
		{}

		SLATE_ARGUMENT( TSharedPtr<class FLevelViewportLayout>, ParentLayout )
		SLATE_ARGUMENT( TWeakPtr<class SLevelEditor>, ParentLevelEditor )
		SLATE_ARGUMENT( ELevelViewportType, ViewportType )
		SLATE_ARGUMENT( bool, Realtime )
		SLATE_ARGUMENT( FString, ConfigKey )
	SLATE_END_ARGS()

	SLevelViewport();
	~SLevelViewport();


	/**
	 * Constructs the viewport widget                   
	 */
	void Construct(const FArguments& InArgs);

	/**
	 * Constructs the widgets for the viewport overlay
	 */
	void ConstructViewportOverlayContent();

	/**
	 * Constructs the level editor viewport client
	 */
	void ConstructLevelEditorViewportClient( const FArguments& InArgs );

	/**
	 * @return true if the viewport is visible. false otherwise                  
	 */
	bool IsVisible() const;

	/**
	 * @return The editor client for this viewport
	 */
	const FLevelEditorViewportClient& GetLevelViewportClient() const 
	{		
		return *LevelViewportClient;
	}

	virtual FLevelEditorViewportClient& GetLevelViewportClient() OVERRIDE
	{		
		return *LevelViewportClient;
	}

	/**
	 * Sets Slate keyboard focus to this viewport
	 */
	void SetKeyboardFocusToThisViewport();

	/**
	 * @return The list of commands on the viewport that are bound to delegates                    
	 */
	const TSharedPtr<FUICommandList>& GetCommandList() const { return CommandList; }

	/** Saves this viewport's config to ULevelEditorViewportSettings */
	void SaveConfig(const FString& ConfigName);

	/** ILevelViewport Interface */
	virtual void StartPlayInEditorSession( UGameViewportClient* PlayClient ) OVERRIDE;
	virtual void EndPlayInEditorSession() OVERRIDE;
	virtual void SwapViewportsForSimulateInEditor() OVERRIDE;
	virtual void SwapViewportsForPlayInEditor() OVERRIDE;
	virtual void OnSimulateSessionStarted() OVERRIDE;
	virtual void OnSimulateSessionFinished() OVERRIDE;
	virtual void RegisterGameViewportIfPIE() OVERRIDE;
	virtual bool HasPlayInEditorViewport() const OVERRIDE; 
	virtual FViewport* GetActiveViewport() OVERRIDE;
	virtual TSharedRef< const SWidget> AsWidget() OVERRIDE { return AsShared(); }

	/** SEditorViewport Interface */
	virtual void OnFocusViewportToSelection() OVERRIDE;

	/**
	 * Called when the maximize command is executed                   
	 */
	FReply OnToggleMaximize();

	/**
	 * @return true if this viewport is maximized, false otherwise
	 */
	bool IsMaximized() const;

	/**
	 * Attempts to switch this viewport into immersive mode
	 *
	 * @param	bWantImmersive Whether to switch to immersive mode, or switch back to normal mode
	 * @param	bAllowAnimation	True to allow animation when transitioning, otherwise false
	 */
	void MakeImmersive( const bool bWantImmersive, const bool bAllowAnimation ) OVERRIDE;

	/**
	 * @return true if this viewport is in immersive mode, false otherwise
	 */
	bool IsImmersive () const OVERRIDE;
	
	/**
	 * Called to get the visibility of the viewport's 'Restore from Immersive' button. Returns EVisibility::Collapsed when not in immersive mode
	 */
	EVisibility GetCloseImmersiveButtonVisibility() const;
		
	/**
	 * Called to get the visibility of the viewport's maximize/minimize toggle button. Returns EVisibility::Collapsed when in immersive mode
	 */
	EVisibility GetMaximizeToggleVisibility() const;

	/**
	 * Called to get the visibility of the viewport's transform toolbar.
	 */
	EVisibility GetTransformToolbarVisibility() const;

	/**
	 * @return true if the active viewport is currently being used for play in editor
	 */
	bool IsPlayInEditorViewportActive() const;

	/**
	 * Called on all viewports, when actor selection changes.
	 * 
	 * @param NewSelection	List of objects that are now selected
	 */
	void OnActorSelectionChanged( const TArray<UObject*>& NewSelection );

	/**
	 * Called when game view should be toggled
	 */
	void ToggleGameView() OVERRIDE;

	/**
	 * @return true if we can toggle game view
	 */
	bool CanToggleGameView() const;

	/**
	 * @return true if we are in game view                   
	 */
	bool IsInGameView() const OVERRIDE;

	/**
	 * Toggles layer visibility in this viewport
	 *
	 * @param LayerID					Index of the layer
	 */
	void ToggleShowLayer( FName LayerName );

	/**
	 * Checks if a layer is visible in this viewport
	 *
	 * @param LayerID					Index of the layer
	 */
	bool IsLayerVisible( FName LayerName ) const;


	/** Called to lock/unlock the actor from the viewport's context menu */
	void OnActorLockToggleFromMenu(AActor* Actor);

	/**
	 * @return true if the actor is locked to the viewport
	 */
	bool IsActorLocked(const TWeakObjectPtr<AActor> Actor) const;

	/**
	 * @return true if an actor is locked to the viewport
	 */
	bool IsAnyActorLocked() const;

	/**
	 * @return true if the viewport is locked to selected actor
	 */
	bool IsSelectedActorLocked() const;

	/**
	 * @return the fixed width that a column returned by CreateActorLockSceneOutlinerColumn expects to be
	 */
	static float GetActorLockSceneOutlinerColumnWidth();

	/**
	 * @return a new custom column for a scene outliner that indicates whether each actor is locked to this viewport
	 */
	TSharedRef< class ISceneOutlinerColumn > CreateActorLockSceneOutlinerColumn( const TWeakPtr< class ISceneOutliner >& SceneOutliner ) const;
	
	/** Gets the world this viewport is for */
	UWorld* GetWorld() const;

	/** Called when Preview Selected Cameras preference is changed.*/
	void OnPreviewSelectedCamerasChange();

	/**
	 * Set the device profile name
	 *
	 * @param ProfileName The profile name to set
	 */
	void SetDeviceProfileString( const FString& ProfileName );

	/**
	 * @return true if the in profile name matches the set profile name
	 */
	bool IsDeviceProfileStringSet( FString ProfileName ) const;

	/**
	 * @return the name of the selected device profile
	 */
	FString GetDeviceProfileString( ) const;

	/** Get the parent level editor for this viewport */
	TWeakPtr<class SLevelEditor> GetParentLevelEditor() const { return ParentLevelEditor; }

	/** Called to get the level text */
	FText GetCurrentLevelText( bool bDrawOnlyLabel ) const;
	
	/** @return The visibility of the current level text display */
	EVisibility GetCurrentLevelTextVisibility() const;

	/**
	 * Sets the current layout on the parent tab that this viewport belongs to.
	 * 
	 * @param ConfigurationName		The name of the layout (for the names in namespace LevelViewportConfigurationNames)
	 */
	void OnSetViewportConfiguration(FName ConfigurationName);

	/**
	 * Returns whether the named layout is currently selected on the parent tab that this viewport belongs to.
	 *
	 * @param ConfigurationName		The name of the layout (for the names in namespace LevelViewportConfigurationNames)
	 * @return						True, if the named layout is currently active
	 */
	bool IsViewportConfigurationSet(FName ConfigurationName) const;

	/** For the specified actor, toggle Pinned/Unpinned of it's ActorPreview */
	void ToggleActorPreviewIsPinned(TWeakObjectPtr<AActor> PreviewActor);

	/** See if the specified actor's ActorPreview is pinned or not */
	bool IsActorPreviewPinned(TWeakObjectPtr<AActor> PreviewActor);

protected:
	/** SEditorViewport interface */
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() OVERRIDE;
	virtual TSharedPtr<SWidget> MakeViewportToolbar() OVERRIDE;

	virtual void OnIncrementPositionGridSize() OVERRIDE;
	virtual void OnDecrementPositionGridSize() OVERRIDE;
	virtual void OnIncrementRotationGridSize() OVERRIDE;
	virtual void OnDecrementRotationGridSize() OVERRIDE;
	virtual const FSlateBrush* OnGetViewportBorderBrush() const OVERRIDE;
	virtual FSlateColor OnGetViewportBorderColorAndOpacity() const OVERRIDE;
	virtual EVisibility OnGetViewportContentVisibility() const OVERRIDE;
	virtual void BindCommands() OVERRIDE;
private:
	/** Loads this viewport's config from the ini file */
	FLevelEditorViewportInstanceSettings LoadLegacyConfigFromIni(const FString& ConfigKey, const FLevelEditorViewportInstanceSettings& InDefaultSettings);

	/** Called when a property is changed */
	void HandleViewportSettingChanged(FName PropertyName);

	/**
	 * Called when the advanced settings should be opened.
	 */
	void OnAdvancedSettings();

	/**
	 * Called when immersive mode is toggled by the user
	 */
	void OnToggleImmersive();

	/**
	* Called to toggle maximize mode of current viewport
	*/
	void OnToggleMaximizeMode();

	/** Starts previewing any selected camera actors using live "PIP" sub-views */
	void PreviewSelectedCameraActors();

	/**
	 * Called when stat rendering should be enabled in the viewport
	 * NOTE: This enables realtime as well because we cant render stats without the viewport being realtime
	 */
	void OnToggleStats();

	/**
	 * @return true if stats are enabled in the viewport
	 */
	bool AreStatsEnabled() const;

	/**
	 * Called when FPS should be visible in the viewport                   
	 * NOTE: This enables realtime as well because we cant render fps without the viewport being realtime
	 */
	void OnToggleFPS();

	/**
	 * @return true if FPS is visible in the viewport                   
	 */
	bool IsShowingFPS() const;

	/**
	 * Called to create a cameraActor in the currently selected perspective viewport
	 */
	void OnCreateCameraActor();

	/**
	 * Called to bring up the screenshot UI
	 */
	void OnTakeHighResScreenshot();

	/**
	 * Called to check currently selected editor viewport is a perspective one
	 */
	bool IsPerspectiveViewport() const;
	
	/**
	 * Toggles a show flag in this viewport
	 *
	 * @param EngineShowFlagIndex	the ID to toggle
	 */
	void ToggleShowFlag( uint32 EngineShowFlagIndex );

	/**
	 * Checks if a show flag is enabled in this viewport
	 *
	 * @param EngineShowFlagIndex	the ID to check
	 * @return true if the show flag is enabled, false otherwise
	 */
	bool IsShowFlagEnabled( uint32 EngineShowFlagIndex ) const;

	/**
	 * Toggles all volume classes visibility
	 *
	 * @param Visible					true if volumes should be visible, false otherwise
	 */
	void OnToggleAllVolumeActors( int32 Visible );

	/**
	 * Toggles volume classes visibility
	 *
	 * @param VolumeID					Index of the volume class
	 */
	void ToggleShowVolumeClass( int32 VolumeID );

	/**
	 * Checks if volume class is visible in this viewport
	 *
	 * @param VolumeID					Index of the volume class
	 */
	bool IsVolumeVisible( int32 VolumeID ) const;

	/**
	 * Toggles all layers visibility
	 *
	 * @param Visible					true if layers should be visible, false otherwise
	 */
	void OnToggleAllLayers( int32 Visible );

	/**
	 * Toggles all sprite categories visibility
	 *
	 * @param Visible					true if sprites should be visible, false otherwise
	 */
	void OnToggleAllSpriteCategories( int32 Visible );

	/**
	 * Toggles sprite category visibility in this viewport
	 *
	 * @param CategoryID					Index of the category
	 */
	void ToggleSpriteCategory( int32 CategoryID );

	/**
	 * Checks if sprite category is visible in this viewport
	 *
	 * @param LayerID					Index of the category
	 */
	bool IsSpriteCategoryVisible( int32 CategoryID ) const;

	/**
	 * Called when show flags for this viewport should be reset to default
	 */
	void OnUseDefaultShowFlags();

	/**
	 * Changes the buffer visualization mode for this viewport
	 *
	 * @param InName	The ID of the required visualization mode
	 */
	void ChangeBufferVisualizationMode( FName InName );

	/**
	 * Checks if a buffer visualization mode is selected
	 * 
	 * @param InName	The ID of the required visualization mode
	 * @return	true if the supplied buffer visualization mode is checked
	 */
	bool IsBufferVisualizationModeSelected( FName InName ) const;

	/**
	 * Called to set a bookmark
	 *
	 * @param BookmarkIndex	The index of the bookmark to set
	 */
	void OnSetBookmark( int32 BookmarkIndex );

	/**
	 * Called to jump to a bookmark
	 *
	 * @param BookmarkIndex	The index of the bookmark to jump to
	 */
	void OnJumpToBookmark( int32 BookmarkIndex );

	/**
	 * Called to clear a bookmark
	 *
	 * @param BookmarkIndex The index of the bookmark to clear
	 */
	void OnClearBookMark( int32 BookmarkIndex );

	/**
	 * Called to clear all bookmarks
	 */
	void OnClearAllBookMarks();

	/**
	 * Called to toggle allowing matinee to use this viewport to preview in
	 */
	void OnToggleAllowMatineePreview();

	/**
	 * @return true if this viewport allows matinee to be previewed in it                   
	 */
	bool DoesAllowMatineePreview() const;

	/** Find currently selected actor in the level script.  */
	void FindSelectedInLevelScript();
	
	/** Can we find the currently selected actor in the level script. */
	bool CanFindSelectedInLevelScript() const;

	/** Called to clear the current actor lock */
	void OnActorUnlock();

	/**
	 * @return true if clearing the current actor lock is a valid input
	 */
	bool CanExecuteActorUnlock() const;

	/** Called to lock the viewport to the currently selected actor */
	void OnActorLockSelected();

	/**
	 * @return true if clearing the setting the actor lock to the selected actor is a valid input
	 */
	bool CanExecuteActorLockSelected() const;

	/**
	 * Called when the viewport should be redrawn
	 *
	 * @param bInvalidateHitProxies	Whether or not to invalidate hit proxies
	 */
	void RedrawViewport( bool bInvalidateHitProxies );

	/** An internal handler for dagging dropable objects into the viewport. */
	bool HandleDragObjects(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent);

	/** An internal handler for dropping objects into the viewport. 
	 *	@param DragDropEvent		The drag event.
	 *	@param bCreateDropPreview	If true, a drop preview actor will be spawned instead of a normal actor.
	 */
	bool HandlePlaceDraggedObjects(const FDragDropEvent& DragDropEvent, bool bCreateDropPreview);

	/** SWidget Interface */
	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent ) OVERRIDE;
	virtual void OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) OVERRIDE;
	virtual void OnDragLeave( const FDragDropEvent& DragDropEvent ) OVERRIDE;
	virtual FReply OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) OVERRIDE;
	virtual FReply OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) OVERRIDE;
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;
	/** End of SWidget interface */

	/**
	 * Bound event Triggered via FLevelViewportCommands::ApplyMaterialToActor, attempts to apply a material selected in the content browser
	 * to an actor being hovered over in the Editor viewport.
	 */ 
	void OnApplyMaterialToViewportTarget();

	/**
	 * Binds commands for our toolbar options menu
	 *
	 * @param CommandList	The list to bind commands to
	 */
	void BindOptionCommands( FUICommandList& CommandList );

	/**
	 * Binds commands for our toolbar view menu
	 *
	 * @param CommandList	The list to bind commands to
	 */
	void BindViewCommands( FUICommandList& CommandList );

	/**
	 * Binds commands for our toolbar show menu
	 *
	 * @param CommandList	The list to bind commands to
	 */
	void BindShowCommands( FUICommandList& CommandList );

	/**
	 * Binds commands for our drag-drop context menu
	 *
	 * @param CommandList	The list to bind commands to
	 */
	void BindDropCommands( FUICommandList& CommandList );

	/**
	 * Called to get the visibility of the level viewport toolbar
	 */
	EVisibility GetToolBarVisibility() const;
	/**
	 * Called to build the viewport context menu when the user is Drag Dropping from the Content Browser
	 */
	TSharedRef< SWidget > BuildViewportDragDropContextMenu();

	/**
	 * Called when a map is changed (loaded,saved,new map, etc)
	 */
	void OnMapChanged( UWorld* World, EMapChangeType::Type MapChangeType );

	/** Gets whether the locked icon should be shown in the viewport because it is locked to an actor */
	EVisibility GetLockedIconVisibility() const;

	/** Gets the locked icon tooltip text showing the meaning of the icon and the name of the locked actor */
	FText GetLockedIconToolTip() const;

	/**
	 * Starts previewing the specified actors.  If the supplied list of actors is empty, turns off the preview.
	 *
	 * @param	ActorsToPreview		List of actors to draw previews for
	 */
	void PreviewActors( const TArray< AActor* >& ActorsToPreview );

	/** Called every frame to update any actor preview viewports we may have */
	void UpdateActorPreviewViewports();

	/**
	 * Removes a specified actor preview from the list
	 *
	 * @param PreviewIndex Array index of the preview to remove.
	 */
	void RemoveActorPreview( int32 PreviewIndex );
	
	/** Adds a widget overlaid over the viewport */
	virtual void AddOverlayWidget(TSharedRef<SWidget> OverlaidWidget) OVERRIDE;

	/** Removes a widget that was previously overlaid on to this viewport */
	virtual void RemoveOverlayWidget(TSharedRef<SWidget> OverlaidWidget) OVERRIDE;

	/** Returns true if this viewport is the active viewport and can process UI commands */
	bool CanProduceActionForCommand(const TSharedRef<const FUICommandInfo>& Command) const;

	void TakeHighResScreenShot();

	/** Called when undo is executed */
	void OnUndo();

	/** Called when undo is executed */
	void OnRedo();

	/** @return Whether or not undo can be executed */
	bool CanExecuteUndo() const;

	/** @return Whether or not redo can be executed */
	bool CanExecuteRedo() const;

	/** @return The current color & opacity for the mouse capture label */
	FLinearColor GetMouseCaptureLabelColorAndOpacity() const;

	/** @return The current text for the mouse capture label */
	FText GetMouseCaptureLabelText() const;

	/** Show the mouse capture label with the specified vertical and horizontal alignment */
	void ShowMouseCaptureLabel(ELabelAnchorMode AnchorMode);

	/** Hide the mouse capture label */
	void HideMouseCaptureLabel();

private:
	/** Tab which this viewport is located in */
	TWeakPtr<class FLevelViewportLayout> ParentLayout;

	/** Pointer to the parent level editor for this viewport */
	TWeakPtr<class SLevelEditor> ParentLevelEditor;

	/** Viewport overlay widget exposed to game systems when running play-in-editor */
	TSharedPtr<SOverlay> PIEViewportOverlayWidget;

	/** Viewport horizontal box used internally for drawing actor previews on top of the level viewport */
	TSharedPtr<SHorizontalBox> ActorPreviewHorizontalBox;

	/** Active Slate viewport for rendering and I/O (Could be a pie viewport)*/
	TSharedPtr<class FSceneViewport> ActiveViewport;

	/**
	 * Inactive Slate viewport for rendering and I/O
	 * If this is valid there is a pie viewport and this is the previous level viewport scene viewport 
	 */
	TSharedPtr<class FSceneViewport> InactiveViewport;

	/**
	 * When in PIE this will contain the editor content for the viewport widget (toolbar). It was swapped
	 * out for GameUI content
	 */
	TSharedPtr<SWidget> InactiveViewportWidgetEditorContent;

	/** Level viewport client */
	TSharedPtr<FLevelEditorViewportClient> LevelViewportClient;

	/** The brush to use if this viewport is the active viewport */
	const FSlateBrush* ActiveBorder;
	/** The brush to use if this viewport is an inactive viewport or not showing a border */
	const FSlateBrush* NoBorder;
	/** The brush to use if this viewport is in debug mode */
	const FSlateBrush* DebuggingBorder;
	/** The brush to use for a black background */
	const FSlateBrush* BlackBackground;
	/** The brush to use when transitioning into Play in Editor mode */
	const FSlateBrush* StartingPlayInEditorBorder;
	/** The brush to use when transitioning into Simulate mode */
	const FSlateBrush* StartingSimulateBorder;
	/** The brush to use when returning back to the editor from PIE or SIE mode */
	const FSlateBrush* ReturningToEditorBorder;

	/** Array of objects dropped during the OnDrop event */
	TArray<UObject*> DroppedObjects;

	/** Caching off of the DragDropEvent Local Mouse Position grabbed from OnDrop */
	FVector2D CachedOnDropLocalMousePos;

	/** Weak pointer to the highres screenshot dialog if it's open. Will become invalid if UI is closed whilst the viewport is open */
	TWeakPtr<class SWindow> HighResScreenshotDialog;

	/** Pointer to the capture region widget in the viewport overlay. Enabled by the high res screenshot UI when capture region selection is required */
	TSharedPtr<class SCaptureRegionWidget> CaptureRegionWidget;

	/** Types of transition effects we support */
	struct EViewTransition
	{
		enum Type
		{
			/** No transition */
			None = 0,

			/** Fade in from black */
			FadingIn,

			/** Entering PIE */
			StartingPlayInEditor,

			/** Entering SIE */
			StartingSimulate,

			/** Leaving either PIE or SIE */
			ReturningToEditor
		};
	};

	/** Type of transition we're currently playing */
	EViewTransition::Type ViewTransitionType;

	/** Animation progress within current view transition */
	FCurveSequence ViewTransitionAnim;

	/** True if we want to kick off a transition animation but are waiting for the next tick to do so */
	bool bViewTransitionAnimPending;

	/** The current device profile string */
	FString DeviceProfile;

	/**
	 * Contains information about an actor being previewed within this viewport
	 */
	class FViewportActorPreview
	{

	public:

		FViewportActorPreview() 
			: bIsPinned(false) 
		{}

		void ToggleIsPinned()
		{
			bIsPinned = !bIsPinned;
		}

		/** The Actor that is the center of attention. */
		TWeakObjectPtr< AActor > Actor;

		/** Level viewport client for our preview viewport */
		TSharedPtr< FLevelEditorViewportClient > LevelViewportClient;

		/** The scene viewport */
		TSharedPtr< FSceneViewport > SceneViewport;

		/** Slate widget that represents this preview in the viewport */
		TSharedPtr< SWidget > PreviewWidget;

		/** Whether or not this actor preview will remain on screen if the actor is deselected */
		bool bIsPinned;		
	};

	/** List of actor preview objects */
	TArray< FViewportActorPreview > ActorPreviews;

	/** The slot index in the SOverlay for the PIE mouse control label */
	int32 PIEOverlaySlotIndex;

	/** Separate curve to control fading out the PIE mouse control label */
	FCurveSequence PIEOverlayAnim;

	/** Whether the PIE view has focus so we can track when to reshow the mouse control label */
	bool bPIEHasFocus;

protected:
	void LockActorInternal(AActor* NewActorToLock);

public:
	static bool GetCameraInformationFromActor(AActor* Actor, FMinimalViewInfo& out_CameraInfo);

	static bool CanGetCameraInformationFromActor(AActor* Actor);
};
