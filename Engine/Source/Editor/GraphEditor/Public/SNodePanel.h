// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GraphEditorModule.h"

//@TODO: Too generic of a name to expose at this scope
typedef class UObject* SelectedItemType;

// Level of detail for graph rendering (lower numbers are 'further away' with fewer details)
namespace EGraphRenderingLOD
{
	enum Type
	{
		// Detail level when zoomed all the way out (all performance optimizations enabled)
		LowestDetail,

		// Detail level that text starts being disabled because it is unreadable
		LowDetail,

		// Detail level at which text starts to get hard to read but is still drawn
		MediumDetail,

		// Detail level when zoomed in at 1:1
		DefaultDetail,

		// Detail level when fully zoomed in (past 1:1)
		FullyZoomedIn,
	};
}

/** Helper for managing marquee operations */
struct FMarqueeOperation
{
	FMarqueeOperation()
		: Operation(Add)
	{
	}

	enum Type
	{
		/** Holding down Ctrl removes nodes */
		Remove,
		/** Holding down Shift adds to the selection */
		Add,
		/** When nothing is pressed, marquee replaces selection */
		Replace
	} Operation;

	bool IsValid() const
	{
		return Rect.IsValid();
	}

	void Start(const FVector2D& InStartLocation, FMarqueeOperation::Type InOperationType)
	{
		Rect = FMarqueeRect(InStartLocation);
		Operation = InOperationType;
	}

	void End()
	{
		Rect = FMarqueeRect();
	}


	/** Given a mouse event, figure out what the marquee selection should do based on the state of Shift and Ctrl keys */
	static FMarqueeOperation::Type OperationTypeFromMouseEvent(const FPointerEvent& MouseEvent)
	{
		if (MouseEvent.IsControlDown())
		{
			return FMarqueeOperation::Remove;
		}
		else if (MouseEvent.IsShiftDown())
		{
			return FMarqueeOperation::Add;
		}
		else
		{
			return FMarqueeOperation::Replace;
		}
	}

public:
	/** The marquee rectangle being dragged by the user */
	FMarqueeRect Rect;

	/** Nodes that will be selected or unselected by the current marquee operation */
	FGraphPanelSelectionSet AffectedNodes;
};

struct GRAPHEDITOR_API FGraphSelectionManager
{
	FGraphPanelSelectionSet SelectedNodes;

	/** Invoked when the selected graph nodes have changed. */
	SGraphEditor::FOnSelectionChanged OnSelectionChanged;
public:
	/** @return the set of selected nodes */
	const FGraphPanelSelectionSet& GetSelectedNodes() const;

	/** Select just the specified node */
	void SelectSingleNode(SelectedItemType Node);

	/** Reset the selection state of all nodes */
	void ClearSelectionSet();

	/** Returns true if any nodes are selected */
	bool AreAnyNodesSelected() const
	{
		return SelectedNodes.Num() > 0;
	}

	/** Changes the selection set to contain exactly all of the passed in nodes */
	void SetSelectionSet(FGraphPanelSelectionSet& NewSet);

	/**
	 * Add or remove a node from the selection set
	 *
	 * @param Node      Node the affect.
	 * @param bSelect   true to select the node; false to unselect.
	 */
	void SetNodeSelection(SelectedItemType Node, bool bSelect);

	/** @return true if Node is selected; false otherwise */
	bool IsNodeSelected(SelectedItemType Node) const;

	// Handle the selection mechanics of starting to drag a node
	void StartDraggingNode(SelectedItemType NodeBeingDragged, const FPointerEvent& MouseEvent);

	// Handle the selection mechanics when a node is clicked on
	void ClickedOnNode(SelectedItemType Node, const FPointerEvent& MouseEvent);
};

// Context passed in when getting popup info
struct FNodeInfoContext
{
public:
	bool bSelected;
};

// Entry for an overlay brush in the node panel
struct FOverlayBrushInfo
{
public:
	/** Brush to draw */
	const FSlateBrush* Brush;
	/** Scale of animation to apply */
	FVector2D AnimationEnvelope;
	/** Offset origin of the overlay from the widget */
	FVector2D OverlayOffset;

public:
	FOverlayBrushInfo()
		: Brush(NULL)
		, AnimationEnvelope(0.0f, 0.0f)
		, OverlayOffset(0.f, 0.f)
	{
	}

