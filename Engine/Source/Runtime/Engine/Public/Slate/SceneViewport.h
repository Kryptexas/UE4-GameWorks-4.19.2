// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#ifndef __SLATE_VIEWPORT_H__
#define __SLATE_VIEWPORT_H__

/** Called in FSceneViewport::ResizeFrame after ResizeViewport*/
DECLARE_DELEGATE_OneParam( FOnSceneViewportResize, FVector2D );

/**
 * A viewport for use with Slate SViewport widgets.
 * This viewport does not render directly to the back buffer but instead renders to a separate render target which Slate uses when it draws
 */
class ENGINE_API FSceneViewport : public FViewportFrame, public FViewport, public ISlateViewport
{
public:
	FSceneViewport( FViewportClient* InViewportClient, TSharedPtr<SViewport> InViewportWidget );
	~FSceneViewport();

	virtual void* GetWindow() { return NULL; }

	/** FViewport interface */
	virtual void MoveWindow(int32 NewPosX, int32 NewPosY, int32 NewSizeX, int32 NewSizeY) {}
	virtual bool HasMouseCapture() const;
	virtual bool HasFocus() const;
	virtual bool IsForegroundWindow() const;
	virtual void CaptureMouse( bool bCapture );
	virtual void LockMouseToViewport( bool bLock );
	virtual void ShowCursor( bool bVisible );
	virtual void SetPreCaptureMousePosFromSlateCursor() OVERRIDE;
	virtual bool IsCursorVisible() const OVERRIDE { return bIsCursorVisible; }
	virtual void ShowSoftwareCursor( bool bVisible ) OVERRIDE { bIsSoftwareCursorVisible = bVisible; }
	virtual void SetSoftwareCursorPosition( FVector2D Position ) OVERRIDE { SoftwareCursorPosition = Position; }
	virtual bool IsSoftwareCursorVisible() const OVERRIDE { return bIsSoftwareCursorVisible; }
	virtual FVector2D GetSoftwareCursorPosition() const OVERRIDE
	{
		return SoftwareCursorPosition;
	}
	virtual FCanvas* GetDebugCanvas() OVERRIDE;

	/**
	 * Captures or uncaptures the joystick
	 *
	 * @param Capture	true if we should capture, false if we should uncapture
	 */
	virtual bool CaptureJoystickInput(bool Capture) OVERRIDE;

	/**
	 * Returns the state of the provided key. 
	 *
	 * @param Key	The name of the key to check
	 *
	 * @return true if the key is pressed, false otherwise
	 */
	virtual bool KeyState(FKey Key) const OVERRIDE;

	/**
	 * @return The current X position of the mouse (in local space, relative to the viewports geometry)                 
	 */
	virtual int32 GetMouseX() const OVERRIDE;

	/**
	 * @return The current Y position of the mouse (in local space, relative to the viewports geometry)                 
	 */
	virtual int32 GetMouseY() const OVERRIDE;

	/**
	 * Sets MousePosition to the current mouse position in local space
	 *
	 * @param MousePosition	Populated with the current mouse position     
	 */
	virtual void GetMousePos( FIntPoint& MousePosition ) OVERRIDE;

	/**
	 * Not implemented                   
	 */
	virtual void SetMouse( int32 X, int32 Y ) OVERRIDE;

	/**
	 * Additional input processing that happens every frame                   
	 */
	virtual void ProcessInput( float DeltaTime ) OVERRIDE;

	/**
	 * Called when the viewport should be invalidated and redrawn                   
	 */
	virtual void InvalidateDisplay() OVERRIDE;

	/**
	 * Invalidates the viewport's cached hit proxies at the end of the frame.
	 */
	virtual void DeferInvalidateHitProxy() OVERRIDE;

	/** FViewportFrame interface */
	virtual FViewport* GetViewport() OVERRIDE { return this; }
	virtual FViewportFrame* GetViewportFrame() OVERRIDE { return this; }

	/** @return The viewport widget being used */
	TWeakPtr<SViewport> GetViewportWidget() const { return ViewportWidget; }

	/** Called before BeginRenderFrame is enqueued */
	virtual void EnqueueBeginRenderFrame() OVERRIDE;

	/** Called when a frame starts to render */
	virtual void BeginRenderFrame() OVERRIDE;

	/** 
	 * Called when a frame is done rendering
	 *
	 * @param bPresent	Not used in Slate viewports
	 * @param bLockToVsync	Not used in Slate viewports
	 */
	virtual void EndRenderFrame( bool bPresent, bool bLockToVsync ) OVERRIDE;

