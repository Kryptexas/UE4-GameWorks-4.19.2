// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LinkedObjEditor.h: Base class for boxes-and-lines editing
=============================================================================*/

#pragma once


/**
 * Provides notifications of messages sent from a linked object editor workspace.
 */
class FLinkedObjEdNotifyInterface
{
public:
	/* =======================================
		Input methods and notifications
	   =====================================*/

	/**
	 * Called when the user right-clicks on an empty region of the viewport
	 */
	virtual void OpenNewObjectMenu() {}

	/**
	 * Called when the user right-clicks on an object in the viewport
	 */
	virtual void OpenObjectOptionsMenu() {}

	virtual void ClickedLine(struct FLinkedObjectConnector &Src, struct FLinkedObjectConnector &Dest) {}
	virtual void DoubleClickedLine(struct FLinkedObjectConnector &Src, struct FLinkedObjectConnector &Dest) {}

	/**
	 * Called when the user double-clicks an object in the viewport
	 *
	 * @param	Obj		the object that was double-clicked on
	 */
	virtual void DoubleClickedObject(UObject* Obj) {}

	/**
	 * Called whent he use double-clicks an object connector.
	 */
	virtual void DoubleClickedConnector(struct FLinkedObjectConnector& Connector) {}
																			   
	/**
	 * Called when the user left-clicks on an empty region of the viewport
	 *
	 * @return	true to deselect all selected objects
	 */
	virtual bool ClickOnBackground() {return true;}

	/**
	 *	Called when the user left-clicks on a connector.
	 *
	 *	@return true to deselect all selected objects
	 */
	virtual bool ClickOnConnector(UObject* InObj, int32 InConnType, int32 InConnIndex) {return true;}

	/**
	 * Called when the user performs a draw operation while objects are selected.
	 *
	 * @param	DeltaX	the X value to move the objects
	 * @param	DeltaY	the Y value to move the objects
	 */
	virtual void MoveSelectedObjects( int32 DeltaX, int32 DeltaY ) {}

	/**
	 * Hook for child classes to perform special logic for key presses.  Called for all key events, event
	 * those which cause other functions to be called (such as ClickOnBackground, etc.)
	 * @returns true if the input was handled
	 */
	virtual bool EdHandleKeyInput(FViewport* Viewport, FKey Key, EInputEvent Event)=0;

	/**
	 * Called once when the user mouses over an new object.  Child classes should implement
	 * this function if they need to perform any special logic when an object is moused over
	 *
	 * @param	Obj		the object that is currently under the mouse cursor
	 */
	virtual void OnMouseOver(UObject* Obj) {}

	/**
	 * Called when the user clicks something in the workspace that uses a special hit proxy (HLinkedObjProxySpecial),
	 * Hook for preventing AddToSelection/SetSelectedConnector from being called for the clicked element.
	 *
	 * @return	false to prevent any selection methods from being called for the clicked element
	 */
	virtual bool SpecialClick( int32 NewX, int32 NewY, int32 SpecialIndex, FViewport* Viewport, UObject* ProxyObj ) { return 0; }

	/**
	 * Called when the user drags an object that uses a special hit proxy (HLinkedObjProxySpecial)
	 * (corresponding method for non-special objects is MoveSelectedObjects)
	 */
	virtual void SpecialDrag( int32 DeltaX, int32 DeltaY, int32 NewX, int32 NewY, int32 SpecialIndex ) {}

	/* =======================================
		General methods and notifications
	   =====================================*/

	/**
	 * Child classes should implement this function to render the objects that are currently visible.
	 */
	virtual void DrawObjects(FViewport* Viewport, FCanvas* Canvas)=0;

	/**
	 * Called when the viewable region is changed, such as when the user pans or zooms the workspace.
	 */
	virtual void ViewPosChanged() {}

	/**
	 * Called when something happens in the linked object drawing that invalidates the property window's
	 * current values, such as selecting a new object.
	 * Child classes should implement this function to update any property windows which are being displayed.
	 */
	virtual void UpdatePropertyWindow() {}

	/**
	 * Called when one or more objects changed
	 */
	virtual void NotifyObjectsChanged() {}

	/**
	 * Called once the user begins a drag operation.  Child classes should implement this method
	 * if they the position of the selected objects is part of the object's state (and thus, should
	 * be recorded into the transaction buffer)
	 */
	virtual void BeginTransactionOnSelected() {}

	/**
	 * Called when the user releases the mouse button while a drag operation is active.
	 * Child classes should implement this only if they also implement BeginTransactionOnSelected.
	 */
	virtual void EndTransactionOnSelected() {}

	/* =======================================
		Selection methods and notifications
	   =====================================*/

	/**
	 * Empty the list of selected objects.
	 */
	virtual void EmptySelection() {}