	FOverlayBrushInfo(const FSlateBrush* InBrush)
		: Brush(InBrush)
		, AnimationEnvelope(0.0f, 0.0f)
		, OverlayOffset(0.f, 0.f)
	{
	}

	FOverlayBrushInfo(const FSlateBrush* InBrush, float HorizontalBounce)
		: Brush(InBrush)
		, AnimationEnvelope(HorizontalBounce, 0.0f)
		, OverlayOffset(0.f, 0.f)
	{
	}
};

// Entry for an information popup in the node panel
struct FGraphInformationPopupInfo
{
public:
	const FSlateBrush* Icon;
	FLinearColor BackgroundColor;
	FString Message;
public:
	FGraphInformationPopupInfo(const FSlateBrush* InIcon, FLinearColor InBackgroundColor, const FString& InMessage)
		: Icon(InIcon)
		, BackgroundColor(InBackgroundColor)
		, Message(InMessage)
	{
	}
};

/**
 * Interface for ZoomLevel values
 * Provides mapping for a range of virtual ZoomLevel values to actual node scaling values
 */
struct FZoomLevelsContainer
{
	/** 
	 * @param InZoomLevel virtual zoom level value
	 * 
	 * @return associated scaling value
	 */
	virtual float						GetZoomAmount(int32 InZoomLevel) const = 0;
	
	/** 
	 * @param InZoomAmount scaling value
	 * 
	 * @return nearest ZoomLevel mapping for provided scale value
	 */
	virtual int32						GetNearestZoomLevel(float InZoomAmount) const = 0;
	
	/** 
	 * @param InZoomLevel virtual zoom level value
	 * 
	 * @return associated friendly name
	 */
	virtual FString						GetZoomPrettyString(int32 InZoomLevel) const = 0;
	
	/** 
	 * @return count of supported zoom levels
	 */
	virtual int32						GetNumZoomLevels() const = 0;
	
	/** 
	 * @return the optimal(1:1) zoom level value, default zoom level for the graph
	 */
	virtual int32						GetDefaultZoomLevel() const = 0;
	
	/** 
	 * @param InZoomLevel virtual zoom level value
	 *
	 * @return associated LOD value
	 */
	virtual EGraphRenderingLOD::Type	GetLOD(int32 InZoomLevel) const = 0;

	// Necessary for Mac OS X to compile 'delete <pointer_to_this_object>;'
	virtual ~FZoomLevelsContainer( void ) {};
};

/**
 * This class is designed to serve as the base class for a panel/canvas that contains interactive widgets
 * which can be selected and moved around by the user.  It also manages zooming and panning, allowing a larger
 * virtual space to be used for the widget placement.
 *
 * The user is responsible for creating widgets (which must be derived from SNode) and any custom drawing
 * code desired.  The other main restriction is that each SNode instance must have a unique UObject* associated
 * with it.
 */
class GRAPHEDITOR_API SNodePanel : public SPanel
{
public:
	class SNode : public SBorder
	{
	public:
		/** @param NewPosition  The Node should be relocated to this position in the graph panel */
		virtual void MoveTo(const FVector2D& NewPosition)
		{
		}

		/** @return the Node's position within the graph */
		virtual FVector2D GetPosition() const
		{
			return FVector2D(0.0f, 0.0f);
		}

		/** @return a user-specified comment on this node; the comment gets drawn in a bubble above the node */
		virtual FString GetNodeComment() const
		{
			return FString();
		}

		/** @return The backing object, used as a unique identifier in the selection set, etc... */
		virtual UObject* GetObjectBeingDisplayed() const
		{
			return NULL;
		}

		/** @return The brush to use for drawing the shadow for this node */
		virtual const FSlateBrush* GetShadowBrush(bool bSelected) const
		{
			return bSelected ? FEditorStyle::GetBrush(TEXT("Graph.Node.ShadowSelected")) : FEditorStyle::GetBrush(TEXT("Graph.Node.Shadow"));
		}

		/** Populate the brushes array with any overlay brushes to render */
		virtual void GetOverlayBrushes(bool bSelected, const FVector2D WidgetSize, TArray<FOverlayBrushInfo>& Brushes) const
		{
		}

		/** Populate the popups array with any popups to render */
		virtual void GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& Popups) const
		{
		}

		/** Returns true if this node is dependent on the location of other nodes (it can only depend on the location of first-pass only nodes) */
		virtual bool RequiresSecondPassLayout() const
		{
			return false;
		}