	/**
	 * Ticks the viewport
	 */
	virtual void Tick( float DeltaTime );

	/**
	 * Performs a resize when in swapping viewports while viewing the play world.
	 *
	 * @param OtherViewport	The previously active viewport
	 */
	void OnPlayWorldViewportSwapped( const FSceneViewport& OtherViewport );

	/**
	 * Indicate that the viewport should be block for vsync.
	 */
	virtual void SetRequiresVsync(bool bShouldVsync) OVERRIDE { bRequiresVsync = bShouldVsync; }

	/**
	 * Returns true if the viewport should be vsynced.
	 */
	virtual bool RequiresVsync() const OVERRIDE { return bRequiresVsync; }

	/**
	 * Called to resize the actual window where this viewport resides
	 *
	 * @param NewSizeX		The new width of the viewport
	 * @param NewSizeY		The new height of the viewport
	 * @param bFullscreen	True if the viewport should be fullscreen
	 */
	virtual void ResizeFrame(uint32 NewSizeX,uint32 NewSizeY,bool bNewFullscreen,int32 InPosX, int32 InPosY ) OVERRIDE;

	/**
	 *	Sets the Viewport resize delegate.
	 */
	void SetOnSceneViewportResizeDel(FOnSceneViewportResize InOnSceneViewportResize) 
	{ 
		OnSceneViewportResizeDel = InOnSceneViewportResize; 
	}

	/** 
	* Sets whether a PIE viewport takes mouse control on startup.
	* @param bGetsMouseControl Takes control if true, or not if false. 
	*/
	void SetPlayInEditorGetsMouseControl( bool bGetsMouseControl )
	{
		bPlayInEditorGetsMouseControl = bGetsMouseControl;
	}

	/** Updates the viewport RHI with a new size and fullscreen flag */
	virtual void UpdateViewportRHI(bool bDestroyed,uint32 NewSizeX,uint32 NewSizeY,bool bNewIsFullscreen) OVERRIDE;

