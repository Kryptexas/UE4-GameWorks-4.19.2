// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FEditorCameraController;
class FCameraControllerUserImpulseData;
class FMouseDeltaTracker;
class FWidget;
class FCameraControllerConfig;
class FCachedJoystickState;
class FPreviewScene;

/** Delegate called by FEditorViewportClient to check its visibility */
DECLARE_DELEGATE_RetVal( bool, FViewportStateGetter );

DECLARE_LOG_CATEGORY_EXTERN(LogEditorViewport, Log, All);

namespace EDragTool
{
	enum Type
	{
		BoxSelect,
		FrustumSelect,
		Measure
	};
}

struct FInputEventState
{
public:
	FInputEventState( FViewport* InViewport, FKey InKey, EInputEvent InInputEvent )
		: Viewport( InViewport )
		, Key( InKey )
		, InputEvent( InInputEvent )
	{}

	FViewport* GetViewport() const { return Viewport; }
	EInputEvent GetInputEvent() const { return InputEvent; }
	FKey GetKey() const { return Key; }

	/** return true if the event causing button is a control key */
	bool IsCtrlButtonEvent() const { return (Key == EKeys::LeftControl || Key == EKeys::RightControl); }
	bool IsShiftButtonEvent() const { return (Key == EKeys::LeftShift || Key == EKeys::RightShift); }
	bool IsAltButtonEvent() const { return (Key == EKeys::LeftAlt || Key == EKeys::RightAlt); }

	bool IsLeftMouseButtonPressed() const { return IsButtonPressed( EKeys::LeftMouseButton ); }
	bool IsMiddleMouseButtonPressed() const { return IsButtonPressed( EKeys::MiddleMouseButton ); }
	bool IsRightMouseButtonPressed() const { return IsButtonPressed( EKeys::RightMouseButton ); }

	bool IsMouseButtonEvent() const { return (Key == EKeys::LeftMouseButton || Key == EKeys::MiddleMouseButton || Key == EKeys::RightMouseButton); } 
	bool IsButtonPressed( FKey InKey ) const { return Viewport->KeyState(InKey); }
	bool IsAnyMouseButtonDown() const { return IsButtonPressed(EKeys::LeftMouseButton) || IsButtonPressed(EKeys::MiddleMouseButton) || IsButtonPressed(EKeys::RightMouseButton); }

	/** return true if alt is pressed right now.  This will be true even if the event was for a different key but an alt key is currently pressed */
	bool IsAltButtonPressed() const { return !( IsAltButtonEvent() && InputEvent == IE_Released ) && ( IsButtonPressed( EKeys::LeftAlt ) || IsButtonPressed( EKeys::RightAlt ) ); }
	bool IsShiftButtonPressed() const { return !( IsShiftButtonEvent() && InputEvent == IE_Released ) && ( IsButtonPressed( EKeys::LeftShift ) || IsButtonPressed( EKeys::RightShift ) ); }
	bool IsCtrlButtonPressed() const { return !( IsCtrlButtonEvent() && InputEvent == IE_Released ) && ( IsButtonPressed( EKeys::LeftControl ) || IsButtonPressed( EKeys::RightControl ) ); }
	bool IsSpaceBarPressed() const { return IsButtonPressed( EKeys::SpaceBar ); }
private:
	/** Viewport the event was sent to */
	FViewport* Viewport;
	/** Pressed Key */
	FKey Key;
	/** Keyboard event */
	EInputEvent InputEvent;
};


/**
 * Contains information about a mouse cursor position within a viewport, transformed into the correct coordinate
 * system for the viewport.
 */
struct FViewportCursorLocation
{
public:
	UNREALED_API FViewportCursorLocation( const class FSceneView* View, class FEditorViewportClient* InViewportClient, int32 X, int32 Y );

	const FVector&		GetOrigin()			const	{ return Origin; }
	const FVector&		GetDirection()		const	{ return Direction; }
	const FIntPoint&	GetCursorPos()		const	{ return CursorPos; }
	ELevelViewportType	GetViewportType()	const;
	class FEditorViewportClient*	GetViewportClient()	const	{ return ViewportClient; }

private:
	FVector	Origin;
	FVector	Direction;
	FIntPoint CursorPos;
	class FEditorViewportClient* ViewportClient;
};

struct FViewportClick : public FViewportCursorLocation
{
public:
	UNREALED_API FViewportClick( const class FSceneView* View, class FEditorViewportClient* ViewportClient, FKey InKey, EInputEvent InEvent, int32 X, int32 Y );

	/** @return The 2D screenspace cursor position of the mouse when it was clicked. */
	const FIntPoint&	GetClickPos()	const	{ return GetCursorPos(); }
	const FKey&			GetKey()		const	{ return Key; }
	EInputEvent			GetEvent()		const	{ return Event; }

	virtual bool	IsControlDown()	const			{ return ControlDown; }
	virtual bool	IsShiftDown()	const			{ return ShiftDown; }
	virtual bool	IsAltDown()		const			{ return AltDown; }

private:
	FKey		Key;
	EInputEvent	Event;
	bool		ControlDown,
				ShiftDown,
				AltDown;
};


/** 
 * Stores the transformation data for the viewport camera
 */