		/** Performs second pass layout; only called if RequiresSecondPassLayout returned true */
		virtual void PerformSecondPassLayout(const TMap< UObject*, TSharedRef<SNode> >& InNodeToWidgetLookup) const
		{
		}

		/** Return false if this node should not be culled. Useful for potentially large nodes that may be improperly culled. */
		virtual bool ShouldAllowCulling() const
		{
			return true;
		}

		/** return if the node can be selected, by pointing given location */
		virtual bool CanBeSelected(const FVector2D& MousePositionInNode) const
		{
			return true;
		}

		/** 
		 *	override, when area used to select node, should be different, than it's size
		 *	e.g. comment node - only title bar is selectable
		 *	return size of node used for Marquee selecting
		 */
		virtual FVector2D GetDesiredSizeForMarquee() const
		{
			return GetDesiredSize();
		}

	protected:
		SNode()
		{
			BorderImage = FEditorStyle::GetBrush("NoBorder");
			ForegroundColor = FEditorStyle::GetColor("Graph.ForegroundColor");
		}
	};

	// SPanel interface
	virtual void ArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const OVERRIDE;
	virtual FVector2D ComputeDesiredSize() const OVERRIDE;
	virtual FChildren* GetChildren() OVERRIDE;
	// End of SPanel interface

	// SWidget interface
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) OVERRIDE;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) OVERRIDE;
	virtual FCursorReply OnCursorQuery( const FGeometry& MyGeometry, const FPointerEvent& CursorEvent ) const OVERRIDE;
	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent ) OVERRIDE;
	virtual FReply OnKeyUp( const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent ) OVERRIDE;
	virtual void OnKeyboardFocusLost( const FKeyboardFocusEvent& InKeyboardFocusEvent ) OVERRIDE;

	// End of SWidget interface
public:
	/**
	 * Is the given node being observed by a widget in this panel?
	 * 
	 * @param Node   The node to look for.
	 *
	 * @return True if the node is being observed by some widget in this panel; false otherwise.
	 */
	bool Contains(UObject* Node) const;

	/** @retun the zoom amount; e.g. a value of 0.25f results in quarter-sized nodes */
	float GetZoomAmount() const;
	/** @return Zoom level as a pretty string */
	FString GetZoomString() const;
	FSlateColor GetZoomTextColorAndOpacity() const;

	/** @return the view offset in graph space */
	FVector2D GetViewOffset() const;

	/** Given a coordinate in panel space (i.e. panel widget space), return the same coordinate in graph space while taking zoom and panning into account */
	FVector2D PanelCoordToGraphCoord(const FVector2D& PanelSpaceCoordinate) const;

	/** Restore the graph panel to the supplied view offset/zoom */
	void RestoreViewSettings(const FVector2D& InViewOffset, float InZoomAmount);

	/** Get the grid snap size */
	static float GetSnapGridSize()
	{
		return 16.f;
	}

	// Zooms out to fit either all nodes or only the selected ones
	void ZoomToFit(bool bOnlySelection);

	/** Get the bounding area for the currently selected nodes 
		@return false if nothing is selected					*/
	bool GetBoundsForSelectedNodes(/*out*/ class FSlateRect& Rect, float Padding = 0.0f);

	/** @return the position where where nodes should be pasted (i.e. from the clipboard) */
	FVector2D GetPastePosition() const;

	/** Ask panel to scroll to location */
	void RequestDeferredPan(const FVector2D& TargetPosition);

	/** If it is focusing on a particular object */
	bool HasDeferredObjectFocus() const;

	/** Returns the current LOD level of this panel, based on the zoom factor */
	EGraphRenderingLOD::Type GetCurrentLOD() const { return CurrentLOD; }

	/** Returns if the panel has been panned or zoomed since the last update */
	bool HasMoved() const;