	/**
	 * Add the specified object to the list of selected objects
	 */
	virtual void AddToSelection( UObject* Obj ) {}

	/**
	 * Remove the specified object from the list of selected objects.
	 */
	virtual void RemoveFromSelection( UObject* Obj ) {}

	/**
	 * Checks whether the specified object is currently selected.
	 * Child classes should implement this to check for their specific
	 * object class.
	 *
	 * @return	true if the specified object is currently selected
	 */
	virtual bool IsInSelection( UObject* Obj ) const { return false; }

	/**
	 * Returns the number of selected objects
	 */
	virtual int32 GetNumSelected() const { return 0; }

	/**
	 * Checks whether we have any objects selected.
	 */
	bool HaveObjectsSelected() const { return GetNumSelected() > 0; }

	/**
	 * Called once the mouse is released after moving objects.
	 * Snaps the selected objects to the grid.
	 */
	virtual void PositionSelectedObjects() {}

	/**
	 * Called when the user right-clicks on an object connector in the viewport.
	 */
	virtual void OpenConnectorOptionsMenu() {}

	// Selection-related methods

	/**
	 * Called when the user clicks on an unselected link connector.
	 *
	 * @param	Connector	the link connector that was clicked on
	 */
	virtual void SetSelectedConnector( struct FLinkedObjectConnector& Connector ) {}

	/**
	 * Gets the position of the selected connector.
	 *
	 * @return	an FIntPoint that represents the position of the selected connector, or (0,0)
	 *			if no connectors are currently selected
	 */
	virtual FIntPoint GetSelectedConnLocation(FCanvas* Canvas) { return FIntPoint(0,0); }

	/**
	 * Adjusts the postion of the selected connector based on the Delta position passed in.
	 * Currently only variable, event, and output connectors can be moved. 
	 * 
	 * @param DeltaX	The amount to move the connector in X
	 * @param DeltaY	The amount to move the connector in Y	
	 */
	virtual void MoveSelectedConnLocation( int32 DeltaX, int32 DeltaY ) {}

	/**
	 * Sets the member variable on the selected connector struct so we can perform different calculations in the draw code
	 * 
	 * @param bMoving	True if the connector is moving
	 */
	virtual void SetSelectedConnectorMoving( bool bMoving ) {}
	/**
	 * Gets the EConnectorHitProxyType for the currently selected connector
	 *
	 * @return	the type for the currently selected connector, or 0 if no connector is selected
	 */
	virtual int32 GetSelectedConnectorType() { return 0; }

	/**
	 * Called when the user mouses over a linked object connector.
	 * Checks whether the specified connector should be highlighted (i.e. whether this connector is valid for linking)
	 */
	virtual bool ShouldHighlightConnector(struct FLinkedObjectConnector& Connector) { return true; }

	/**
	 * Gets the color to use for drawing the pending link connection.  Only called when the user
	 * is connecting a link to a link connector.
	 */
	virtual FColor GetMakingLinkColor() { return FColor(0,0,0); }

	/**
	 * Called when the user releases the mouse over a link connector during a link connection operation.
	 * Make a connection between selected connector and an object or another connector.
	 *
	 * @param	Connector	the connector corresponding to the endpoint of the new link
	 */
	virtual void MakeConnectionToConnector( struct FLinkedObjectConnector& Connector ) {}

	/**
	 * Called when the user releases the mouse over an object during a link connection operation.
	 * Makes a connection between the current selected connector and the specified object.
	 *
	 * @param	Obj		the object target corresponding to the endpoint of the new link connection
	 */
	virtual void MakeConnectionToObject( UObject* Obj ) {}

	/**
	 * Called when the user releases the mouse over a link connector and is holding the ALT key.
	 * Commonly used as a shortcut to breaking connections.
	 *
	 * @param	Connector	The connector that was ALT+clicked upon.
	 */
	virtual void AltClickConnector(struct FLinkedObjectConnector& Connector) {}

	/* =============
	    Bookmarks
	   ===========*/
	/**
	 * Called when the user attempts to set a bookmark via CTRL + # key.
	 *
	 * @param	InIndex		Index of the bookmark to set
	 */
	virtual void SetBookmark( uint32 InIndex ) {}

	/**
	 * Called when the user attempts to check a bookmark.
	 *
	 * @param	InIndex		Index of the bookmark to check
	 */
	virtual bool CheckBookmark( uint32 InIndex ) { return false; }

	/**
	 * Called when the user attempts to jump to a bookmark via a # key.
	 *
	 * @param	InIndex		Index of the bookmark to jump to
	 */
	virtual void JumpToBookmark( uint32 InIndex ) {}

	/* ================================================
	    Navigation history methods and notifications
	   ==============================================*/

	/**
	 * Sets the user's navigation history back one entry, if possible.
	 */
	virtual void NavigationHistoryBack() {}

