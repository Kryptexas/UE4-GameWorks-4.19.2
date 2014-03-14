// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SSection : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SSection )
	{}
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, TSharedRef<FTrackNode> SectionNode, int32 SectionIndex );

	TSharedPtr<ISequencerSection> GetSectionInterface() const { return SectionInterface; }

	/** Caches the parent geometry to be given to section interfaces that need it on tick */
	void CacheParentGeometry(const FGeometry& InParentGeometry) {ParentGeometry = InParentGeometry;}

private:
	/** Information about how to draw a key area */
	struct FKeyAreaElement
	{
		FKeyAreaElement( FSectionKeyAreaNode& InKeyAreaNode, float InHeightOffset )
			: KeyAreaNode( InKeyAreaNode )
			, HeightOffset( InHeightOffset )
		{}

		FSectionKeyAreaNode& KeyAreaNode;
		float HeightOffset;
	};

	/**
	 * Gets all the visible key areas in this section
	 *
	 * @param SectionAreaNode		The section area node containing key areas
	 * @param OutKeyAreaElements	Populated list of key elements to draw
	 */
	void GetKeyAreas( const TSharedPtr<FTrackNode>& SectionAreaNode, TArray<FKeyAreaElement>& OutKeyAreaElements ) const;

	/**
	 * Recursively gets keys areas from a list of nodes
	 *
	 * @param Nodes					List of nodes that could be key areas or contain key areas
	 * @param HeightOffset			The current height offset incremented for each key area
	 * @param OutKeyAreaElements	Populated list of key elements to draw
	 */
	void GetKeyAreas_Recursive(  const TArray< TSharedRef<FSequencerDisplayNode> >& Nodes, float& HeightOffset, TArray<FKeyAreaElement>& OutKeyAreaElements ) const;

	/**
	 * Computes the geometry for a key area
	 *
	 * @param KeyArea			The key area to compute geometry for
	 * @param SectionGeometry	The geometry of the section
	 * @return The geometry of the key area
	 */
	FGeometry GetKeyAreaGeometry( const FKeyAreaElement& KeyArea, const FGeometry& SectionGeometry ) const;

	/**
	 * Determines the key that is under the mouse
	 *
	 * @param MousePosition		The current screen space position of the mouse
	 * @param SectionGeometry	The geometry of the section
	 * @return The key that is under the mouse.  Invalid if there is no key under the mouse
	 */
	FSelectedKey GetKeyUnderMouse( const FVector2D& MousePosition, const FGeometry& SectionGeometry ) const;

	/**
	 * Checks for user interaction (via the mouse) with the left and right edge of a section
	 *
	 * @param MousePosition		The current screen space position of the mouse
	 * @param SectionGeometry	The geometry of the section
	 */
	void CheckForEdgeInteraction( const FPointerEvent& MousePosition, const FGeometry& SectionGeometry );

	/** SWidget interface */
	virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const OVERRIDE;
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual FReply OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual FReply OnMouseButtonDoubleClick( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual FReply OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual void OnMouseLeave( const FPointerEvent& MouseEvent ) OVERRIDE;
	virtual FCursorReply OnCursorQuery( const FGeometry& MyGeometry, const FPointerEvent& CursorEvent ) const OVERRIDE;

	/**
	 * Paints keys visible inside the section
	 *
	 * @param AllottedGeometry	The geometry of the section where keys are painted
	 * @param MyClippingRect	ClippingRect of the section
	 * @param OutDrawElements	List of draw elements to add to
	 * @param LayerId			The starting draw area
	 */
	void PaintKeys( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle ) const;

	/** Summons a context menu over the associated section */
	TSharedPtr<SWidget> OnSummonContextMenu( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent );

	/**
	 * Selects the section or keys if they are found under the mouse
	 *
	 * @param MyGeometry Geometry of the section
	 * @param MouseEvent	The mouse event that may cause selection
	 */
	void HandleSelection( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent );

	/**
	 * Selects a key if needed
	 *
	 * @param Key				The key to select
	 * @param MouseEvent		The mouse event that may have caused selection
	 * @param bSelectDueToDrag	If we are selecting the key due to a drag operation rather than it just being clicked
	 */
	void HandleKeySelection( const FSelectedKey& Key, const FPointerEvent& MouseEvent, bool bSelectDueToDrag );

	/**
	 * Selects the section if needed
	 *
	 * @param MouseEvent		The mouse event that may have caused selection
	 */
	void HandleSectionSelection( const FPointerEvent& MouseEvent );

	/** @return the sequencer interface */
	ISequencerInternals& GetSequencer() const;

	/**
	 * Creates a drag operation based on the current mouse event and what is selected
	 *
	 * @param MyGeometry		Geometry of the section
	 * @param MouseEvent		The mouse event that caused the drag
	 * @param bKeysUnderMouse	true if there are any keys under the mouse
	 */
	void CreateDragOperation( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, bool bKeysUnderMouse );

	/**
	 * Resets entire internal state of the widget
	 */
	void ResetState();

	/**
	 * Resets the hovered state of elements in the widget (section edges, keys)
	 */
	void ResetHoveredState();
private:
	/** Interface to section data */
	TSharedPtr<ISequencerSection> SectionInterface;
	/** Section area where this section resides */
	TSharedPtr<FTrackNode> ParentSectionArea;
	/** Key areas inside this section */
	TArray<FKeyAreaElement> KeyAreas;
	/** Current drag operation if any */
	TSharedPtr<class FSequencerDragOperation> DragOperation;
	/** The currently pressed key if any */
	FSelectedKey PressedKey;
	/** The current hovered key if any */
	FSelectedKey HoveredKey;
	/** The distance we have dragged since the mouse was pressed down */
	float DistanceDragged;
	/** The index of this section in the parent section area */
	int32 SectionIndex;
	/** Whether or not we are dragging */
	bool bDragging;
	/** Whether or not the left edge of the section is hovered */
	bool bLeftEdgeHovered;
	/** Whether or not the left edge of the section is pressed */
	bool bLeftEdgePressed;
	/** Whether or not the right edge of the section is hovered */
	bool bRightEdgeHovered;
	/** Whether or not the right edge of the section is pressed */
	bool bRightEdgePressed;
	/** Cached parent geometry to pass down to any section interfaces that need it during tick */
	FGeometry ParentGeometry;
};