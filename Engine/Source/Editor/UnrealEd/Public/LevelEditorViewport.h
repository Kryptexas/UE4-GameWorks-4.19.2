// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once

// Forward declarations.

class IMatineeBase;


/** Describes an object that's currently hovered over in the level viewport */
struct FViewportHoverTarget
{
	/** The actor we're drawing the hover effect for, or NULL */
	AActor* HoveredActor;

	/** The BSP model we're drawing the hover effect for, or NULL */
	UModel* HoveredModel;

	/** Surface index on the BSP model that currently has a hover effect */
	uint32 ModelSurfaceIndex;


	/** Construct from an actor */
	FViewportHoverTarget( AActor* InActor )
		: HoveredActor( InActor ),
			HoveredModel( NULL ),
			ModelSurfaceIndex( INDEX_NONE )
	{
	}

	/** Construct from an BSP model and surface index */
	FViewportHoverTarget( UModel* InModel, int32 InSurfaceIndex )
		: HoveredActor( NULL ),
			HoveredModel( InModel ),
			ModelSurfaceIndex( InSurfaceIndex )
	{
	}

	/** Equality operator */
	bool operator==( const FViewportHoverTarget& RHS ) const
	{
		return RHS.HoveredActor == HoveredActor &&
				RHS.HoveredModel == HoveredModel &&
				RHS.ModelSurfaceIndex == ModelSurfaceIndex;
	}

	friend uint32 GetTypeHash( const FViewportHoverTarget& Key )
	{
		return Key.HoveredActor ? GetTypeHash(Key.HoveredActor) : GetTypeHash(Key.HoveredModel)+Key.ModelSurfaceIndex;
	}

};

struct FTrackingTransaction
{
	/** State of this transaction */
	struct ETransactionState
	{
		enum Enum
		{
			Inactive,
			Active,
			Pending,
		};
	};

	FTrackingTransaction();
	~FTrackingTransaction();

	/**
	 * Initiates a transaction.
	 */
	void Begin(const FText& Description);

	void End();

	void Cancel();

	/** Begin a pending transaction, which won't become a real transaction until PromotePendingToActive is called */
	void BeginPending(const FText& Description);

	/** Promote a pending transaction (if any) to an active transaction */
	void PromotePendingToActive();

	bool IsActive() const { return TrackingTransactionState == ETransactionState::Active; }

	bool IsPending() const { return TrackingTransactionState == ETransactionState::Pending; }

	int32 TransCount;

private:
	/** The current transaction. */
	class FScopedTransaction*	ScopedTransaction;

	/** This is set to Active if TrackingStarted() has initiated a transaction, Pending if a transaction will begin before the next delta change */
	ETransactionState::Enum TrackingTransactionState;

	/** The description to use if a pending transaction turns into a real transaction */
	FText PendingDescription;
};


/** */
class UNREALED_API FLevelEditorViewportClient : public FEditorViewportClient
{
public:

	/** @return Returns the current global drop preview actor, or a NULL pointer if we don't currently have one */
	static const TArray< TWeakObjectPtr<AActor> >& GetDropPreviewActors()
	{
		return DropPreviewActors;
	}

	FVector2D GetDropPreviewLocation() const { return FVector2D( DropPreviewMouseX, DropPreviewMouseY ); }

	/**
	 * Constructor
	 */
	FLevelEditorViewportClient();

	/**
	 * Destructor
	 */
	virtual ~FLevelEditorViewportClient();

	////////////////////////////
	// FViewElementDrawer interface
	virtual void Draw(const FSceneView* View,FPrimitiveDrawInterface* PDI) OVERRIDE;
	// End of FViewElementDrawer interface
	
	virtual FSceneView* CalcSceneView(FSceneViewFamily* ViewFamily) OVERRIDE;