struct UNREALED_API FViewportCameraTransform
{
public:
	FViewportCameraTransform();

	/** Sets the transform's location */
	void SetLocation( const FVector& Position );

	/** Sets the transform's rotation */
	void SetRotation( const FRotator& Rotation )
	{
		ViewRotation = Rotation;
	}

	/** Sets the location to look at during orbit */
	void SetLookAt( const FVector& InLookAt )
	{
		LookAt = InLookAt;
	}

	/** Set the ortho zoom amount */
	void SetOrthoZoom( float InOrthoZoom )
	{
		OrthoZoom = InOrthoZoom;
	}


	/** @return The transform's location */
	FORCEINLINE const FVector& GetLocation() const { return ViewLocation; }
	
	/** @return The transform's rotation */
	FORCEINLINE const FRotator& GetRotation() const { return ViewRotation; }

	/** @return The look at point for orbiting */
	FORCEINLINE const FVector& GetLookAt() const { return LookAt; }

	/** @return The ortho zoom amount */
	FORCEINLINE float GetOrthoZoom() const { return OrthoZoom; }

	/**
	 * Animates from the current location to the desired location
	 *
	 * @param InDesiredLocation	The location to transition to
	 * @param bInstant			If the desired location should be set instantly rather than transitioned to over time
	 */
	void TransitionToLocation( const FVector& InDesiredLocation, bool bInstant );

	/**
	 * Updates any current location transitions
	 *
	 * @return true if there is currently a transition
	 */
	bool UpdateTransition();

	/**
	 * Computes a matrix to use for viewport location and rotation when orbiting
	 */
	FMatrix ComputeOrbitMatrix() const;
private:
	/** Curve for animating between locations */
	TSharedPtr<struct FCurveSequence> TransitionCurve;
	/** Current viewport Position. */
	FVector	ViewLocation;
	/** Current Viewport orientation; valid only for perspective projections. */
	FRotator ViewRotation;
	/** Desired viewport location when animating between two locations */
	FVector	DesiredLocation;
	/** When orbiting, the point we are looking at */
	FVector LookAt;
	/** Viewport start location when animating to another location */
	FVector StartLocation;
	/** Ortho zoom amount */
	float OrthoZoom;
};

class UNREALED_API FEditorViewportClient : public FCommonViewportClient, public FViewElementDrawer, public FGCObject
{
public:
	friend class FMouseDeltaTracker;

	FEditorViewportClient(class FPreviewScene* InPreviewScene = NULL);
	virtual ~FEditorViewportClient();

	/**
	 * Toggles whether or not the viewport updates in realtime and returns the updated state.
	 *
	 * @return		The current state of the realtime flag.
	 */
	bool ToggleRealtime()
	{ 
		SetRealtime(!bIsRealtime);
		return bIsRealtime;
	}

	/** Sets whether or not the viewport updates in realtime. */
	void SetRealtime( bool bInRealtime, bool bStoreCurrentValue = false )
	{ 
		if( bStoreCurrentValue )
		{
			//Cache the realtime, showFPS and ShowStats flags
			bStoredRealtime = bIsRealtime;
			bStoredShowFPS = bShowFPS;
			bStoredShowStats = bShowStats;
		}
		
		bIsRealtime	= bInRealtime;

		if( !bIsRealtime )
		{
			SetShowFPS(false);
			SetShowStats(false);
		}
	}

	/** @return		True if viewport is in realtime mode, false otherwise. */
	bool IsRealtime() const				
	{ 
		return bIsRealtime; 
	}

	/**
	 * Restores realtime setting to stored value. This will only enable realtime and 
	 * never disable it (unless bAllowDisable is true)
	 */
	void RestoreRealtime( const bool bAllowDisable = false )
	{
		if( bAllowDisable )
		{
			bIsRealtime = bStoredRealtime;
			bShowFPS = bStoredShowFPS;
			bShowStats = bStoredShowStats;
		}
		else
		{
			bIsRealtime |= bStoredRealtime;
			bShowFPS |= bStoredShowFPS;
			bShowStats |= bStoredShowStats;
		}
	}


	// this set ups camera for both orbit and non orbit control
	void SetCameraSetup(const FVector & LocationForOrbiting, const FRotator & InOrbitRotation, const FVector & InOrbitZoom, const FVector & InOrbitLookAt, 
			const FVector & InViewLocation, const FRotator &InViewRotation );

	/** Callback for toggling the camera lock flag. */
	virtual void SetCameraLock();

	/** Callback for checking the camera lock flag. */
	bool IsCameraLocked() const;

	/** Callback for toggling the grid show flag. */
	void SetShowGrid();

	/** Callback for checking the grid show flag. */
	bool IsSetShowGridChecked() const;

	/** Sets the show bounds flag */
	void SetShowBounds(bool bShow);

	/** Callback for toggling the bounds show flag. */
	void ToggleShowBounds();

	/** Callback for checking the bounds show flag. */
	bool IsSetShowBoundsChecked() const;

	/** Callback for toggling the collision geometry show flag. */
	void SetShowCollision();

	/** Callback for checking the collision geometry show flag. */
	bool IsSetShowCollisionChecked() const;

	/** Callback for toggling the realtime preview flag. */
	void SetRealtimePreview();