protected:
	/** Initialize members */
	void Construct();

	/** Update the new view offset location  */
	void UpdateViewOffset(const FGeometry& MyGeometry, const FVector2D& TargetPosition);

	/** Compute much panel needs to change to pan to location  */
	static FVector2D ComputeEdgePanAmount(const FGeometry& MyGeometry, const FVector2D& MouseEvent);

	/** Given a coordinate in graph space (e.g. a node's position), return the same coordinate in widget space while taking zoom and panning into account */
	FVector2D GraphCoordToPanelCoord(const FVector2D& GraphSpaceCoordinate) const;

	/** Given a rectangle in panel space, return a rectangle in graph space. */
	FSlateRect PanelRectToGraphRect(const FSlateRect& PanelSpaceRect) const;

	/** 
	 * Lets the CanvasPanel know that the user is interacting with a node.
	 * 
	 * @param InNodeToDrag   The node that the user wants to drag
	 * @param GrabOffset     Where within the node the user grabbed relative to split between inputs and outputs.
	 */
	virtual void OnBeginNodeInteraction(const TSharedRef<SNode>& InNodeToDrag, const FVector2D& GrabOffset);

	/** 
	 * Lets the CanvasPanel know that the user has ended interacting with a node.
	 * 
	 * @param InNodeToDrag   The node that the user was to dragging
	 */
	virtual void OnEndNodeInteraction(const TSharedRef<SNode>& InNodeToDrag);

	/** Figure out which nodes intersect the marquee rectangle */
	void FindNodesAffectedByMarquee(FGraphPanelSelectionSet& OutAffectedNodes) const;

	/**
	 * Apply the marquee operation to the current selection
	 *
	 * @param InMarquee            The marquee operation to apply.
	 * @param CurrentSelection     The selection before the marquee operation.
	 * @param OutNewSelection      The selection resulting from Marquee being applied to CurrentSelection.
	 */
	static void ApplyMarqueeSelection(const FMarqueeOperation& InMarquee, const FGraphPanelSelectionSet& CurrentSelection, FGraphPanelSelectionSet& OutNewSelection);

	/** @return a bounding rectangle around all the node locations; the graph bounds are padded out for the user's convenience */
	FSlateRect ComputeSensibleGraphBounds() const;

	/**
	 * On the next tick, centers and selects the widget associated with the object if it exists
	 *
	 * @param ObjectToSelect	The object to select, and potentially center on
	 * @param bCenter			Whether or not to center the graph node
	 */
	void SelectAndCenterObject(const UObject* ObjectToSelect, bool bCenter);

	/** Add a slot to the CanvasPanel dynamically */
	virtual void AddGraphNode(const TSharedRef<SNode>& NodeToAdd);

	/** Add a node widget to the back of the panel */
	virtual void AddGraphNodeToBack(const TSharedRef<SNode>& NodeToAdd);

	/** Remove all nodes from the panel */
	virtual void RemoveAllNodes();
	
	/** Populate visibile children array */
	virtual void PopulateVisibleChildren(const FGeometry& AllottedGeometry);

	// Paint the background as lines
	void PaintBackgroundAsLines(const FSlateBrush* BackgroundImage, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32& DrawLayerId) const;

	// Paint the well shadow (around the perimeter)
	void PaintSurroundSunkenShadow(const FSlateBrush* ShadowImage, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 DrawLayerId) const;

	// Paint the marquee selection rectangle
	void PaintMarquee(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 DrawLayerId) const;

	// Paint the software mouse if necessary
	void PaintSoftwareCursor(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 DrawLayerId) const;

	// Paint a comment bubble
	void PaintComment(const FString& CommentText, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 DrawLayerId, const FLinearColor& CommentTinting, float& HeightAboveNode, const FWidgetStyle& InWidgetStyle ) const;

	/** Determines if a specified node is not visually relevant. */
	bool IsNodeCulled(const TSharedRef<SNode>& Node, const FGeometry& AllottedGeometry) const;

protected:
	///////////
	// INTERFACE TO IMPLEMENT
	///////////
	/** @return the widget in the summoned context menu that should be focused. */
	virtual TSharedPtr<SWidget> OnSummonContextMenu(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) { return TSharedPtr<SWidget>(); }
	virtual bool OnHandleLeftMouseRelease(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) { return false; }
protected:
	/**
	 * Get the bounds of the selected nodes 
	 * @param bSelectionSetOnly If true, limits the query to just the selected nodes.  Otherwise it does all nodes.
	 * @return True if successful
	 */
	bool GetBoundsForNodes(bool bSelectionSetOnly, /*out*/ FVector2D& MinCorner, /*out*/ FVector2D& MaxCorner, float Padding = 0.0f);

	/**
	 * Scroll the view to the desired location 
	 * @return true when the desired location is reached
	 */
	bool ScrollToLocation(const FGeometry& MyGeometry, FVector2D DesiredCenterPosition, const float InDeltaTime);

	/**
	 * Zoom to fit the desired size 
	 * @return true when zoom fade has completed & fits the desired size
	 */
	bool ZoomToLocation(const FVector2D& CurrentSizeWithoutZoom, const FVector2D& DesiredSize, bool bDoneScrolling);

	/**
	 * Change zoom level by the specified zoom level delta, about the specified origin.
	 */
	void ChangeZoomLevel(int32 ZoomLevelDelta, const FVector2D& WidgetSpaceZoomOrigin, bool bOverrideZoomLimiting);

	// Should be called whenever the zoom level has changed
	void PostChangedZoom();