	////////////////////////////
	// FEditorViewportClient interface
	virtual void DrawCanvas( FViewport& InViewport, FSceneView& View, FCanvas& Canvas ) OVERRIDE;
	virtual bool InputKey(FViewport* Viewport, int32 ControllerId, FKey Key, EInputEvent Event, float AmountDepressed = 1.f, bool bGamepad=false) OVERRIDE;
	virtual bool InputAxis(FViewport* Viewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime, int32 NumSamples=1, bool bGamepad=false) OVERRIDE;
	virtual void MouseEnter( FViewport* Viewport,int32 x, int32 y ) OVERRIDE;
	virtual void MouseLeave( FViewport* Viewport ) OVERRIDE;
	virtual void MouseMove(FViewport* Viewport,int32 x, int32 y) OVERRIDE;
	virtual EMouseCursor::Type GetCursor(FViewport* Viewport,int32 X,int32 Y) OVERRIDE;
	virtual void CapturedMouseMove( FViewport* InViewport, int32 InMouseX, int32 InMouseY ) OVERRIDE;
	virtual void Tick(float DeltaSeconds) OVERRIDE;
	virtual bool InputWidgetDelta( FViewport* Viewport, EAxisList::Type CurrentAxis, FVector& Drag, FRotator& Rot, FVector& Scale ) OVERRIDE;
	virtual TSharedPtr<FDragTool> MakeDragTool( EDragTool::Type DragToolType ) OVERRIDE;
	virtual bool IsLevelEditorClient() const { return ParentLevelEditor.IsValid(); }
	virtual void TrackingStarted( const struct FInputEventState& InInputState, bool bIsDraggingWidget, bool bNudge ) OVERRIDE;
	virtual void TrackingStopped() OVERRIDE;
	virtual void SetWidgetMode( FWidget::EWidgetMode NewMode ) OVERRIDE;
	virtual bool CanSetWidgetMode( FWidget::EWidgetMode NewMode ) const OVERRIDE;
	virtual void SetWidgetCoordSystemSpace( ECoordSystem NewCoordSystem ) OVERRIDE;
	virtual FWidget::EWidgetMode GetWidgetMode() const OVERRIDE;
	virtual FVector GetWidgetLocation() const OVERRIDE;
	virtual FMatrix GetWidgetCoordSystem() const;
	virtual ECoordSystem GetWidgetCoordSystemSpace() const;
	virtual void SetupViewForRendering( FSceneViewFamily& ViewFamily, FSceneView& View ) OVERRIDE;
	virtual FSceneInterface* GetScene() const OVERRIDE;
	virtual FLinearColor GetBackgroundColor() const OVERRIDE;
	virtual bool IsAspectRatioConstrained() const OVERRIDE;
	virtual float GetCameraSpeed(int32 SpeedSetting) const OVERRIDE;

	void SetIsCameraCut( bool bInIsCameraCut ) { bEditorCameraCut = bInIsCameraCut; }

	/**
	 * Reset the camera position and rotation.  Used when creating a new level.
	 */
	void ResetCamera();

	/**
	 * Reset the view for a new map 
	 */
	void ResetViewForNewMap();

	/**
	 * Stores camera settings that may be adversely affected by PIE, so that they may be restored later
	 */
	void PrepareCameraForPIE();

	/**
	 * Restores camera settings that may be adversely affected by PIE
	 */
	void RestoreCameraFromPIE();


	/** true to force realtime audio to be true, false to stop forcing it true */
	void SetForcedAudioRealtime( bool bShouldForceAudioRealtime )
	{
		bForceAudioRealtime = bShouldForceAudioRealtime;
	}

	/**
	 * Updates the audio listener for this viewport 
	 *
 	 * @param View	The scene view to use when calculate the listener position
	 */
	void UpdateAudioListener( const FSceneView& View );

	/** Determines if the new MoveCanvas movement should be used */
	bool ShouldUseMoveCanvasMovement (void);