	/** Sets the location of the viewport's camera */
	void SetViewLocation( const FVector& NewLocation )
	{
		ViewTransform.SetLocation( NewLocation );
	}

	/** Sets the location of the viewport's camera */
	void SetViewRotation( const FRotator& NewRotation )
	{
		ViewTransform.SetRotation( NewRotation );
	}

	/** 
	 * Sets the look at location of the viewports camera for orbit *
	 * 
	 * @param LookAt The new look at location
	 * @param bRecalulateView	If true, will recalculate view location and rotation to look at the new point immediatley
	 */
	void SetLookAtLocation( const FVector& LookAt, bool bRecalculateView = false )
	{
		ViewTransform.SetLookAt( LookAt );

		if( bRecalculateView )
		{
			FMatrix OrbitMatrix = ViewTransform.ComputeOrbitMatrix();
			OrbitMatrix = OrbitMatrix.Inverse();

			ViewTransform.SetRotation( OrbitMatrix.Rotator() );
			ViewTransform.SetLocation( OrbitMatrix.GetOrigin() );
		}
	}

	/** Sets ortho zoom amount */
	void SetOrthoZoom( float InOrthoZoom ) 
	{
		ViewTransform.SetOrthoZoom( InOrthoZoom );
	}

	/** @return the current viewport camera location */
	const FVector& GetViewLocation() const
	{
		return ViewTransform.GetLocation();
	}

	/** @return the current viewport camera rotation */
	const FRotator& GetViewRotation() const
	{
		return ViewTransform.GetRotation();
	}

	/** @return the current look at location */
	const FVector& GetLookAtLocation() const
	{
		return ViewTransform.GetLookAt();
	}

	/** @return the current ortho zoom amount */
	float GetOrthoZoom() const
	{
		return ViewTransform.GetOrthoZoom();
	}

	/** @return The number of units per pixel displayed in this viewport */
	float GetOrthoUnitsPerPixel(const FViewport* Viewport) const;

	void RemoveCameraRoll()
	{
		FRotator Rotation = GetViewRotation();
		Rotation.Roll = 0;
		SetViewRotation( Rotation );
	}

	void SetInitialViewTransform( const FVector& ViewLocation, const FRotator& ViewRotation, float InOrthoZoom );

	virtual void ProcessScreenShots(FViewport* Viewport) OVERRIDE;

	void TakeHighResScreenShot();

	/** FViewElementDrawer interface */
	virtual void Draw(const FSceneView* View,FPrimitiveDrawInterface* PDI) OVERRIDE;
	virtual void Draw(FViewport* Viewport,FCanvas* Canvas) OVERRIDE;

	/** FViewportClient interface */
	virtual void RedrawRequested(FViewport* Viewport) OVERRIDE;
	virtual void RequestInvalidateHitProxy(FViewport* Viewport) OVERRIDE;
	virtual bool InputKey(FViewport* Viewport, int32 ControllerId, FKey Key, EInputEvent Event, float AmountDepressed = 1.f, bool bGamepad=false) OVERRIDE;
	virtual bool InputAxis(FViewport* Viewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime, int32 NumSamples=1, bool bGamepad=false) OVERRIDE;
	virtual bool InputGesture(FViewport* Viewport, EGestureEvent::Type GestureType, const FVector2D& GestureDelta) OVERRIDE;
	virtual void ReceivedFocus(FViewport* Viewport) OVERRIDE;
	virtual void OnJoystickPlugged(const uint32 InControllerID, const uint32 InType, const uint32 bInConnected)  OVERRIDE;
	virtual void MouseMove(FViewport* Viewport,int32 x, int32 y) OVERRIDE;
	virtual EMouseCursor::Type GetCursor(FViewport* Viewport,int32 X,int32 Y) OVERRIDE;
	virtual void CapturedMouseMove( FViewport* InViewport, int32 InMouseX, int32 InMouseY ) OVERRIDE;
	virtual bool IsOrtho() const OVERRIDE;
	virtual void LostFocus(FViewport* Viewport) OVERRIDE;


	/** FGCObject interface */
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE;

	/**
	 * Called when the user clicks in the viewport
	 *
	 * @param View			The view of the scene in the viewport
	 * @param HitProxy		Any hit proxy that was clicked
	 * @param Key			The key that caused the click event
	 * @param Event			State of the pressed key
	 * @param HitX			The X location of the mouse
	 * @param HitY			The Y location of the mouse
	 */
	virtual void ProcessClick(class FSceneView& View, class HHitProxy* HitProxy, FKey Key, EInputEvent Event, uint32 HitX, uint32 HitY) {}
	
	/**
	 * Called when mouse movement tracking begins
	 *
	 * @param InInputState		The current mouse and keyboard input state
	 * @param bIsDraggingWidget	True if a widget is being dragged
	 * @param bNudge			True if we are tracking due to a nudge movement with the arrow keys
	 */
	virtual void TrackingStarted( const struct FInputEventState& InInputState, bool bIsDraggingWidget, bool bNudge ) {}
	
	/**
	 * Called when mouse movement tracking stops
	 */
	virtual void TrackingStopped() {}