protected:
	// The interface for mapping ZoomLevel values to actual node scaling values
	TScopedPointer<FZoomLevelsContainer> ZoomLevels;

	/** The position within the graph at which the user is looking */
	FVector2D ViewOffset;

	/** The position within the graph at which the user was looking last tick */
	FVector2D OldViewOffset;

	/** How zoomed in/out we are. e.g. 0.25f results in quarter-sized nodes. */
	int32 ZoomLevel;

	/** Previous Zoom Level */
	int32 PreviousZoomLevel;

	/** The actual scalar zoom amount last tick */
	float OldZoomAmount;

	/** Are we panning the view at the moment? */
	bool bIsPanning;

	/** The graph node widgets owned by this panel */
	TSlotlessChildren<SNode> Children;
	TSlotlessChildren<SNode> VisibleChildren;

	/** The node that the user is dragging. Null when they are not dragging a node. */
	TWeakPtr<SNode> NodeUnderMousePtr;

	/** Where in the title the user grabbed to initiate the drag */
	FVector2D NodeGrabOffset;

	/** The total distance that the mouse has been dragged while down */
	float TotalMouseDelta;

	/** The Y component of mouse drag (used when zooming) */
	float TotalMouseDeltaY;

	/** Offset in the panel the user started the LMB+RMB zoom from */
	FVector2D ZoomStartOffset;
public:
	/** Nodes selected in this instance of the editor; the selection is per-instance of the GraphEditor */
	FGraphSelectionManager SelectionManager;
protected:
	/** A pending marquee operation if it's active */
	FMarqueeOperation Marquee;

	/** Is the graph editable (can nodes be moved, etc...)? */
	TAttribute<bool> IsEditable;

	/** Given a node, find the corresponding widget */
	TMap< UObject*, TSharedRef<SNode> > NodeToWidgetLookup;

	/** If non-null and a part of this panel, this node will be selected and brought into view on the next Tick */
	const UObject* DeferredSelectionTargetObject;
	/** If non-null and a part of this panel, this node will be brought into view on the next Tick */
	const UObject* DeferredMovementTargetObject;

	/** Deferred zoom to selected node extents */
	bool bDeferredZoomToSelection;

	/** Deferred zoom to node extents */
	bool bDeferredZoomToNodeExtents;

	/** Are we currently zooming to fit? */
	bool bDeferredZoomingToFit;

	/** Zoom selection padding */
	float ZoomPadding;

	/** Zoom target rectangle */
	FVector2D ZoomTargetTopLeft;
	FVector2D ZoomTargetBottomRight;

	/** Allow continous zoom interpolation? */
	bool bAllowContinousZoomInterpolation;

	/** Teleport immediately, or smoothly scroll when doing a deferred zoom */
	bool bTeleportInsteadOfScrollingWhenZoomingToFit;

	/** Fade on zoom for graph */
	FCurveSequence ZoomLevelGraphFade;

	/** Curve that handles fading the 'Zoom +X' text */
	FCurveSequence ZoomLevelFade;

	/** The position where we should paste when a user executes the paste command. */
	FVector2D PastePosition;

	/** Position to pan to */
	FVector2D DeferredPanPosition;

	/** true if pending request for deferred panning */
	bool bRequestDeferredPan;

	/**	The current position of the software cursor */
	FVector2D SoftwareCursorPosition;

	/**	Whether the software cursor should be drawn */
	bool bShowSoftwareCursor;

	/** Current LOD level for nodes/pins */
	EGraphRenderingLOD::Type CurrentLOD;

	/** Invoked when the user may be attempting to spawn a node using a shortcut */
	SGraphEditor::FOnSpawnNodeByShortcut OnSpawnNodeByShortcut;

	/** The last key gesture detected in this graph panel */
	FInputGesture LastKeyGestureDetected;
};
