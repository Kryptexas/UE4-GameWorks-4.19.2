// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


#include "Core.h"
#include "PrimitiveTypes.h"
#include "DrawElements.h"
#include "Events.h"

class SLATE_API FSlateControlledConstruction
{
public:
	FSlateControlledConstruction(){}
	virtual ~FSlateControlledConstruction(){}
	
private:
	/** UI objects cannot be copy-constructed */
	FSlateControlledConstruction(const FSlateControlledConstruction& Other){}
	
	/** UI objects cannot be copied. */
	void operator= (const FSlateControlledConstruction& Other){}

	/** Widgets should only ever be constructed via SNew or SAssignNew */
	void* operator new (const size_t InSize )
	{
		return FMemory::Malloc(InSize);
	}

	template<class WidgetType, typename RequiredArgsPayloadType>
	friend struct TDecl;

public:
	void operator delete(void * mem)
	{
		FMemory::Free(mem);
	}
};

class SToolTip;


/**
 * STOP. DO NOT INHERIT DIRECTLY FROM WIDGET!
 *
 * Inheritance:
 *   Widget is not meant to be directly inherited.
 *   Instead consider inheriting from LeafWidget or Panel,
 *   which represent intended use cases and provide a
 *   succinct set of methods which to override
 *
 *   Widget is the base class for all interactive Slate entities.
 *   Widget's public interface describes everything that a Widget can do
 *   and is fairly complex as a result.
 *
 * 
 * Events:
 *   Events in Slate are implemented as virtual functions that the Slate system will call
 *   on a Widget in order to notify the Widget about an important occurrence (e.g. a key press)
 *   or querying the Widget regarding some information (e.g. what mouse cursor should be displayed).
 *
 *   Widget provides a default implementation for most events; the default implementation does nothing
 *   and does not handle the event.
 *
 *   Some events are able to reply to the system by returning an FReply, FCursorReply, or similar
 *   object. 
 */