	/**
	 * Determines if InComponent is inside of InSelBBox.  This check differs depending on the type of component.
	 * If InComponent is NULL, false is returned.
	 *
	 * @param	InActor							Used only when testing billboard components.
	 * @param	InComponent						The component to query.  If NULL, false is returned.
	 * @param	InSelBox						The selection box.
	 * @param	bConsiderOnlyBSP				If true, consider only BSP.
	 * @param	bMustEncompassEntireComponent	If true, the entire component must be encompassed by the selection box in order to return true.
	 */
	bool ComponentIsTouchingSelectionBox( AActor* InActor, UPrimitiveComponent* InComponent, const FBox& InSelBBox, bool bConsiderOnlyBSP, bool bMustEncompassEntireComponent );

	/** 
	 * Returns true if the passed in volume is visible in the viewport (due to volume actor visibility flags)
	 *
	 * @param VolumeActor	The volume to check
	 */
	bool IsVolumeVisibleInViewport( const AActor& VolumeActor ) const;

	/**
	 * Sets the actor that should "drive" this view's location and other settings
	 *
	 * @param	Actor	The actor that will push view settings to this view
	 */
	void SetControllingActor( const AActor* Actor )
	{
		ControllingActor = Actor;
	}

	/** Called every time the viewport is ticked to update the view based on an actor that is
	    driving the location, FOV and other settings for this view.  This is used for live camera PIP
		preview within editor viewports */
	void PushControllingActorDataToViewportClient();


	/**
	 * Allows to set the postprocess camera actor to unset it (0)
	 */
	void SetPostprocessCameraActor(ACameraActor* InPostprocessCameraActor);

	/**
	 * Returns the horizontal axis for this viewport.
	 */
	EAxisList::Type GetHorizAxis() const;

	/**
	 * Returns the vertical axis for this viewport.
	 */
	EAxisList::Type GetVertAxis() const;

	/** Returns true if the viewport is allowed to be possessed by Matinee for previewing sequences */
	bool AllowMatineePreview() const { return bAllowMatineePreview; }

	/** Sets whether or not this viewport is allowed to be possessed by Matinee */
	void SetAllowMatineePreview( const bool bInAllowMatineePreview )
	{
		bAllowMatineePreview = bInAllowMatineePreview;
	}

	void NudgeSelectedObjects( const struct FInputEventState& InputState );

	/**
	 * Moves the viewport camera according to the locked actors location and rotation
	 */
	void MoveCameraToLockedActor();

	/**
	 * Check to see if this actor is locked by the viewport
	 */
	bool IsActorLocked(const TWeakObjectPtr<AActor> InActor) const;

	/**
	 * Check to see if any actor is locked by the viewport
	 */
	bool IsAnyActorLocked() const;

	void ApplyDeltaToActors( const FVector& InDrag, const FRotator& InRot, const FVector& InScale );
	void ApplyDeltaToActor( AActor* InActor, const FVector& InDeltaDrag, const FRotator& InDeltaRot, const FVector& InDeltaScale );

	void ApplyDeltaToComponents( const FVector& InDrag, const FRotator& InRot, const FVector& InScale );
	void ApplyDeltaToComponent( USceneComponent* InComponent, const FVector& InDeltaDrag, const FRotator& InDeltaRot, const FVector& InDeltaScale );

	/** Updates the rotate widget with the passed in delta rotation. */
	void ApplyDeltaToRotateWidget( const FRotator& InRot );

	void SetIsSimulateInEditorViewport( bool bInIsSimulateInEditorViewport );

	/**
	 * Draws a screen space bounding box around the specified actor
	 *
	 * @param	InCanvas		Canvas to draw on
	 * @param	InView			View to render
	 * @param	InViewport		Viewport we're rendering into
	 * @param	InActor			Actor to draw a bounding box for
	 * @param	InColor			Color of bounding box
	 * @param	bInDrawBracket	True to draw a bracket, otherwise a box will be rendered
	 * @param	bInLabelText	Optional label text to draw
	 */
	void DrawActorScreenSpaceBoundingBox( FCanvas* InCanvas, const FSceneView* InView, FViewport* InViewport, AActor* InActor, const FLinearColor& InColor, const bool bInDrawBracket, const FString& InLabelText = TEXT( "" ) );