	/**
	 * Called to give the viewport client a chance to handle widgets being moved
	 *
	 * @param InViewport	The viewport being rendered
	 * @param CurrentAxis	The current widget axis being moved
	 * @param Drag			The amount the widget was translated (the value depends on the coordinate system of the widget.  See GetWidgetCoordSystem )
	 * @param Rot			The amount the widget was rotated  (the value depends on the coordinate system of the widget.  See GetWidgetCoordSystem )
	 * @param Scale			The amount the widget was scaled (the value depends on the coordinate system of the widget.  See GetWidgetCoordSystem )
	 */
	virtual bool InputWidgetDelta( FViewport* InViewport, EAxisList::Type CurrentAxis, FVector& Drag, FRotator& Rot, FVector& Scale ) { return false; }

	/**
	 * Sets the current widget mode
	 */
	virtual void SetWidgetMode( FWidget::EWidgetMode NewMode ) {};

	/**
	 * Whether or not the new widget mode can be set in this viewport
	 */
	virtual bool CanSetWidgetMode( FWidget::EWidgetMode NewMode ) const { return true; }

	/**
	 * Whether or not the widget mode can be cycled
	 */
	virtual bool CanCycleWidgetMode() const { return true; }

	/**
	 * @return The current display mode for transform widget 
	 */
	virtual FWidget::EWidgetMode GetWidgetMode() const { return FWidget::WM_None; }

	/**
	 * @return The world space location of the transform widget
	 */
	virtual FVector GetWidgetLocation() const { return FVector::ZeroVector; }

	/**
	 * @return The current coordinate system for drawing and input of the transform widget.  
	 * For world coordiante system return the identity matrix
	 */
	virtual FMatrix GetWidgetCoordSystem() const { return FMatrix::Identity; }

	/**
	 * Sets the coordinate system space to use
	 */
	virtual void SetWidgetCoordSystemSpace( ECoordSystem NewCoordSystem ) {};

	/**
	 * @return The coordinate system space (world or local) to display the widget in
	 */
	virtual ECoordSystem GetWidgetCoordSystemSpace() const { return COORD_World; }

	/**
	 * Sets the current axis being manipulated by the transform widget
	 */
	virtual void SetCurrentWidgetAxis( EAxisList::Type InAxis );

	/**
	 * Called to do any additional set up of the view for rendering
	 * 
	 * @param ViewFamily	The view family being rendered
	 * @param View			The view being rendered
	 */
	virtual void SetupViewForRendering( FSceneViewFamily& ViewFamily, FSceneView& View );

	/**
	 * Called to draw onto the viewports 2D canvas
	 *
	 * @param InViewport	The viewport being rendered
	 * @param View			The view of the scene to be rendered
	 * @param Canvas		The canvas to draw on
	 */
	virtual void DrawCanvas( FViewport& InViewport, FSceneView& View, FCanvas& Canvas ) {};

	/**
	 * Configures the specified FSceneView object with the view and projection matrices for this viewport.
	 * @param	View		The view to be configured.  Must be valid.
	 * @return	A pointer to the view within the view family which represents the viewport's primary view.
	 */
	virtual FSceneView* CalcSceneView(FSceneViewFamily* ViewFamily);

	/** 
	 * @return The scene being rendered in this viewport
	 */
	virtual FSceneInterface* GetScene() const;

	/**
	 * @return The background color of the viewport 
	 */
	virtual FLinearColor GetBackgroundColor() const;

	/**
	 * Called to override any post process settings for the view
	 *
	 * @param View	The view to override post process settings on
	 */
	virtual void OverridePostProcessSettings( FSceneView& View ) {};

	/**
	 * Ticks this viewport client
	 */
	virtual void Tick(float DeltaSeconds);

	/**
	 * Called each frame to update the viewport based on delta mouse movements
	 */
	virtual void UpdateMouseDelta();
	
	/**
	 * Called each frame to update the viewport based on delta trackpad gestures
	 */
	virtual void UpdateGestureDelta();


	/** 
	* Use the viewports Scene to get a world. 
	* Will use Global world instance of the scene or its world is invalid.
	*
	* @return 		A valid pointer to the viewports world scene.
	*/
	virtual UWorld* GetWorld() const OVERRIDE;

	/** If true, this is a level editor viewport */
	virtual bool IsLevelEditorClient() const { return false; }
	
	/**
	 * Called to make a drag tool when the user starts dragging in the viewport
	 *
	 * @param DragToolType	The type of drag tool to make
	 * @return The new drag tool
	 */
	virtual TSharedPtr<class FDragTool> MakeDragTool( EDragTool::Type DragToolType );

	/** @return Whether or not to orbit the camera */
	virtual bool ShouldOrbitCamera() const ;

	bool IsMovingCamera() const;

	virtual void UpdateLinkedOrthoViewports( bool bInvalidate = false ) {};


	/**
	 * Called when the perpsective viewport camera moves
	 */
	virtual void PerspectiveCameraMoved() {}

	/**
	 * @return true to lock the pitch of the viewport camera
	 */
	virtual bool ShouldLockPitch() const;

	/**
	 * Called when the mouse cursor is hovered over a hit proxy
	 * 
	 * @param HoveredHitProxy	The hit proxy currently hovered over
	 */
	virtual void CheckHoveredHitProxy( HHitProxy* HoveredHitProxy );

