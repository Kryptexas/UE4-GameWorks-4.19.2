// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SWidget;

/**
 * A Reply is something that a Slate event returns to the system to notify it about certain aspect of how an event was handled.
 * For example, a widget may handle an OnMouseDown event by asking the system to give mouse capture to a specific Widget.
 * To do this, return FReply::CaptureMouse( NewMouseCapture ).
 */
class FReply
{
	public:
		/** An event should return a FReply::Handled() to let the system know that an event was handled. */
		static FReply Handled()
		{
			FReply NewReply;
			NewReply.bIsHandled = true;
			return NewReply;
		}
		
		/** An event should return a FReply::Unhandled() to let the system know that an event was unhandled. */
		static FReply Unhandled()
		{
			FReply NewReply;
			NewReply.bIsHandled = false;
			return NewReply;
		}
		
		/** An event should return a FReply::Handled().CaptureMouse( SomeWidget ) as a means of asking the system to forward all mouse events to SomeWidget */
		FReply& CaptureMouse( TSharedRef<SWidget> InMouseCaptor )
		{
			this->MouseCaptor = InMouseCaptor;
			return *this;
		}

		FReply& CaptureJoystick( TSharedRef<SWidget> InJoystickCaptor, bool bInAllJoysticks = false )
		{
			this->JoystickCaptor = InJoystickCaptor;
			this->bAllJoysticks = bInAllJoysticks;
			return *this;
		}
		/**
		 * Enables the use of high precision (raw input) mouse movement, for more accurate mouse movement without mouse ballistics
		 * NOTE: This implies mouse capture and hidden mouse movement.  Releasing mouse capture deactivates this mode.
		 */
		FReply& UseHighPrecisionMouseMovement( TSharedRef<SWidget> InMouseCaptor )
		{
			this->MouseCaptor = InMouseCaptor;
			this->bUseHighPrecisionMouse = true;
			return *this;
		}

		/**
		 * An even should return FReply::Handled().SetMousePos to ask Slate to move the mouse cursor to a different location
		 */
		FReply& SetMousePos( const FIntPoint& NewMousePos )
		{
			this->RequestedMousePos = NewMousePos;
			return *this;
		}

		/** An event should return FReply::Handled().SetKeyboardFocus( SomeWidget ) as a means of asking the system to set keyboard focus to the provided widget*/
		FReply& SetKeyboardFocus( TSharedRef<SWidget> GiveMeFocus, EKeyboardFocusCause::Type ReasonFocusIsChanging )
		{
			this->FocusRecipient = GiveMeFocus;
			this->FocusChangeReason = ReasonFocusIsChanging;
			return *this;
		}
		
		/**
		 * An event should return FReply::Handled().LockMouseToWidget( SomeWidget ) as a means of asking the system to 
		 * Lock the mouse so it cannot move out of the bounds of the widget.
		 */
		FReply& LockMouseToWidget( TSharedRef<SWidget> InWidget )
		{
			this->MouseLockWidget = InWidget;
			this->bShouldReleaseMouseLock = false;
			return *this;
		}

		/** 
		 * An event should return a FReply::Handled().ReleaseMouseLock() to ask the system to release mouse lock on a widget
		 */
		FReply& ReleaseMouseLock()
		{
			this->bShouldReleaseMouseLock = true;
			MouseLockWidget.Reset();
			return *this;
		}

		/** 
		 * An event should return a FReply::Handled().ReleaseMouse() to ask the system to release mouse capture
		 * NOTE: Deactivates high precision mouse movement if activated.
		 */
		FReply& ReleaseMouseCapture()
		{
			this->bReleaseMouseCapture = true;
			this->bUseHighPrecisionMouse = false;
			return *this;		
		}

		/** 
		 * An event should return a FReply::Handled().ReleaseJoystickCapture() to ask the system to release joystick capture
		 */
		FReply& ReleaseJoystickCapture(bool bInAllJoysticks = false)
		{
			this->bReleaseJoystickCapture = true;
			this->bAllJoysticks = bInAllJoysticks;
			return *this;		
		}

		/**
		 * Ask Slate to detect if a user started dragging in this widget.
		 * If a drag is detected, Slate will send an OnDragDetected event.
		 *
		 * @param DetectDragInMe  Detect dragging in this widget
		 * @param MouseButton     This button should be pressed to detect the drag
		 */
		FReply& DetectDrag( const TSharedRef<SWidget>& DetectDragInMe, FKey MouseButton )
		{
			this->DetectDragForWidget = DetectDragInMe;
			this->DetectDragForMouseButton = MouseButton;
			return *this;
		}

		/**
		 * An event should return FReply::Handled().BeginDragDrop( Content ) to initiate a drag and drop operation.
		 *
		 * @param InDragDropContent  The content that is being dragged. This could be a widget, or some arbitrary data
		 *
		 * @return reference back to the FReply so that this call can be chained.
		 */
		FReply& BeginDragDrop(TSharedRef<FDragDropOperation> InDragDropContent)
		{
			this->DragDropContent = InDragDropContent;
			return *this;
		}

		/** An event should return FReply::Handled().EndDragDrop() to request that the current drag/drop operation be terminated. */
		FReply& EndDragDrop()
		{
			this->bEndDragDrop = true;
			return *this;
		}