	/**
	 *	Draw the texture streaming bounds.
	 */
	void DrawTextureStreamingBounds(const FSceneView* View,FPrimitiveDrawInterface* PDI);

	/** GC references. */
	void AddReferencedObjects( FReferenceCollector& Collector );
	
	/**
	 * Copies layout and camera settings from the specified viewport
	 *
	 * @param InViewport The viewport to copy settings from
	 */
	void CopyLayoutFromViewport( const FLevelEditorViewportClient& InViewport );

	/**
	 * Returns whether the provided unlocalized sprite category is visible in the viewport or not
	 *
	 * @param	InSpriteCategory	Sprite category to get the index of
	 *
	 * @return	true if the specified category is visible in the viewport; false if it is not
	 */
	bool GetSpriteCategoryVisibility( const FName& InSpriteCategory ) const;

	/**
	 * Returns whether the sprite category specified by the provided index is visible in the viewport or not
	 *
	 * @param	Index	Index of the sprite category to check
	 *
	 * @return	true if the category specified by the index is visible in the viewport; false if it is not
	 */
	bool GetSpriteCategoryVisibility( int32 Index ) const;

	/**
	 * Sets the visibility of the provided unlocalized category to the provided value
	 *
	 * @param	InSpriteCategory	Sprite category to get the index of
	 * @param	bVisible			true if the category should be made visible, false if it should be hidden
	 */
	void SetSpriteCategoryVisibility( const FName& InSpriteCategory, bool bVisible );

	/**
	 * Sets the visibility of the category specified by the provided index to the provided value
	 *
	 * @param	Index		Index of the sprite category to set the visibility of
	 * @param	bVisible	true if the category should be made visible, false if it should be hidden
	 */
	void SetSpriteCategoryVisibility( int32 Index, bool bVisible );

	/**
	 * Sets the visibility of all sprite categories to the provided value
	 *
	 * @param	bVisible	true if all the categories should be made visible, false if they should be hidden
	 */
	void SetAllSpriteCategoryVisibility( bool bVisible );

	/** FEditorViewportClient Interface*/
	virtual void UpdateMouseDelta() OVERRIDE;
	virtual void ProcessClick(FSceneView& View, HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY);
	virtual void SetCurrentWidgetAxis( EAxisList::Type NewAxis ) OVERRIDE;
	virtual UWorld* GetWorld() const OVERRIDE;

	void SetReferenceToWorldContext(FWorldContext& WorldContext);

	void RemoveReferenceToWorldContext(FWorldContext& WorldContext);

	/** Returns true if a placement dragging actor exists */
	bool HasDropPreviewActors() const;

	/**
	 * If dragging an actor for placement, this function updates its position.
	 *
 	 * @param MouseX						The position of the mouse's X coordinate
	 * @param MouseY						The position of the mouse's Y coordinate
	 * @param DroppedObjects				The Objects that were used to create preview objects
	 * @param out_bDroppedObjectsVisible	Output, returns if preview objects are visible or not
	 *
	 * Returns true if preview actors were updated
	 */
	bool UpdateDropPreviewActors(int32 MouseX, int32 MouseY, const TArray<UObject*>& DroppedObjects, bool& out_bDroppedObjectsVisible, class UActorFactory* FactoryToUse = NULL);

	/**
	 * If dragging an actor for placement, this function destroys the actor.
	 */
	void DestroyDropPreviewActors();

	/**
	 * Checks the viewport to see if the given object can be dropped using the given mouse coordinates local to this viewport
	 *
 	 * @param MouseX			The position of the mouse's X coordinate
	 * @param MouseY			The position of the mouse's Y coordinate
	 * @param AssetInfo			Asset in question to be dropped
	 */
	bool CanDropObjectsAtCoordinates(int32 MouseX, int32 MouseY, const FAssetData& AssetInfo);