	/**
	 * Sets the user's navigation history forward one entry, if possible.
	 */
	virtual void NavigationHistoryForward() {}

	/**
	 * Jumps the user's navigation history to the entry at the specified index, if possible.
	 *
	 * @param	InIndex	Index of history entry to jump to
	 */
	virtual void NavigationHistoryJump( int32 InIndex ) {}

	/**
	 * Add a new history data item to the user's navigation history, storing the current state
	 *
	 * @param	InHistoryString		The string that identifies the history data operation and will display in a navigation menu (CANNOT be empty)
	 */
	virtual void AddNewNavigationHistoryDataItem( FString InHistoryString ) {}

	/**
	 * Update the current history data item of the user's navigation history with any desired changes that have occurred since it was first added,
	 * such as camera updates, etc.
	 */
	virtual void UpdateCurrentNavigationHistoryData() {}

};

class UNREALED_API FLinkedObjViewportClient : public FEditorViewportClient
{
public:
	FLinkedObjViewportClient( FLinkedObjEdNotifyInterface* InEdInterface );

	virtual void Draw(FViewport* Viewport,FCanvas* Canvas) OVERRIDE;
	virtual bool InputKey(FViewport* Viewport, int32 ControllerId, FKey Key, EInputEvent Event, float AmountDepressed = 1.f, bool bGamepad=false) OVERRIDE;
	virtual bool InputAxis(FViewport* Viewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime, int32 NumSamples=1, bool bGamepad=false) OVERRIDE;
	virtual void MouseMove(FViewport* Viewport, int32 X, int32 Y) OVERRIDE;
	virtual void CapturedMouseMove( FViewport* InViewport, int32 InMouseX, int32 InMouseY ) OVERRIDE;
	virtual void Tick(float DeltaSeconds) OVERRIDE;

	/**
	 * Returns the cursor that should be used at the provided mouse coordinates
	 *
	 * @param Viewport	The viewport where the cursor resides
	 * @param X The X position of the mouse
	 * @param Y The Y position of the mouse
	 *
	 * @return The type of cursor to use
	 */
	virtual EMouseCursor::Type GetCursor(FViewport* Viewport,int32 X,int32 Y) OVERRIDE;

	/**
	 * Sets the cursor to be visible or not.  Meant to be called as the mouse moves around in "move canvas" mode (not just on button clicks)
	 */
	bool UpdateCursorVisibility (void);
	/**
	 * Given that we're in "move canvas" mode, set the snap back visible mouse position to clamp to the viewport
	 */
	void UpdateMousePosition(void);
	/** Determines if the cursor should presently be visible
	 * @return - true if the cursor should remain visible
	 */
	bool ShouldCursorBeVisible (void);

	/** 
	 * See if cursor is in 'scroll' region around the edge, and if so, scroll the view automatically. 
	 * Returns the distance that the view was moved.
	 */
	FIntPoint DoScrollBorder(float DeltaSeconds);

	/**
	 * Sets whether or not the viewport should be invalidated in Tick().
	 */
	void SetRedrawInTick(bool bInAlwaysDrawInTick);

	FLinkedObjEdNotifyInterface* EdInterface;

	FIntPoint Origin2D;

	/**
	 * If set non-zero, the viewport will automatically pan to DesiredOrigin2D before resetting to 0.
	 */
	float DesiredPanTime;
	FIntPoint DesiredOrigin2D;

	float Zoom2D;
	float MinZoom2D, MaxZoom2D;
	int32 NewX, NewY; // Location for creating new object

	int32 OldMouseX, OldMouseY;
	FVector2D BoxOrigin2D;
	int32 BoxStartX, BoxStartY;
	int32 BoxEndX, BoxEndY;
	int32 DistanceDragged;
	float DeltaXFraction;	// Fractional delta X, which can add up when zoomed in
	float DeltaYFraction;	// Fractional delta Y, which can add up when zoomed in

	FVector2D ScrollAccum;

	bool bTransactionBegun;
	bool bMouseDown;

	bool bMakingLine;
	
	/** Whether or not we are moving a ocnnector */
	bool bMovingConnector;

	bool bBoxSelecting;
	bool bSpecialDrag;
	bool bAllowScroll;
	
	bool bHasMouseCapture;
	bool bShowCursorOverride;
	bool bIsScrolling;

protected:

	/** Handle mouse over events */
	void OnMouseOver( int32 X, int32 Y );

	/** If true, invalidate the viewport in Tick().  Used for mouseover support, etc. */
	bool bAlwaysDrawInTick;

public:
	// For mouse over stuff.
	UObject* MouseOverObject;
	int32 MouseOverConnType;
	int32 MouseOverConnIndex;
	double MouseOverTime;
	int32 ToolTipDelayMS;

	int32 SpecialIndex;
};