	/** ISlateViewport interface */
	virtual FSlateShaderResource* GetViewportRenderTargetTexture() const OVERRIDE;
	virtual void OnDrawViewport( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) OVERRIDE;
	virtual FCursorReply OnCursorQuery( const FGeometry& MyGeometry, const FPointerEvent& CursorEvent ) OVERRIDE;
	virtual FReply OnMouseButtonDown( const FGeometry& InGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual FReply OnMouseButtonUp( const FGeometry& InGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual void OnMouseEnter( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual void OnMouseLeave( const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual FReply OnMouseMove( const FGeometry& InGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual FReply OnMouseWheel( const FGeometry& InGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual FReply OnMouseButtonDoubleClick( const FGeometry& InGeometry, const FPointerEvent& InMouseEvent ) OVERRIDE;
	virtual FReply OnControllerButtonPressed( const FGeometry& MyGeometry, const FControllerEvent& ControllerEvent ) OVERRIDE;
	virtual FReply OnControllerButtonReleased( const FGeometry& MyGeometry, const FControllerEvent& ControllerEvent ) OVERRIDE;
	virtual FReply OnControllerAnalogValueChanged( const FGeometry& MyGeometry, const FControllerEvent& ControllerEvent ) OVERRIDE;
	virtual FReply OnTouchStarted( const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent ) OVERRIDE;
	virtual FReply OnTouchMoved( const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent ) OVERRIDE;
	virtual FReply OnTouchEnded( const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent ) OVERRIDE;
	virtual FReply OnTouchGesture( const FGeometry& MyGeometry, const FPointerEvent& InGestureEvent ) OVERRIDE;
	virtual FReply OnMotionDetected( const FGeometry& MyGeometry, const FMotionEvent& InMotionEvent ) OVERRIDE;
	virtual FReply OnKeyDown( const FGeometry& InGeometry, const FKeyboardEvent& InKeyboardEvent ) OVERRIDE;
	virtual FReply OnKeyUp( const FGeometry& InGeometry, const FKeyboardEvent& InKeyboardEvent ) OVERRIDE;
	virtual FReply OnKeyChar( const FGeometry& InGeometry, const FCharacterEvent& InCharacterEvent ) OVERRIDE;
	virtual FReply OnKeyboardFocusReceived( const FKeyboardFocusEvent& InKeyboardFocusEvent ) OVERRIDE;
	virtual void OnKeyboardFocusLost( const FKeyboardFocusEvent& InKeyboardFocusEvent ) OVERRIDE;
	virtual void OnViewportClosed() OVERRIDE;
	virtual FIntPoint GetSize() const OVERRIDE { return GetSizeXY(); }
	
private:
	/**
	 * Called when this viewport is destroyed
	 */
	void Destroy();

	// FRenderResource interface.
	virtual void InitDynamicRHI() OVERRIDE;
	virtual void ReleaseDynamicRHI() OVERRIDE;

	// @todo UE4 DLL: Without these functions we get unresolved linker errors with FRenderResource
	virtual void InitRHI() OVERRIDE {}
	virtual void ReleaseRHI() OVERRIDE {}
	virtual void InitResource() OVERRIDE { FViewport::InitResource(); }
	virtual void ReleaseResource() OVERRIDE { FViewport::ReleaseResource(); }
	virtual FString GetFriendlyName() const OVERRIDE { return FString(TEXT("FSlateSceneViewport"));}

	/**
	 * Called from Slate when the viewport should be resized
	 *
	 * @param NewSizeX		 The new width of the viewport
	 * @param NewSizeY		 The new height of the viewport
	 * @param bNewFullscreen True if the viewport should be fullscreen
	 */
	virtual void ResizeViewport( uint32 NewSizeX,uint32 NewSizeY,bool bNewFullscreen,int32 InPosX, int32 InPosY );


	/**
	 * Updates the cached mouse position from a mouse event
	 *
	 * @param InGeometry	The geometry of the viewport to convert to local space
	 * @param InMouseEvent	The mouse event containing the position of the mouse in absolute space
	 */
	void UpdateCachedMousePos( const FGeometry& InGeometry, const FPointerEvent& InMouseEvent );

	/**
	 * Updates the cached viewport geometry
	 *
	 * @param InGeometry	The geometry of the viewport to convert to local space
	 * @param InMouseEvent	The mouse event containing the position of the mouse in absolute space
	 */
	void UpdateCachedGeometry( const FGeometry& InGeometry );

	/**
	 * Updates the KeyStateMap via the modifier keys from a mouse event.
	 * This ensures that the key state is correct after focus changes.
	 *
	 * @param InMouseEvent	The mouse event containing the current state of modifier keys.
	 */
	void UpdateModifierKeys( const FPointerEvent& InMouseEvent );

	/**
	 * Calls InputKey on the ViewportClient via the modifier keys.
	 * This ensures that the key state is correct just prior to focus change
	 *
	 * @param InKeysState	The key state containing the states of the modifier keys
	 */
	void ApplyModifierKeys( const FModifierKeysState& InKeysState );

private:
	/** An intermediate reply state that is reset whenever an input event is generated */
	FReply CurrentReplyState;
	/** A mapping of key names to their pressed state */
	TMap<FKey,bool> KeyStateMap;
	/** The last known mouse position in local space */
	FIntPoint CachedMousePos;
	/** The last known geometry info */
	FGeometry CachedGeometry;
	/** Mouse position before the latest capture */
	FIntPoint PreCaptureMousePos;
	/**	The current position of the software cursor */
	FVector2D SoftwareCursorPosition;
	/**	Whether the software cursor should be drawn in the viewport */
	bool bIsSoftwareCursorVisible;
	/** The render target used by Slate to draw the viewport.  Can be null if this viewport renders directly to the backbuffer */
	class FSlateRenderTargetRHI* SlateRenderTargetHandle;
	/** Draws the debug canvas in Slate */
	TSharedPtr<class FDebugCanvasDrawer, ESPMode::ThreadSafe> DebugCanvasDrawer;
	/** The Slate viewport widget where this viewport is drawn */
	TWeakPtr<SViewport> ViewportWidget;
	/** The number of input samples in X since input was was last processed */
	int32 NumMouseSamplesX;
	/** The number of input samples in Y since input was was last processed */
	int32 NumMouseSamplesY;
	/** The current mouse delta */
	FIntPoint MouseDelta;
	/** true if the cursor is currently visible */
	bool bIsCursorVisible;
	/** true if this viewport requires vsync. */
	bool bRequiresVsync;
	/** true if this viewport renders to a separate render target.  false to render directly to the windows back buffer */
	bool bUseSeparateRenderTarget;
	/** Whether or not we are currently resizing */
	bool bIsResizing;
	/** Delegate that is fired off in ResizeFrame after ResizeViewport */
	FOnSceneViewportResize OnSceneViewportResizeDel;
	/** Whether a PIE viewport should take mouse control on startup */
	bool bPlayInEditorGetsMouseControl;
};


#endif