	/**
	 * Attempts to intelligently drop the given objects in the viewport, using the given mouse coordinates local to this viewport
	 *
 	 * @param MouseX			 The position of the mouse's X coordinate
	 * @param MouseY			 The position of the mouse's Y coordinate
	 * @param DroppedObjects	 The Objects to be placed into the editor via this viewport
	 * @param OutNewActors		 The new actor objects that were created
	 * @param bOnlyDropOnTarget  Flag that when True, will only attempt a drop on the actor targeted by the Mouse position. Defaults to false.
	 * @param bCreateDropPreview If true, a drop preview actor will be spawned instead of a normal actor.
	 */
	bool DropObjectsAtCoordinates(int32 MouseX, int32 MouseY, const TArray<UObject*>& DroppedObjects, TArray<AActor*>& OutNewActors, bool bOnlyDropOnTarget = false, bool bCreateDropPreview = false, bool SelectActor = true, class UActorFactory* FactoryToUse = NULL );

	/**
 	 * Sets GWorld to the appropriate world for this client
	 * 
	 * @return the previous GWorld
 	 */
	virtual UWorld* ConditionalSetWorld() OVERRIDE;

	/**
 	 * Restores GWorld to InWorld
	 *
	 * @param InWorld	The world to restore
 	 */
	virtual void ConditionalRestoreWorld( UWorld* InWorld  ) OVERRIDE;

	/**
	 *	Called to check if a material can be applied to an object, given the hit proxy
	 */
	bool CanApplyMaterialToHitProxy( const HHitProxy* HitProxy ) const;

	/**
	 * Static: Adds a hover effect to the specified object
	 *
	 * @param	InHoverTarget	The hoverable object to add the effect to
	 */
	static void AddHoverEffect( struct FViewportHoverTarget& InHoverTarget );

	/**
	 * Static: Removes a hover effect to the specified object
	 *
	 * @param	InHoverTarget	The hoverable object to remove the effect from
	 */
	static void RemoveHoverEffect( struct FViewportHoverTarget& InHoverTarget );

	/**
	 * Static: Clears viewport hover effects from any objects that currently have that
	 */
	static void ClearHoverFromObjects();


	/**
	 * Helper function for ApplyDeltaTo* functions - modifies scale based on grid settings.
	 * Currently public so it can be re-used in FEdModeBlueprint.
	 */
	void ModifyScale( USceneComponent* InComponent, FVector& ScaleDelta ) const;

	void RenderDragTool( const FSceneView* View,FCanvas* Canvas );

	/** 
	 * Gets the world space cursor info from the current mouse position
	 * 
	 * @param InViewportClient	The viewport client to check for mouse position and to set up the scene view.
	 * @return					An FViewportCursorLocation containing information about the mouse position in world space.
	 */
	FViewportCursorLocation GetCursorWorldLocationFromMousePos();
	
protected:
	/** 
	 * Checks the viewport to see if the given blueprint asset can be dropped on the viewport.
	 * @param AssetInfo		The blueprint Asset in question to be dropped
	 *
	 * @return true if asset can be dropped, false otherwise
	 */
	bool CanDropBlueprintAsset ( const struct FSelectedAssetInfo& );


	/** Called when editor cleanse event is triggered */
	void OnEditorCleanse();

	/** Callback for when an editor user setting has changed */
	void HandleViewportSettingChanged(FName PropertyName);

	/** Delegate handler for ActorMoved events */
	void OnActorMoved(AActor* InActor);

	/** FEditorViewportClient Interface*/
	virtual void UpdateLinkedOrthoViewports( bool bInvalidate = false ) OVERRIDE;
	virtual ELevelViewportType GetViewportType() const OVERRIDE;
	virtual void OverridePostProcessSettings( FSceneView& View ) OVERRIDE;
	virtual void PerspectiveCameraMoved() OVERRIDE;
	virtual bool ShouldLockPitch() const OVERRIDE;
	virtual void CheckHoveredHitProxy( HHitProxy* HoveredHitProxy ) OVERRIDE;

private:
	/**
	 * Checks to see the viewports locked actor need updating
	 */
	void UpdateLockedActorViewports(const AActor* InActor, const bool bCheckRealtime);
	void UpdateLockedActorViewport(const AActor* InActor, const bool bCheckRealtime);