	/** True if the window is maximized or floating */
	bool IsVisible() const;

	bool IsSimulateInEditorViewport() const { return bIsSimulateInEditorViewport; }

	/**
	 * Returns true if FPS information should be displayed over the viewport
	 *
	 * @return	true if frame rate should be displayed
	 */
	bool ShouldShowFPS() const
	{
		return bShowFPS;
	}


	/**
	 * Sets whether or not frame rate info is displayed over the viewport
	 *
	 * @param	bWantFPS	true if frame rate should be displayed
	 */
	void SetShowFPS( bool bWantFPS )
	{
		bShowFPS = bWantFPS;
	}

	/**
	 * Returns true if status information should be displayed over the viewport
	 *
	 * @return	true if stats should be displayed
	 */
	bool ShouldShowStats() const
	{
		return bShowStats;
	}

	/**
	 * Sets whether or not stats info is displayed over the viewport
	 *
	 * @param	bWantStats	true if stats should be displayed
	 */
	void SetShowStats( bool bWantStats )
	{
		bShowStats = bWantStats;
	}
	
	/**
	 * Sets how the viewport is displayed (lit, wireframe, etc) for the current viewport type
	 * 
	 * @param	InViewModeIndex				View mode to set for the current viewport type
	 */
	virtual void SetViewMode(EViewModeIndex InViewModeIndex);

	/**
	 * Sets how the viewport is displayed (lit, wireframe, etc)
	 * 
	 * @param	InPerspViewModeIndex		View mode to set when this viewport is of type LVT_Perspective
	 * @param	InOrthoViewModeIndex		View mode to set when this viewport is not of type LVT_Perspective
	 */
	void SetViewModes(const EViewModeIndex InPerspViewModeIndex, const EViewModeIndex InOrthoViewModeIndex);

	/**
	 * @return The current view mode in this viewport, for the current viewport type
	 */
	EViewModeIndex GetViewMode() const;

	/**
	 * @return The view mode to use when this viewport is of type LVT_Perspective
	 */
	EViewModeIndex GetPerspViewMode() const
	{
		return PerspViewModeIndex;
	}

	/**
	 * @return The view mode to use when this viewport is not of type LVT_Perspective
	 */
	EViewModeIndex GetOrthoViewMode() const
	{
		return OrthoViewModeIndex;
	}

	/** @return True if InViewModeIndex is the current view mode index */
	bool IsViewModeEnabled(EViewModeIndex InViewModeIndex) const
	{
		return GetViewMode() == InViewModeIndex;
	}

	/**
	 * Invalidates this viewport and optionally child views.
	 *
	 * @param	bInvalidateChildViews		[opt] If true (the default), invalidate views that see this viewport as their parent.
	 * @param	bInvalidateHitProxies		[opt] If true (the default), invalidates cached hit proxies too.
	 */
	void Invalidate(bool bInvalidateChildViews=true, bool bInvalidateHitProxies=true);
	
	/**
	 * Gets the dimensions of the viewport
	 */
	void GetViewportDimensions( FIntPoint& OutOrigin, FIntPoint& OutSize );
	
	/**
	 * Determines which axis InKey and InDelta most refer to and returns a corresponding FVector.  This
	 * vector represents the mouse movement translated into the viewports/widgets axis space.
	 *
	 * @param	InNudge		If 1, this delta is coming from a keyboard nudge and not the mouse
	 */
	FVector TranslateDelta( FKey InKey, float InDelta, bool InNudge );

	/**
	 * Returns the effective viewport type (taking into account any actor locking or camera possession)
	 */
	virtual ELevelViewportType GetViewportType() const;

	/**
	 * Set the viewport type of the client
	 *
	 * @param InViewportType	The viewport type to set the client to
	 */
	virtual void SetViewportType( ELevelViewportType InViewportType );
	
	/**
	 * @return If InViewportType is the active viewport type 
	 */
	bool IsActiveViewportType(ELevelViewportType InViewportType) const;

	/** Returns true if this viewport is perspective. */
	bool IsPerspective() const;

	/** Is the aspect ratio currently constrained? */
	virtual bool IsAspectRatioConstrained() const;

	/** 
	 * Focuses the viewport to the center of the bounding box ensuring that the entire box is in view 
	 *
	 * @param BoundingBox			The box to focus
	 * @param bUpdateLinkedOrthoViewports	Whether or not to updated linked viewports when this viewport changes
	 * @param bInstant			Whether or not to focus the viewport instantly or over time
	 */
	void FocusViewportOnBox( const FBox& BoundingBox, bool bInstant = false );

	FEditorCameraController* GetCameraController(void) { return CameraController; }

	void InputAxisForOrbit(FViewport* Viewport, const FVector& DragDelta, FVector& Drag, FRotator& Rot);

	/**
	 * Implements screenshot capture for editor viewports.  Should be called by derived class' InputKey.
	 */
	bool InputTakeScreenshot(FViewport* Viewport, FKey Key, EInputEvent Event);

	/**
	 * Opens the screenshot in the uses default bitmap viewer (determined by OS)
	 */
	void OpenScreenshot( FString SourceFilePath );

