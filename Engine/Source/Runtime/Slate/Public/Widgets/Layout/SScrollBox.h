// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** SScrollBox can scroll through an arbitrary number of widgets. */
class SLATE_API SScrollBox : public SCompoundWidget
{
public:
	/** A Slot that provides layout options for the contents of a scrollable box. */
	class SLATE_API FSlot : public TSupportsOneChildMixin<SWidget, FSlot>, public TSupportsContentPaddingMixin<FSlot>
	{
	public:
		FSlot()
			: HAlignment(HAlign_Fill)
		{

		}

		FSlot& HAlign( EHorizontalAlignment InHAlignment )
		{
			HAlignment = InHAlignment;
			return *this;
		}
		
		EHorizontalAlignment HAlignment;
	};

	SLATE_BEGIN_ARGS(SScrollBox)
		: _Style( &FCoreStyle::Get().GetWidgetStyle<FScrollBoxStyle>("ScrollBox") )
		, _ExternalScrollbar()
		, _OnUserScrolled()
		{}
		
		SLATE_SUPPORTS_SLOT( FSlot )

		/** Style used to draw this scrollbox */
		SLATE_STYLE_ARGUMENT( FScrollBoxStyle, Style )

		/** Custom scroll bar */
		SLATE_ARGUMENT( TSharedPtr<SScrollBar>, ExternalScrollbar )

		/** Called when the button is clicked */
		SLATE_EVENT(FOnUserScrolled, OnUserScrolled)

	SLATE_END_ARGS()


	/** @return a new slot. Slots contain children for SScrollBox */
	static FSlot& Slot();

	void Construct( const FArguments& InArgs );

	/** Adds a slot to SScrollBox */
	SScrollBox::FSlot& AddSlot();

	/** Removes a slot at the specified location */
	void RemoveSlot( const TSharedRef<SWidget>& WidgetToRemove );

	/** Removes all children from the box */
	void ClearChildren();

	/** @return Returns true if the user is currently interactively scrolling the view by holding
		        the right mouse button and dragging. */
	bool IsRightClickScrolling() const;

	float GetScrollOffset();

	void SetScrollOffset( float NewScrollOffset );

public:

	// SWidget interface
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override;
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual FReply OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual FReply OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual void OnMouseLeave( const FPointerEvent& MouseEvent ) override;
	virtual FReply OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual FReply OnDragDetected( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual FCursorReply OnCursorQuery( const FGeometry& MyGeometry, const FPointerEvent& CursorEvent ) const override;
	virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const override;
	// End of SWidget interface

private:

	/** Scroll offset that the user asked for. We will clamp it before actually scrolling there. */
	float DesiredScrollOffset;

	/**
	 * Scroll the view by ScrollAmount given its currently AllottedGeometry.
	 *
	 * @param AllottedGeometry  The geometry allotted for this SScrollBox by the parent
	 * @param ScrollAmount      
	 * @return Whether or not the scroll was fully handled
	 */
	bool ScrollBy( const FGeometry& AllottedGeometry, float ScrollAmount, bool InAnimateScroll = true );

	/** Invoked when the user scroll via the scrollbar */
	void ScrollBar_OnUserScrolled( float InScrollOffsetFraction );

	/** Does the user need a hint that they can scroll up? */
	FSlateColor GetTopShadowOpacity() const;
	
	/** Does the user need a hint that they can scroll down? */
	FSlateColor GetBottomShadowOpacity() const;

	TSharedPtr<class SScrollPanel> ScrollPanel;
	TSharedPtr<SScrollBar> ScrollBar;
	
	/** Are we actively scrolling right now */
	bool bIsScrolling;

	/** Should the current scrolling be animated or immediately jump to the desired scroll offer */
	bool bAnimateScroll;

	/** How much we scrolled while the rmb has been held */
	float AmountScrolledWhileRightMouseDown;

	/** Helper object to manage inertial scrolling */
	FInertialScrollManager InertialScrollManager;

	/**	The current position of the software cursor */
	FVector2D SoftwareCursorPosition;

	/**	Whether the software cursor should be drawn in the viewport */
	bool bShowSoftwareCursor;

	/** Fired when the user scrolls the scrollbox */
	FOnUserScrolled OnUserScrolled;
};