	/**
	 * Moves the locked actor according to the viewport cameras location and rotation
	 */
	void MoveLockedActorToCamera() const;

	
	/** @return	Returns true if the delta tracker was used to modify any selected actors or BSP.  Must be called before EndTracking(). */
	bool HaveSelectedObjectsBeenChanged() const;

	/**
	 * Called when to attempt to apply an object to a BSP surface
	 *
	 * @param	ObjToUse			The object to attempt to apply
	 * @param	ModelHitProxy		The hitproxy of the BSP model whose surface the user is clicking on
	 * @param	Cursor				Mouse cursor location
	 *
	 * @return	true if the object was applied to the object
	 */
	bool AttemptApplyObjAsMaterialToSurface( UObject* ObjToUse, class HModel* ModelHitProxy, FViewportCursorLocation& Cursor );

	/**
	 * Called when an asset is dropped onto the blank area of a viewport.
	 *
	 * @param	Cursor				Mouse cursor location
	 * @param	DroppedObjects		Array of objects dropped into the viewport
	 * @param	ObjectFlags			The object flags to place on the actors that this function spawns.
	 * @param	OutNewActors		The list of actors created while dropping
	 *
	 * @return	true if the drop operation was successfully handled; false otherwise
	 */
	bool DropObjectsOnBackground( struct FViewportCursorLocation& Cursor, const TArray<UObject*>& DroppedObjects, EObjectFlags ObjectFlags, TArray<AActor*>& OutNewActors, bool SelectActor = true, class UActorFactory* FactoryToUse = NULL );

	/**
	* Called when an asset is dropped upon an existing actor.
	*
	* @param	Cursor				Mouse cursor location
	* @param	DroppedObjects		Array of objects dropped into the viewport
	* @param	TargetProxy			Hit proxy representing the dropped upon actor
	* @param	ObjectFlags			The object flags to place on the actors that this function spawns.
	* @param	OutNewActors		The list of actors created while dropping
	*
	* @return	true if the drop operation was successfully handled; false otherwise
	*/
	bool DropObjectsOnActor( struct FViewportCursorLocation& Cursor, const TArray<UObject*>& DroppedObjects, struct HActor* TargetProxy, EObjectFlags ObjectFlags, TArray<AActor*>& OutNewActors, bool SelectActor = true, class UActorFactory* FactoryToUse = NULL );

	/**
	 * Called when an asset is dropped upon a BSP surface.
	 *
	 * @param	View				The SceneView for the dropped-in viewport
	 * @param	Cursor				Mouse cursor location
	 * @param	DroppedObjects		Array of objects dropped into the viewport
	 * @param	TargetProxy			Hit proxy representing the dropped upon model
	 * @param	ObjectFlags			The object flags to place on the actors that this function spawns.
	 * @param	OutNewActors		The list of actors created while dropping
	 *
	 * @return	true if the drop operation was successfully handled; false otherwise
	 */
	bool DropObjectsOnBSPSurface( FSceneView* View, struct FViewportCursorLocation& Cursor, const TArray<UObject*>& DroppedObjects, class HModel* TargetProxy, EObjectFlags ObjectFlags, TArray<AActor*>& OutNewActors, bool SelectActor, UActorFactory* FactoryToUse );

	/**
	 * Called when an asset is dropped upon a manipulation widget.
	 *
	 * @param	View				The SceneView for the dropped-in viewport
	 * @param	Cursor				Mouse cursor location
	 * @param	DroppedObjects		Array of objects dropped into the viewport
	 * @param	TargetProxy			Hit proxy representing the dropped upon manipulation widget
	 *
	 * @return	true if the drop operation was successfully handled; false otherwise
	 */
	bool DropObjectsOnWidget( FSceneView* View, struct FViewportCursorLocation& Cursor, const TArray<UObject*>& DroppedObjects);

	