	/**
	 * Takes the screenshot capture, this is called by keybinded events as well InputTakeScreenshot
	 *
	 * @param Viewport				The viewport to take a screenshot of.
	 * @param bInvalidateViewport	Some methods already invalidate the viewport before calling this method.
	 */
	void TakeScreenshot(FViewport* Viewport, bool bInvalidateViewport);

	/**
	 * Converts a generic movement delta into drag/rotation deltas based on the viewport and keys held down.
	 */
	void ConvertMovementToDragRot( const FVector& InDelta, FVector& InDragDelta, FRotator& InRotDelta ) const;
	void ConvertMovementToOrbitDragRot(const FVector& InDelta, FVector& InDragDelta, FRotator& InRotDelta) const;

	
	/** Toggle between orbit camera and fly camera */
	void ToggleOrbitCamera( bool bEnableOrbitCamera );

	/**
	 * Sets the camera view location such that the LookAtPoint point is at the specified location.
	 */
	void SetViewLocationForOrbiting(const FVector& LookAtPoint, float DistanceToCamera = 256.f );

	/**
	 * Moves the viewport Scamera according to the specified drag and rotation.
	 */
	void MoveViewportCamera( const FVector& InDrag, const FRotator& InRot, bool bDollyCamera = false );

	
	// Utility functions to return the modifier key states
	bool IsAltPressed() const;
	bool IsCtrlPressed() const;
	bool IsShiftPressed() const;

	/** @return True if the window is in an immersive viewport */
	bool IsInImmersiveViewport() const;

	void ClearAudioFocus() { bHasAudioFocus = false; }
	
	void SetAudioFocus() { bHasAudioFocus = true; }

	void MarkMouseMovedSinceClick();

	/** Determines whether this viewport is currently allowed to use Absolute Movement */
	bool IsUsingAbsoluteTranslation() const;

	bool IsForcedRealtimeAudio() const { return bForceAudioRealtime; }

	/** @return true if a mouse button is down and it's movement being tracked for operations inside the viewport */
	bool IsTracking() const { return bIsTracking; }

	/**
	 * Allows custom disabling of camera recoil
	 */
	void SetMatineeRecordingWindow(class IMatineeBase* InInterpEd);

	/**
	 * Returns true if camera recoil is currently allowed
	 */
	bool IsMatineeRecordingWindow() const
	{
		return (RecordingInterpEd != NULL);
	}
	
	EAxisList::Type GetCurrentWidgetAxis() const;

	void SetRequiredCursorOverride( bool WantOverride, EMouseCursor::Type RequiredCursor = EMouseCursor::Default ); 

	/**
	 * Utility to get the actually mouse/camera speed (multiplier) based on the passed in setting.
	 *
	 * @param SpeedSetting	The desired speed steeing
	 */
	virtual float GetCameraSpeed(int32 SpeedSetting) const;

	void DrawBoundingBox(FBox &Box, FCanvas* InCanvas, const FSceneView* InView, const FViewport* InViewport, const FLinearColor& InColor, const bool bInDrawBracket, const FString &InLabelText) ;

	void SetGameView(bool bGameViewEnable);

	bool IsInGameView() const { return bInGameViewMode; }

	/**
	 * Overrides the aspect ratio bar display setting
	 */
	void OverrideSafeFrameDisplay( bool bEnableOverrides, bool bShowAspectRatioBars, bool bShowSafeFrameBox );
	bool IsOverridingAspectRatioBarDisplay() const { return bEnableSafeFrameOverride && bShowAspectRatioBarsOverride; }
	bool IsOverridingSafeFrameBoxDisplay() const { return bEnableSafeFrameOverride && bShowSafeFrameBoxOverride; }

	/** Get the near clipping plane for this viewport. */
	float GetNearClipPlane() const;

protected:
	/** Subclasses may override the near clipping plane. Set to a negative value to disable the override. */
	void OverrideNearClipPlane(float InNearPlane);

	/**
	 * Used to store the required cursor visibility states and override cursor appearance
	 */
	struct FRequiredCursorState
	{
		/** Should the software cursor be visible */
		bool	bSoftwareCursorVisible;

		/** Should the hardware be visible */
		bool	bHardwareCursorVisible;

		/** Should the software cursor position be reset to pre-drag */
		bool	bDontResetCursor;

		/** Should we override the cursor appearance with the value in RequiredCursor */
		bool	bOverrideAppearance;

		/** What the cursor should look like */
		EMouseCursor::Type RequiredCursor;
	};

	/**
	 * Updates the visibility of the hardware and software cursors according to the viewport's state.
	 */
	void UpdateAndApplyCursorVisibility();

	/** Setup the cursor visibility state we require and store in RequiredCursorVisibiltyAndAppearance struct */
	void UpdateRequiredCursorVisibility();
	
	/** 
	 * Apply the required cursor visibility states from the RequiredCursorVisibiltyAndAppearance struct 
	 * @param	View				True - Set the position of the software cursor if being made visible. This defaults to FALSE.
	*/
	void ApplyRequiredCursorVisibility(  bool bUpdateSoftwareCursorPostion = false );

	bool ShouldUseMoveCanvasMovement() const;