class SLATE_API SWidget
	: public FSlateControlledConstruction,
	  public TSharedFromThis<SWidget>		// Enables 'this->AsShared()'
{
public:
	
	/**
	 * Construct a SWidget based on initial parameters.
	 */
	void Construct(
		const TAttribute<FString> & InToolTipText ,
		const TSharedPtr<SToolTip> & InToolTip ,
		const TAttribute< TOptional<EMouseCursor::Type> > & InCursor ,
		const TAttribute<bool> & InEnabledState ,
		const TAttribute<EVisibility> & InVisibility,
		const FName& InTag );

	//
	// GENERAL EVENTS
	//

	/**
	 * The widget should respond by populating the OutDrawElements array with FDrawElements 
	 * that represent it and any of its children.
	 *
	 * @param AllottedGeometry  The FGeometry that describes an area in which the widget should appear.
	 * @param MyClippingRect    The clipping rectangle allocated for this widget and its children.
	 * @param OutDrawElements   A list of FDrawElements to populate with the output.
	 * @param LayerId           The Layer onto which this widget should be rendered.
	 * @param InColorAndOpacity Color and Opacity to be applied to all the descendants of the widget being painted
 	 * @param bParentEnabled	True if the parent of this widget is enabled.
	 *
	 * @return The maximum layer ID attained by this widget or any of its children.
	 */
	virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const = 0;

	/**
	 * Ticks this widget.  Override in derived classes, but always call the parent implementation.
	 *
	 * @param  AllottedGeometry The space allotted for this widget
	 * @param  InCurrentTime  Current absolute real time
	 * @param  InDeltaTime  Real time passed since last tick
	 */
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime );

	/**
	 * Called when a widget is being hit tested and has passed the bounding box test to determine
	 * if the precise widget geometry is under the cursor.
	 *
	 * @param MyGeometry The Geometry of the widget receiving the event
	 * @param InAbsoluteCursorPosition	The absolute cursor position
	 *
	 * @return true if this widget considers itself hit by the passed in cursor position.
	 */
	virtual bool OnHitTest( const FGeometry& MyGeometry, FVector2D InAbsoluteCursorPosition );

	//
	// KEYBOARD INPUT
	//

	/**
	 * Called when keyboard focus is given to this widget.  This event does not bubble.
	 *
	 * @param MyGeometry The Geometry of the widget receiving the event
	 * @param  InKeyboardFocusEvent  KeyboardFocusEvent
	 *
	 * @return  Returns whether the event was handled, along with other possible actions
	 */
	virtual FReply OnKeyboardFocusReceived( const FGeometry& MyGeometry, const FKeyboardFocusEvent& InKeyboardFocusEvent );

	/**
	 * Called when this widget loses the keyboard focus.  This event does not bubble.
	 *
	 * @param  InKeyboardFocusEvent  KeyboardFocusEvent
	 */
	virtual void OnKeyboardFocusLost( const FKeyboardFocusEvent& InKeyboardFocusEvent );

	/** Called whenever a focus path is changing on all the widgets within the old and new focus paths */
	virtual void OnKeyboardFocusChanging( const FWeakWidgetPath& PreviousFocusPath, const FWidgetPath& NewWidgetPath );

	/**
	 * Called after a character is entered while this widget has keyboard focus
	 *
	 * @param MyGeometry The Geometry of the widget receiving the event
	 * @param  InCharacterEvent  Character event
	 *
	 * @return  Returns whether the event was handled, along with other possible actions
	 */
	virtual FReply OnKeyChar( const FGeometry& MyGeometry,const FCharacterEvent& InCharacterEvent );

	/**
	 * Called after a key is pressed when this widget or a child of this widget has keyboard focus
	 * If a widget handles this event, OnKeyDown will *not* be passed to the focused widget.
	 *
	 * This event is primarily to allow parent widgets to consume an event before a child widget processes
	 * it and it should be used only when there is no better design alternative.
	 *
	 * @param MyGeometry The Geometry of the widget receiving the event
	 * @param  InKeyboardEvent  Keyboard event
	 *
	 * @return  Returns whether the event was handled, along with other possible actions
	 */
	virtual FReply OnPreviewKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent );

	/**
	 * Called after a key is pressed when this widget has keyboard focus (this event bubbles if not handled)
	 *
	 * @param MyGeometry The Geometry of the widget receiving the event
	 * @param  InKeyboardEvent  Keyboard event
	 *
	 * @return  Returns whether the event was handled, along with other possible actions
	 */
	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent );

	/**
	 * Called after a key is released when this widget has keyboard focus
	 *
	 * @param MyGeometry The Geometry of the widget receiving the event
	 * @param  InKeyboardEvent  Keyboard event
	 *
	 * @return  Returns whether the event was handled, along with other possible actions
	 */
	virtual FReply OnKeyUp( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent );

	//
	// MOUSE INPUT
	//

	/**
	 * The system calls this method to notify the widget that a mouse button was pressed within it. This event is bubbled.
	 *
	 * @param MyGeometry The Geometry of the widget receiving the event
	 * @param MouseEvent Information about the input event
	 *
	 * @return Whether the event was handled along with possible requests for the system to take action.
	 */
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent );

	/**
	 * Just like OnMouseButtonDown, but tunnels instead of bubbling.
	 * If this even is handled, OnMouseButtonDown will not be sent.
	 * 
	 * Use this event sparingly as preview events generally make UIs more
	 * difficult to reason about.
	 */
	virtual FReply OnPreviewMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent );
	
	/**
	 * The system calls this method to notify the widget that a mouse button was release within it. This event is bubbled.
	 *
	 * @param MyGeometry The Geometry of the widget receiving the event
	 * @param MouseEvent Information about the input event
	 *
	 * @return Whether the event was handled along with possible requests for the system to take action.
	 */
	virtual FReply OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent );
	
	/**
	 * The system calls this method to notify the widget that a mouse moved within it. This event is bubbled.
	 *
	 * @param MyGeometry The Geometry of the widget receiving the event
	 * @param MouseEvent Information about the input event
	 *
	 * @return Whether the event was handled along with possible requests for the system to take action.
	 */
	virtual FReply OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent );
	
	/**
	 * The system will use this event to notify a widget that the cursor has entered it. This event is NOT bubbled.
	 *
	 * @param MyGeometry The Geometry of the widget receiving the event
	 * @param MouseEvent Information about the input event
	 */
	virtual void OnMouseEnter( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent );
	
	/**
	 * The system will use this event to notify a widget that the cursor has left it. This event is NOT bubbled.
	 *
	 * @param MouseEvent Information about the input event
	 */
	virtual void OnMouseLeave( const FPointerEvent& MouseEvent );
	
	/**
	 * Called when the mouse wheel is spun. This event is bubbled.
	 *
	 * @param  MouseEvent  Mouse event
	 *
	 * @return  Returns whether the event was handled, along with other possible actions
	 */
	virtual FReply OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent );
	
	/**
	 * The system asks each widget under the mouse to provide a cursor. This event is bubbled.
	 * 
	 * @return FCursorReply::Unhandled() if the event is not handled; return FCursorReply::Cursor() otherwise.
	 */
	virtual FCursorReply OnCursorQuery( const FGeometry& MyGeometry, const FPointerEvent& CursorEvent ) const;

	/**
	 * Called when a mouse button is double clicked.  Override this in derived classes.
	 *
	 * @param  InMyGeometry  Widget geometry
	 * @param  InMouseEvent  Mouse button event
	 *
	 * @return  Returns whether the event was handled, along with other possible actions
	 */
	virtual FReply OnMouseButtonDoubleClick( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent );

	/**
	 * Called when Slate wants to visualize tooltip.
	 * If nobody handles this event, Slate will use default tooltip visualization.
	 * If you override this event, you should probably return true.
	 * 
	 * @param  TooltipContent    The TooltipContent that I may want to visualize.
	 *
	 * @return true if this widget visualized the tooltip content; i.e., the event is handled.
	 */
	virtual bool OnVisualizeTooltip( const TSharedPtr<SWidget>& TooltipContent );

	/**
	 * Called when Slate detects that a widget started to be dragged.
	 * Usage:
	 * A widget can ask Slate to detect a drag.
	 * OnMouseDown() reply with FReply::Handled().DetectDrag( SharedThis(this) ).
	 * Slate will either send an OnDragDetected() event or do nothing.
	 * If the user releases a mouse button or leaves the widget before
	 * a drag is triggered (maybe user started at the very edge) then no event will be
	 * sent.
	 *
	 * @param  InMyGeometry  Widget geometry
	 * @param  InMouseEvent  MouseMove that triggered the drag
	 * 
	 */
	virtual FReply OnDragDetected( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent );

	//
	// DRAG AND DROP (DragDrop)
	//


	/**
	 * Called during drag and drop when the drag enters a widget.
	 *
	 * Enter/Leave events in slate are meant as lightweight notifications.
	 * So we do not want to capture mouse or set focus in response to these.
	 * However, OnDragEnter must also support external APIs (e.g. OLE Drag/Drop)
	 * Those require that we let them know whether we can handle the content
	 * being dragged OnDragEnter.
	 *
	 * The concession is to return a can_handled/cannot_handle
	 * boolean rather than a full FReply.
	 *
	 * @param MyGeometry      The geometry of the widget receiving the event.
	 * @param DragDropEvent   The drag and drop event.
	 *
	 * @return A reply that indicated whether the contents of the DragDropEvent can potentially be processed by this widget.
	 */
	virtual void OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent );
	
	/**
	 * Called during drag and drop when the drag leaves a widget.
	 *
	 * @param DragDropEvent   The drag and drop event.
	 */
	virtual void OnDragLeave( const FDragDropEvent& DragDropEvent );

	/**
	 * Called during drag and drop when the the mouse is being dragged over a widget.
	 *
	 * @param MyGeometry      The geometry of the widget receiving the event.
	 * @param DragDropEvent   The drag and drop event.
	 *
	 * @return A reply that indicated whether this event was handled.
	 */
	virtual FReply OnDragOver( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent );
	
	/**
	 * Called when the user is dropping something onto a widget; terminates drag and drop.
	 *
	 * @param MyGeometry      The geometry of the widget receiving the event.
	 * @param DragDropEvent   The drag and drop event.
	 *
	 * @return A reply that indicated whether this event was handled.
	 */
	virtual FReply OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent );
	
	/**
	 * Called when a controller button is pressed
	 * 
	 * @param ControllerEvent	The controller event generated
	 */
	virtual FReply OnControllerButtonPressed( const FGeometry& MyGeometry, const FControllerEvent& ControllerEvent );

	/**
	 * Called when a controller button is released
	 * 
	 * @param ControllerEvent	The controller event generated
	 */
	virtual FReply OnControllerButtonReleased( const FGeometry& MyGeometry, const FControllerEvent& ControllerEvent );

	/**
	 * Called when an analog value on a controller changes
	 * 
	 * @param ControllerEvent	The controller event generated
	 */
	virtual FReply OnControllerAnalogValueChanged( const FGeometry& MyGeometry, const FControllerEvent& ControllerEvent );

	/**
	 * Called when the user performs a gesture on trackpad. This event is bubbled.
	 *
	 * @param  GestureEvent  gesture event
	 *
	 * @return  Returns whether the event was handled, along with other possible actions
	 */
	virtual FReply OnTouchGesture( const FGeometry& MyGeometry, const FPointerEvent& GestureEvent );
	
	/**
	 * Called when a touchpad touch is started (finger down)
	 * 
	 * @param TouchEvent	The touch event generated
	 */
	virtual FReply OnTouchStarted( const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent );

	/**
	 * Called when a touchpad touch is moved  (finger moved)
	 * 
	 * @param TouchEvent	The touch event generated
	 */
	virtual FReply OnTouchMoved( const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent );

	/**
	 * Called when a touchpad touch is ended (finger lifted)
	 * 
	 * @param TouchEvent	The touch event generated
	 */
	virtual FReply OnTouchEnded( const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent );

	/**
	 * Called when motion is detected (controller or device)
	 * 
	 * @param MotionEvent	The motion event generated
	 */
	virtual FReply OnMotionDetected( const FGeometry& MyGeometry, const FMotionEvent& InMotionEvent );

	/**
	 * Called when the mouse is moved over the widget's window, to determine if we should report whether
	 * OS-specific features should be active at this location (such as a title bar grip, system menu, etc.)
	 * Usually you should not need to override this function.
	 *
	 * @return	The window "zone" the cursor is over, or EWindowZone::Unspecified if no special behavior is needed
	 */
	virtual EWindowZone::Type GetWindowZoneOverride() const;

	//
	// LAYOUT
	//

	/**
	 * Applies styles as it descends through the widget hierarchy.
	 * Gathers desired sizes on the way up.
	 * i.e. Caches the desired size of all of this widget's children recursively, then caches desired size for itself.
	 */
	void SlatePrepass();

	/** @return the DesiredSize that was computed the last time CacheDesiredSize() was called. */
	const FVector2D& GetDesiredSize() const;

	/**
	 * The system calls this method. It performs a breadth-first traversal of every visible widget and asks
	 * each widget to cache how big it needs to be in order to present all of its content.
	 */
	protected: virtual void CacheDesiredSize();
	
	/**
	 * Explicitly set the desired size. This is highly advanced functionality that is meant
	 * to be used in conjunction with overriding CacheDesiredSize. Use ComputeDesiredSize() instead.
	 */
	protected: void Advanced_SetDesiredSize(const FVector2D& InDesiredSize) { DesiredSize = InDesiredSize; }

	/**
	 * Compute the ideal size necessary to display this widget. For aggregate widgets (e.g. panels) this size should include the
	 * size necessary to show all of its children. CacheDesiredSize() guarantees that the size of descendants is computed and cached
	 * before that of the parents, so it is safe to call GetDesiredSize() for any children while implementing this method.
	 *
	 * Note that ComputeDesiredSize() is meant as an aide to the developer. It is NOT meant to be very robust in many cases. If your
	 * widget is simulating a bouncing ball, you should just return a reasonable size; e.g. 160x160. Let the programmer set up a reasonable
	 * rule of resizing the bouncy ball simulation.
	 *
	 * @return The desired size.
	 */
	protected: virtual FVector2D ComputeDesiredSize() const = 0;

	public:

	/**
	 * Compute the Geometry of all the children and add populate the ArrangedChildren list with their values.
	 * Each type of Layout panel should arrange children based on desired behavior.
	 *
	 * @param AllottedGeometry    The geometry allotted for this widget by its parent.
	 * @param ArrangedChildren    The array to which to add the WidgetGeometries that represent the arranged children.
	 */
	virtual void ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const = 0;

	/**
	 * Every widget that has children must implement this method. This allows for iteration over the Widget's
	 * children regardless of how they are actually stored.
	 */
	// @todo Slate: Consider renaming to GetVisibleChildren  (not ALL children will be returned in all cases)
	virtual FChildren* GetChildren() = 0;

	/**
	 * Checks to see if this widget supports keyboard focus.  Override this in derived classes.
	 *
	 * @return  True if this widget can take keyboard focus
	 */
	virtual bool SupportsKeyboardFocus() const;


	/**
	 * Checks to see if this widget currently has the keyboard focus
	 *
	 * @return  True if this widget has keyboard focus
	 */
	virtual bool HasKeyboardFocus() const;

	/**
	 * @return Whether this widget has any descendants with keyboard focus
	 */
	bool HasFocusedDescendants() const;

	/**
	 * Checks to see if this widget is the current mouse captor
	 *
	 * @return  True if this widget has captured the mouse
	 */
	bool HasMouseCapture() const;

	/**
	 * Called when this widget had captured the mouse, but that capture has been revoked for some reason.
	 */
	virtual void OnMouseCaptureLost();

	/**
	 * Ticks this widget and all of it's child widgets.  Should not be called directly.
	 *
	 * @param  AllottedGeometry The space allotted for this widget
	 * @param  InCurrentTime  Current absolute real time
	 * @param  InDeltaTime  Real time passed since last tick
	 */
	void TickWidgetsRecursively( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime );

	/**
	 * Sets the enabled state of this widget
	 *
	 * @param InEnabledState	An attribute containing the enabled state or a delegate to call to get the enabled state.
	 */
	void SetEnabled( const TAttribute<bool>& InEnabledState )
	{
		EnabledState = InEnabledState;
	}

	/** @return Whether or not this widget is enabled */
	bool IsEnabled() const 
	{
		return EnabledState.Get();
	}

	/** @return The tool tip associated with this widget; Invalid reference if there is not one */
	virtual TSharedPtr<SToolTip> GetToolTip();

	/** Called when a tooltip displayed from this widget is being closed */
	virtual void OnToolTipClosing();

	/**
	 * Sets whether this widget is a "tool tip force field".  That is, tool-tips should never spawn over the area
	 * occupied by this widget, and will instead be repelled to an outside edge
	 *
	 * @param	bEnableForceField	True to enable tool tip force field for this widget
	 */
	void EnableToolTipForceField( const bool bEnableForceField );

	/** @return True if a tool tip force field is active on this widget */
	bool HasToolTipForceField() const
	{
		return bToolTipForceFieldEnabled;
	}

	/** @return True if this widget hovered */
	virtual bool IsHovered() const
	{
		return bIsHovered;
	}

	/** @return is this widget visible, hidden or collapsed */
	EVisibility GetVisibility() const
	{
		return Visibility.Get();
	}

	/** @param InVisibility  should this widget be */
	void SetVisibility( TAttribute<EVisibility> InVisibility )
	{
		Visibility = InVisibility;
	}

	/**
	 * Set the tool tip that should appear when this widget is hovered.
	 *
	 * @param InToolTipText  the text that should appear in the tool tip
	 */
	void SetToolTipText( const TAttribute<FString>& InToolTipText );

	void SetToolTipText( const FText& InToolTipText );

	/**
	 * Set the tool tip that should appear when this widget is hovered.
	 *
	 * @param InToolTip  the widget that should appear in the tool tip
	 */
	void SetToolTip( const TSharedPtr<SToolTip>& InToolTip );

	/**
	 * Set the cursor that should appear when this widget is hovered
	 */
	void SetCursor( const TAttribute< TOptional<EMouseCursor::Type> >& InCursor );

	/**
	 * Used by Slate to set the runtime debug info about this widget.
	 */
	void SetDebugInfo( const ANSICHAR* InType, const ANSICHAR* InFile, int32 OnLine );

