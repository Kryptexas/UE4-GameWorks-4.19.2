// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class FVirtualTrackArea;
struct ISequencerHotspot;

class IEditToolDragOperation
{
public:
	virtual ~IEditToolDragOperation(){}

	/**
	 * Called to initiate a drag
	 *
	 * @param LocalMousePos		The current location of the mouse, relative to the top-left corner of the virtual track area
	 * @param VirtualTrackArea	A virtual track area that can be used for pixel->time conversions and hittesting
	 */
	virtual void OnBeginDrag(const FVector2D& LocalMousePos, const FVirtualTrackArea& VirtualTrackArea) = 0;

	/** Called when a drag has ended */
	virtual void OnEndDrag() = 0;

	/** Request the cursor for this drag operation */
	virtual FCursorReply GetCursor() const { return FCursorReply::Cursor( EMouseCursor::Default ); }

	/**
	 * Notification called when the mouse moves while dragging
	 *
	 * @param MouseEvent		The associated mouse event for dragging
	 * @param LocalMousePos		The current location of the mouse, relative to the top-left corner of the virtual track area
	 * @param VirtualTrackArea	A virtual track area that can be used for pixel->time conversions and hittesting
	 */
	virtual void OnDrag( const FPointerEvent& MouseEvent, const FVector2D& LocalMousePos, const FVirtualTrackArea& VirtualTrackArea ) = 0;
};

class ISequencerEditTool
{
public:
	virtual ~ISequencerEditTool() { }

	virtual FReply OnMouseButtonDown(SWidget& OwnerWidget, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) { return FReply::Unhandled(); }
	virtual FReply OnMouseButtonUp(SWidget& OwnerWidget, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) { return FReply::Unhandled(); }
	virtual FReply OnMouseMove(SWidget& OwnerWidget, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) { return FReply::Unhandled(); }
	virtual FReply OnMouseWheel(SWidget& OwnerWidget, const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) { return FReply::Unhandled(); }
	virtual void   OnMouseCaptureLost() { }
	virtual int32  OnPaint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId) const { return LayerId; }
	virtual void   Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) { }
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const { return FCursorReply::Unhandled(); }

	virtual ISequencer& GetSequencer() const = 0;

	virtual FName GetIdentifier() const = 0;

	/** Get the current active hotspot */
	TSharedPtr<ISequencerHotspot> GetHotspot() { return Hotspot; }

	/** Set the hotspot to something else */
	void SetHotspot(TSharedPtr<ISequencerHotspot> NewHotspot) { Hotspot = MoveTemp(NewHotspot); }

protected:

	/** The current hotspot that can be set from anywhere to initiate drags */
	TSharedPtr<ISequencerHotspot> Hotspot;
};