	/**
	 * Draws viewport axes
	 *
	 * @param	InViewport		Viewport we're rendering into
	 * @param	InCanvas		Canvas to draw on
	 * @param	InRotation		Specifies the rotation to apply to the axes
	 * @param	InAxis			Specifies which axes to draw
	 */
	void DrawAxes(FViewport* Viewport,FCanvas* Canvas, const FRotator* InRotation = NULL, EAxisList::Type InAxis = EAxisList::XYZ);

	/**
	 * Draws viewport scale units
	 *
	 * @param	InViewport		Viewport we're rendering into
	 * @param	InCanvas		Canvas to draw on
	 * @param	InView			Scene view used for rendering
	 */
	void DrawScaleUnits(FViewport* Viewport, FCanvas* Canvas, const FSceneView& InView);

	/**
	 * Starts tracking the mouse due to mouse input
	 *
	 * @param InputState	The mouse and keyboard input state at the time the mouse input happened
	 * @param View			The view of the scene in this viewport
	 */
	void StartTrackingDueToInput( const struct FInputEventState& InputState, FSceneView& View );

	/**
	 * Handles clicking in the viewport
	 *
	 * @param InputState	The mouse and keyboard input state at the time the click happened
	 * @param View			The view of the scene in this viewport
	 */
	void ProcessClickInViewport( const FInputEventState& InputState, FSceneView& View );

	/**
	 * Handles double clicking in the viewport
	 *
	 * @param InputState	The mouse and keyboard input state at the time the click happened
	 * @param View			The view of the scene in this viewport
	 */
	void ProcessDoubleClickInViewport( const struct FInputEventState& InputState, FSceneView& View );

	/**
	 * Called when a user zooms the ortho viewport
	 *
	 * @param InputState	The current state of mouse and keyboard buttons
	 * @param Scale			Allows modifying the default amount of zoom
	 */
	void OnOrthoZoom( const struct FInputEventState& InputState, float Scale = 1.0f );

	/**
	 * Called when a user dollys the perspective camera
	 *
	 * @param InputState	The current state of mouse and keyboard buttons
	 */
	void OnDollyPerspectiveCamera( const struct FInputEventState& InputState );

	/**
	 * Called when a user changes the camera speed
	 *
	 * @param InputState	The current state of mouse and keyboard buttons
	 */
	void OnChangeCameraSpeed( const struct FInputEventState& InputState );
	
	/**
	 * Returns whether or not the flight camera is active
	 *
	 * @return true if the flight camera is active
	 */
	bool IsFlightCameraActive() const;

	/**
	 * Stops any mouse tracking
	 */
	void StopTracking();

	/** Enables or disables camera lock **/
	void EnableCameraLock(bool bEnable);

	/**
	 * Gets a joystick state cache for the specified controller ID
	 */
	FCachedJoystickState* GetJoystickState(const uint32 InControllerID);
private:
	/** @return Whether or not the camera should be panned */
	bool ShouldPanCamera() const;

	void ConditionalCheckHoveredHitProxy();

	/** Returns true if perspective flight camera input mode is currently active in this viewport */
	bool IsFlightCameraInputModeActive() const;


	/** Moves a perspective camera */
	void MoveViewportPerspectiveCamera( const FVector& InDrag, const FRotator& InRot, bool bDollyCamera = false );

	
	/**Applies Joystick axis control to camera movement*/
	void UpdateCameraMovementFromJoystick(const bool bRelativeMovement, FCameraControllerConfig& InConfig);

	/**
	 * Updates real-time camera movement.  Should be called every viewport tick!
	 *
	 * @param	DeltaTime	Time interval in seconds since last update
	 */
	void UpdateCameraMovement( float DeltaTime );

	/**
	 * Forcibly disables lighting show flags if there are no lights in the scene, or restores lighting show
	 * flags if lights are added to the scene.
	 */
	void UpdateLightingShowFlags();

	/** InOut might get adjusted depending on viewmode or viewport type */
	void ApplyEditorViewModeAdjustments(FEngineShowFlags& InOut) const;

	/** Helper used by DrawSafeFrames to get the current safe frame aspect ratio. */
	bool GetActiveSafeFrame(float& OutAspectRatio) const;

	/** Renders the safe frame lines. */
	void DrawSafeFrames(FViewport& Viewport, FSceneView& View, FCanvas& Canvas);

	void DrawSafeFrameQuad( FCanvas &Canvas, FVector2D V1, FVector2D V2 );
	
	virtual FEngineShowFlags* GetEngineShowFlags() 
	{ 
		return &EngineShowFlags; 
	}

public:
	static const uint32 MaxCameraSpeeds;

	/** Delegate used to get whether or not this client is in an immersive viewport */
	FViewportStateGetter ImmersiveDelegate;

	/** Delegate used to get the visibility of this client from a slate viewport layout and tab configuration */
	FViewportStateGetter VisibilityDelegate; 

	FViewport*				Viewport;

	/** Viewport camera transform data */
	FViewportCameraTransform		ViewTransform;

	/** The viewport type. */
	ELevelViewportType		ViewportType;

	/** The viewport's scene view state. */
	FSceneViewStateReference ViewState;

	/** A set of flags that determines visibility for various scene elements. */
	FEngineShowFlags		EngineShowFlags;

	/** Previous value for engine show flags, used for toggling. */
	FEngineShowFlags		LastEngineShowFlags;

	/** Editor setting to allow designers to override the automatic expose */
	FExposureSettings		ExposureSettings;