	/** Helper functions for ApplyDeltaTo* functions - modifies scale based on grid settings */
	void ModifyScale( AActor* InActor, FVector& ScaleDelta, bool bCheckSmallExtent = false ) const;
	void ValidateScale( const FVector& CurrentScale, const FVector& BoxExtent, FVector& ScaleDelta, bool bCheckSmallExtent = false ) const;


public:
	/** Static: List of objects we're hovering over */
	static TSet< FViewportHoverTarget > HoveredObjects;
		
	/** Parent level editor that owns this viewport.  Currently, this may be null if the parent doesn't happen to be a level editor. */
	TWeakPtr< class ILevelEditor > ParentLevelEditor;

	/** List of layers that are hidden in this view */
	TArray<FName>			ViewHiddenLayers;

	/** Special volume actor visibility settings. Each bit represents a visibility state for a specific volume class. 1 = visible, 0 = hidden */
	TBitArray<>				VolumeActorVisibility;

	/** When the viewpoint is locked to an actor this references the actor, invalid if not locked (replaces bLockSelectedToCamera) */
	TWeakObjectPtr<AActor>	ActorLockedToCamera;

	/** The viewport location that is restored when exiting PIE */
	FVector					LastEditorViewLocation;
	/** The viewport orientation that is restored when exiting PIE */
	FRotator				LastEditorViewRotation;

	FVector					ColorScale;

	FColor					FadeColor;

	
	float					FadeAmount;

	bool					bEnableFading;

	bool					bEnableColorScaling;

	/** If true then the pivot has been moved independantly of the actor and position updates should not occur when the actor is moved. */
	bool					bPivotMovedIndependantly;

	/** If true, we switched between two different cameras. Set by matinee, used by the motion blur to invalidate this frames motion vectors */
	bool					bEditorCameraCut;

	/** If true, draw vertices for selected BSP brushes and static meshes if the large vertices show flag is set. */
	bool					bDrawVertices;

	/** Indicates whether, of not, the base attachment volume should be drawn for this viewport. */
	bool bDrawBaseInfo;

	/**
	 * Used for actor drag duplication.  Set to true on Alt+LMB so that the selected
	 * actors will be duplicated as soon as the widget is displaced.
	 */
	bool					bDuplicateActorsOnNextDrag;

	/**
	* bDuplicateActorsOnNextDrag will not be set again while bDuplicateActorsInProgress is true.
	* The user needs to release ALT and all mouse buttons to clear bDuplicateActorsInProgress.
	*/
	bool					bDuplicateActorsInProgress;


	/**
	 * true when a brush is being transformed by its Widget
	 */
	bool					bIsTrackingBrushModification;
private:
	/** The actors that are currently being placed in the viewport via dragging */
	static TArray< TWeakObjectPtr< AActor > > DropPreviewActors;

	/** Optional actor that is 'controlling' this view.  That is, the view's location, rotation, FOV and other settings
	    should be pushed to this caInera every frame.  This is used by the editor's live viewport camera PIP feature */
	TWeakObjectPtr< AActor > ControllingActor;

	/** Bit array representing the visibility of every sprite category in the current viewport */
	TBitArray<>	SpriteCategoryVisibility;


	/** Valid if there is a camera available that can override the post process setting */
	TWeakObjectPtr<class ACameraActor> PostprocessCameraActor;

	UWorld* World;

	FTrackingTransaction TrackingTransaction;

	/** Represents the last known drop preview mouse position. */
	int32 DropPreviewMouseX;
	int32 DropPreviewMouseY;

	/** true if this window is allowed to be possessed by Matinee for previewing sequences in real-time */
	bool bAllowMatineePreview;

	/** If this view was controlled by another view this/last frame, don't update itself */
	bool bWasControlledByOtherViewport;
};