public:
	//
	// Widget Inspector and debugging methods
	//

	/** @return A String representation of the widget */
	virtual FString ToString() const;

	/** @return A String of the widget's type */
	FString GetTypeAsString() const;

	/** @return The widget's type as an FName ID */
	FName GetType() const;

	/** @return A String of the widget's code location in readable format */
	virtual FString GetReadableLocation() const;

	/** @return A String of the widget's code location in parsable format */
	virtual FString GetParsableFileAndLineNumber() const;

	/** @return the Foreground color that this widget sets; unset options if the widget does not set a foreground color */
	virtual FSlateColor GetForegroundColor() const;

protected:

	/** Constructor */
	SWidget();

	/** 
	 * Find the geometry of a descendant widget. This method assumes that WidgetsToFind are a descendants of this widget.
	 * Note that not all widgets are guaranteed to be found; OutResult will contain null entries for missing widgets.
	 *
	 * @param MyGeometry      The geometry of this widget.
	 * @param WidgetsToFind   The widgets whose geometries we wish to discover.
	 * @param OutResult       A map of widget references to their respective geometries.
	 *
	 * @return True if all the WidgetGeometries were found. False otherwise.
	 */
	bool FindChildGeometries( const FGeometry& MyGeometry, const TSet< TSharedRef<SWidget> >& WidgetsToFind, TMap<TSharedRef<SWidget>, FArrangedWidget>& OutResult ) const;

	/**
	 * Actual implementation of FindChildGeometries.
	 *
	 * @param MyGeometry      The geometry of this widget.
	 * @param WidgetsToFind   The widgets whose geometries we wish to discover.
	 * @param OutResult       A map of widget references to their respective geometries.
	 */
	void FindChildGeometries_Helper( const FGeometry& MyGeometry, const TSet< TSharedRef<SWidget> >& WidgetsToFind, TMap<TSharedRef<SWidget>, FArrangedWidget>& OutResult ) const;

	/** 
	 * Find the geometry of a descendant widget. This method assumes that WidgetToFind is a descendant of this widget.
	 *
	 * @param MyGeometry   The geometry of this widget.
	 * @param WidgetToFind The widget whose geometry we wish to discover.
	 *
	 * @return the geometry of WidgetToFind.
	 */
	FGeometry FindChildGeometry( const FGeometry& MyGeometry, TSharedRef<SWidget> WidgetToFind ) const;

	/** @return The index of the child that the mouse is currently hovering */
	static int32 FindChildUnderMouse( const FArrangedChildren& Children, const FPointerEvent& MouseEvent );

	/** 
	 * Determines if this widget should be enabled.
	 * 
	 * @param InParentEnabled	true if the parent of this widget is enabled
	 * @return true if the widget is enabled
	 */
	bool ShouldBeEnabled( bool InParentEnabled ) const
	{
		// This widget should be enabled if its parent is enabled and it is enabled
		return IsEnabled() && InParentEnabled;
	}