	FName CurrentBufferVisualizationMode;

	/** The number of frames since this viewport was last drawn.  Only applies to linked orthographic movement. */
	int32 FramesSinceLastDraw;

	/** Index of this view in the editor's list of views */
	int32 ViewIndex;

	/** Viewport's current horizontal field of view (can be modified by locked cameras etc.) */
	float ViewFOV;
	/** Viewport's stored horizontal field of view (saved in ini files). */
	float FOVAngle;

	float AspectRatio;
	
	/** true if we've forced the SHOW_Lighting show flag off because there are no lights in the scene */
	bool bForcingUnlitForNewMap;

	/** true if the widget's axis is being controlled by an active mouse drag. */
	bool bWidgetAxisControlledByDrag;
	
	/** The number of pending viewport redraws. */
	bool bNeedsRedraw;

	bool bNeedsLinkedRedraw;

	/** If, following the next redraw, we should invalidate hit proxies on the viewport */
	bool bNeedsInvalidateHitProxy;
	
	/** True if the orbit camera is currently being used */
	bool bUsingOrbitCamera;
	

	/**
	 * true if all input is rejected from this viewport
	 */
	bool					bDisableInput;
	
	/** If true, draw the axis indicators when the viewport is perspective. */
	bool					bDrawAxes;

	/** If true, the listener position will be set */
	bool					bSetListenerPosition;
	
protected:

	FWidget*				Widget;

	FMouseDeltaTracker*		MouseDeltaTracker;
		
	/**InterpEd, should only be set if used for matinee recording*/
	class IMatineeBase* RecordingInterpEd;

	/** If true, the canvas has been been moved using bMoveCanvas Mode*/
	bool bHasMouseMovedSinceClick;

	/** Cursor visibility and appearance information */
	FRequiredCursorState RequiredCursorVisibiltyAndAppearance;

	/** Cached state of joystick axes and buttons*/
	TMap<int32, FCachedJoystickState*> JoystickStateMap;

	/** Camera controller object that's used for piloting the camera around */
	FEditorCameraController* CameraController;
	
	/** Current cached impulse state */
	FCameraControllerUserImpulseData* CameraUserImpulseData;

	/** When we have LOD locking, it's slow to force redraw of other viewports, so we delay invalidates to reduce the number of redraws */
	double TimeForForceRedraw;

	/** Extra camera speed scale for flight camera */
	float FlightCameraSpeedScale;

	bool bUseControllingActorViewInfo;
	FMinimalViewInfo ControllingActorViewInfo;

	uint32 LastMouseX;
	uint32 LastMouseY;

	/** Represents the last known mouse position. */
	uint32 CachedMouseX;
	uint32 CachedMouseY;

	/**
	 * true when within a FMouseDeltaTracker::StartTracking/EndTracking block.
	 */
	bool bIsTracking;

	/**
	 * true if the user is dragging by a widget handle.
	 */
	bool					bDraggingByHandle;

	/** Cumulative camera drag and rotation deltas from trackpad gesture in current Tick */
	FVector CurrentGestureDragDelta;
	FRotator CurrentGestureRotDelta;
	
	/** If true, force this viewport to use real time audio regardless of other settings */
	bool					bForceAudioRealtime;

	/** if the viewport is currently realtime */
	bool bIsRealtime;
	
	/** Cached realtime flag */
	bool bStoredRealtime;
	
	/** Cached show FPS flag */	
	bool bStoredShowFPS;

	/** Cached show statistics flag */	
	bool bStoredShowStats;

	/** True if we should draw FPS info over the viewport */
	bool bShowFPS;

	/** True if we should draw stats over the viewport */
	bool bShowStats;

	/** If true, this viewport gets to set audio parameters (e.g. set the listener) */
	bool bHasAudioFocus;
	
	/** true when we are in a state where we can check the hit proxy for hovering. */
	bool bShouldCheckHitProxy;

	/** True if this viewport uses a draw helper */
	bool bUsesDrawHelper;

	/** True if this level viewport can be used to view Simulate in Editor sessions */
	bool bIsSimulateInEditorViewport;

	/** Camera Lock or not **/
	bool bCameraLock;

	/** Viewport specific overrides for safe frame display */
	bool bEnableSafeFrameOverride;
	bool bShowAspectRatioBarsOverride;
	bool bShowSafeFrameBoxOverride;

	/** Draw helper for rendering common editor functionality like the grid */
	FEditorCommonDrawHelper DrawHelper;

	/** The scene used for the viewport. Owned externally */
	FPreviewScene* PreviewScene;

	/** default set up for toggling back **/
	FRotator DefaultOrbitRotation;
	FVector DefaultOrbitLocation;
	FVector DefaultOrbitZoom;
	FVector DefaultOrbitLookAt;
	
private:
	/* View mode to set when this viewport is of type LVT_Perspective */
	EViewModeIndex PerspViewModeIndex;

	/* View mode to set when this viewport is not of type LVT_Perspective */
	EViewModeIndex OrthoViewModeIndex;

	/** near plane adjustable for each editor view, if < 0 GNearClippingPlane should be used. */
	float NearPlane;

	/** If true, we are in Game View mode*/
	bool bInGameViewMode;
};