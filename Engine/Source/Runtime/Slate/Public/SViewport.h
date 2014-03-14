// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


class SLATE_API SViewport : public SCompoundWidget
{
public:
	
	SLATE_BEGIN_ARGS( SViewport )
		: _Content()
		, _ShowEffectWhenDisabled(true)
		, _RenderDirectlyToWindow(false)
		, _EnableGammaCorrection(true)
		, _EnableBlending(false)
		, _IgnoreTextureAlpha(true)
		{}

		SLATE_DEFAULT_SLOT( FArguments, Content )
		/** Whether or not to show the disabled effect when this viewport is disabled */
		SLATE_ATTRIBUTE( bool, ShowEffectWhenDisabled )
		/** 
		 * Whether or not to render directly to the window's backbuffer or an offscreen render target that is applied to the window later 
		 * Rendering to an offscreen target is the most common option in the editor where there may be many frames which this viewport's interface may wish to not re-render but use a cached buffer instead
		 * Rendering directly to the backbuffer is the most common option in the game where you want to update each frame without the cost of writing to an intermediate target first.
		 */
		SLATE_ARGUMENT( bool, RenderDirectlyToWindow )

		/**
		 * Whether or not to enable gamma correction.  Doesn't apply when rendering directly to a backbuffer 
		 */
		SLATE_ARGUMENT( bool, EnableGammaCorrection )

		/**
		 * Allow this viewport to blend with its background
		 */
		SLATE_ARGUMENT( bool, EnableBlending )

		/**
		 * If true, the viewport's texture alpha is ignored when performing blending.  In this case only the viewport tint opacity is used
		 * If false, the texture alpha is used during blending
		 */
		SLATE_ARGUMENT( bool, IgnoreTextureAlpha )
	SLATE_END_ARGS()

	/**
	 * Construct the widget.
	 *
	 * @param InArgs  Declaration from which to construct the widget.
	 */
	void Construct(const FArguments& InArgs);
	
	SViewport();

	/** SViewport wants keyboard focus */
	virtual bool SupportsKeyboardFocus() const OVERRIDE { return true; }

	/** 
	 * Computes the ideal size necessary to display this widget.
	 */
	virtual FVector2D ComputeDesiredSize() const OVERRIDE
	{
		return FVector2D( 320, 240 );
	}

	/**
	 * Sets the interface to be used by this viewport for rendering and I/O
	 *
	 * @param InViewportInterface	The interface to use
	 */
	void SetViewportInterface( TSharedRef<ISlateViewport> InViewportInterface )
	{
		ViewportInterface = InViewportInterface;
	}

	/**
	 * Sets the content for this widget
	 *
	 * @param InContent	The new content (can be null)
	 */
	void SetContent( TSharedPtr<SWidget> InContent )
	{
		ChildSlot
		[
			InContent.IsValid() ? InContent.ToSharedRef() : (TSharedRef<SWidget>)SNullWidget::NullWidget
		];
	}

	const TSharedPtr<SWidget> GetContent() const { return ChildSlot.Widget; }

	/**
	 * A delegate called when the viewports top level window is being closed
	 *
	 * @param InWindowBeingClosed	The window that is about to be closed
	 */
	void OnWindowClosed( const TSharedRef<SWindow>& InWindowBeingClosed );


	/** @return Whether or not this viewport renders directly to the backbuffer */
	bool ShouldRenderDirectly() const { return bRenderDirectlyToWindow; }

	/**
	 * Sets a widget that should become focused when this window is next activated
	 *
	 * @param	InWidget	The widget to set focus to when this window is activated
	 */
	void SetWidgetToFocusOnActivate( const TSharedPtr< SWidget >& InWidget )
	{
		
		WidgetToFocusOnActivate = InWidget;
	}

	/** Removes the widget to focus on activate so the viewport will be focused */
	void ClearWidgetToFocusOnActivate()
	{
		WidgetToFocusOnActivate.Reset();
	}

	// SWidget interface
	virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const OVERRIDE;
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;
	virtual FReply OnTouchStarted( const FGeometry& MyGeometry, const FPointerEvent& TouchEvent ) OVERRIDE;
	virtual FReply OnTouchMoved( const FGeometry& MyGeometry, const FPointerEvent& TouchEvent ) OVERRIDE;
	virtual FReply OnTouchEnded( const FGeometry& MyGeometry, const FPointerEvent& TouchEvent ) OVERRIDE;
	virtual FReply OnTouchGesture( const FGeometry& MyGeometry, const FPointerEvent& GestureEvent ) OVERRIDE;
	virtual FCursorReply OnCursorQuery( const FGeometry& MyGeometry, const FPointerEvent& CursorEvent ) const OVERRIDE;
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual FReply OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual void OnMouseEnter( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual void OnMouseLeave( const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual FReply OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual FReply OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual FReply OnMouseButtonDoubleClick( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& KeyboardEvent ) OVERRIDE;
	virtual FReply OnKeyUp( const FGeometry& MyGeometry, const FKeyboardEvent& KeyboardEvent ) OVERRIDE;
	virtual FReply OnKeyChar( const FGeometry& MyGeometry, const FCharacterEvent& CharacterEvent ) OVERRIDE;
	virtual FReply OnKeyboardFocusReceived( const FGeometry& MyGeometry, const FKeyboardFocusEvent& InKeyboardFocusEvent ) OVERRIDE;
	virtual void OnKeyboardFocusLost( const FKeyboardFocusEvent& InKeyboardFocusEvent ) OVERRIDE;
	virtual FReply OnControllerButtonPressed( const FGeometry& MyGeometry, const FControllerEvent& ControllerEvent ) OVERRIDE;
	virtual FReply OnControllerButtonReleased( const FGeometry& MyGeometry, const FControllerEvent& ControllerEvent ) OVERRIDE;
	virtual FReply OnControllerAnalogValueChanged( const FGeometry& MyGeometry, const FControllerEvent& ControllerEvent ) OVERRIDE;
	virtual FReply OnMotionDetected( const FGeometry& MyGeometry, const FMotionEvent& MotionEvent ) OVERRIDE;
	// End of SWidget interface


private:
	FSimpleSlot& Decl_GetContent() { return ChildSlot; }
	
	FPointerEvent MouseEventToTouchEvent(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const;

	bool ShouldThunkMouseEventsAsTouchEvents() const { return CurrentlyThunkingMouseEventsAsTouchEvents.Get(); }
private:
	/** Interface to the rendering and I/O implementation of the viewport */
	TWeakPtr<ISlateViewport> ViewportInterface;
	/** Whether or not to show the disabled effect when this viewport is disabled */
	TAttribute<bool> ShowDisabledEffect;
	/** Whether or not this viewport renders directly to the window backbuffer */
	bool bRenderDirectlyToWindow;
	/** Whether or not to apply gamma correction on the render target supplied by the ISlateViewport */
	bool bEnableGammaCorrection;
	/** Whether or not to blend this viewport with the background */
	bool bEnableBlending;
	/** Whether or not to allow texture alpha to be used in blending calculations */
	bool bIgnoreTextureAlpha;
	/** Whether or not to convert mouse movement with the LMB pressed into touch events */
	TAttribute<bool> CurrentlyThunkingMouseEventsAsTouchEvents;

	/** Widget to transfer keyboard focus to when this window becomes active, if any.  This is used to
	    restore focus to a widget after a popup has been dismissed. */
	TWeakPtr< SWidget > WidgetToFocusOnActivate;
};