		/** Set the widget that handled the event; undefined if never handled. This method is to be used by SlateApplication only! */
		FReply& SetHandler( const TSharedRef<SWidget>& InHandler )
		{
			this->EventHandler = InHandler;
			return *this;
		}
		
		/** Ensures throttling for Slate UI responsiveness is not done on mouse down */
		FReply& PreventThrottling()
		{
			this->bPreventThrottling = true;
			return *this;
		}

	public:
		/** True if this reply indicated that the event was handled */
		bool IsEventHandled() const { return bIsHandled; }
		/** The widget that ultimately handled the event */
		const TSharedPtr<SWidget> GetHandler() const { return EventHandler; }
		/** True if this reply indicated that we should release mouse capture as a result of the event being handled */
		bool ShouldReleaseMouse() const { return bReleaseMouseCapture; }
		/** true if this reply indicated that we should release joystick capture as a result of the event being handled */
		bool ShouldReleaseJoystick() const { return bReleaseJoystickCapture; }
		/** If the event replied with a request to capture or release the joystick whether it should do it for all joysticks or just the one tied to the UserIndex */
		bool AffectsAllJoysticks() const { return bAllJoysticks; }
		/** True if this reply indicated that we should use high precision mouse movement */
		bool ShouldUseHighPrecisionMouse() const { return bUseHighPrecisionMouse; }
		/** True if the reply indicated that we should release mouse lock */
		bool ShouldReleaseMouseLock() const { return bShouldReleaseMouseLock; }
		/** Whether or not we should throttle on mouse down */
		bool ShouldThrottle() const { return !bPreventThrottling; }
		/** Returns the widget that the mouse should be locked to (if any) */
		const TSharedPtr<SWidget>& GetMouseLockWidget() const { return MouseLockWidget; }
		/** When not NULL, keyboard focus has been requested to be set on the FocusRecipient. */
		const TSharedPtr<SWidget>& GetFocusRecepient() const { return FocusRecipient; }
		/** Get the reason that a focus change is being requested. */
		EKeyboardFocusCause::Type GetFocusCause() const { return FocusChangeReason; }
		/** If the event replied with a request to capture the mouse, this returns the desired mouse captor. Otherwise returns an invalid pointer. */
		const TSharedPtr<SWidget>& GetMouseCaptor() const { return MouseCaptor; }	
		/** If the event replied with a request to capture the joystick, this returns the desired joystick captor. Otherwise returns an invalid pointer. */
		const TSharedPtr<SWidget>& GetJoystickCaptor() const { return JoystickCaptor; }
		/** @return the Content that we should use for the Drag and Drop operation; Invalid SharedPtr if a drag and drop operation is not requested*/
		const TSharedPtr<FDragDropOperation>& GetDragDropContent() const { return DragDropContent; }
		/** @return true if the user has asked us to terminate the ongoing drag/drop operation */
		bool ShouldEndDragDrop() const { return bEndDragDrop; }
		/** @return a widget for which to detect a drag; Invalid SharedPtr if no drag detection requested */
		const TSharedPtr<SWidget>& GetDetectDragRequest() const { return DetectDragForWidget; }  
		/** @return the mouse button for which we are detecting a drag */
		FKey GetDetectDragRequestButton() const { return DetectDragForMouseButton; }
		/** @return The coordinates of the requested mouse position */
		const TOptional<FIntPoint>& GetRequestedMousePos() const { return RequestedMousePos; }
		
	private:
		FReply()
		: RequestedMousePos()
		, EventHandler( NULL )
		, MouseCaptor( NULL )
		, JoystickCaptor( NULL )
		, FocusRecipient( NULL )
		, MouseLockWidget( NULL )
		, DragDropContent( NULL )
		, FocusChangeReason( EKeyboardFocusCause::SetDirectly )
		, bIsHandled( false )
		, bReleaseMouseCapture( false )
		, bReleaseJoystickCapture( false )
		, bAllJoysticks( false )
		, bShouldReleaseMouseLock( false )
		, bUseHighPrecisionMouse( false )
		, bPreventThrottling( false )
		, bEndDragDrop( false )
		{
		}
		
		TOptional<FIntPoint> RequestedMousePos;
		TSharedPtr<SWidget> EventHandler;
		TSharedPtr<SWidget> MouseCaptor;
		TSharedPtr<SWidget> JoystickCaptor;
		TSharedPtr<SWidget> FocusRecipient;
		TSharedPtr<SWidget> MouseLockWidget;
		TSharedPtr<SWidget> DetectDragForWidget;
		FKey DetectDragForMouseButton;
		TSharedPtr<FDragDropOperation> DragDropContent;
		EKeyboardFocusCause::Type FocusChangeReason;
		uint32 bIsHandled:1;
		uint32 bReleaseMouseCapture:1;
		uint32 bReleaseJoystickCapture:1;
		uint32 bAllJoysticks:1;
		uint32 bShouldReleaseMouseLock:1;
		uint32 bUseHighPrecisionMouse:1;
		uint32 bPreventThrottling:1;
		uint32 bEndDragDrop:1;
		
		
};