protected:
	
	//	DEBUG INFORMATION
	// @todo Slate: Should compile out in final release builds?
	FName TypeOfWidget;
	/** Full file path in which this widget was created */
	FName CreatedInFileFullPath;
	/** Filename in which this widget was created */
	FName CreatedInFile;
	/** Line number on which this widget was created */
	int32 CreatedOnLine;

	/** The cursor to show when the mouse is hovering over this widget. */
	TAttribute< TOptional<EMouseCursor::Type> > Cursor;

	/** Whether or not this widget is enabled */
	TAttribute< bool > EnabledState;

	/** Is this widget visible, hidden or collapsed */
	TAttribute< EVisibility > Visibility;

private:

	/**
	 * Stores the ideal size this widget wants to be.
	 * This member is intentionally private, because only the very base class (Widget) can write DesiredSize.
	 * See CacheDesiredSize(), ComputeDesiredSize()
	 */
	FVector2D DesiredSize;

	/** Tool tip content for this widget */
	TSharedPtr<SToolTip> ToolTip;

	// Whether this widget is a "tool tip force field".  That is, tool-tips should never spawn over the area
	// occupied by this widget, and will instead be repelled to an outside edge
 	bool bToolTipForceFieldEnabled;

	/** Is this widget hovered? */
	bool bIsHovered;
};
