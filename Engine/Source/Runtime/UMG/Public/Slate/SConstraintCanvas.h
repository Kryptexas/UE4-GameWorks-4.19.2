// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SConstraintCanvas.h: Declares the SConstraintCanvas class.
=============================================================================*/

#pragma once

/**
 * ConstraintCanvas is a layout widget that allows you to arbitrary position and size child widgets in a relative coordinate space
 */
class UMG_API SConstraintCanvas
	: public SPanel
{
public:

	/**
	 * ConstraintCanvas slots allow child widgets to be positioned and sized
	 */
	class FSlot
	{
	public:		
		FSlot& Offset( const TAttribute<FMargin>& InOffset )
		{
			OffsetAttr = InOffset;
			return *this;
		}

		FSlot& Anchors( const TAttribute<FAnchors>& InAnchors )
		{
			AnchorsAttr = InAnchors;
			return *this;
		}

		FSlot& Alignment(const TAttribute<FVector2D>& InAlignment)
		{
			AlignmentAttr = InAlignment;
			return *this;
		}

		FSlot& operator[]( TSharedRef<SWidget> InWidget )
		{
			Widget = InWidget;
			return *this;
		}

		FSlot& Expose( FSlot*& OutVarToInit )
		{
			OutVarToInit = this;
			return *this;
		}

		/** The child widget contained in this slot. */
		TSharedRef<SWidget> Widget;

		/** Offset */
		TAttribute<FMargin> OffsetAttr;

		/** Anchors */
		TAttribute<FAnchors> AnchorsAttr;

		/** Size */
		TAttribute<FVector2D> AlignmentAttr;

		/** Default values for a slot. */
		FSlot()
			: Widget( SNullWidget::NullWidget )
			, OffsetAttr( FMargin( 0, 0, 1, 1 ) )
			, AnchorsAttr( FAnchors( 0.5f, 0.5f ) )
			, AlignmentAttr( FVector2D( 0.5f, 0.5f ) )
		{ }
	};

	SLATE_BEGIN_ARGS( SConstraintCanvas )
		{
			_Visibility = EVisibility::SelfHitTestInvisible;
		}

		SLATE_SUPPORTS_SLOT( SConstraintCanvas::FSlot )

	SLATE_END_ARGS()

	/**
	 * Construct this widget
	 *
	 * @param	InArgs	The declaration data for this widget
	 */
	void Construct( const FArguments& InArgs );

	static FSlot& Slot()
	{
		return *(new FSlot());
	}

	/**
	 * Adds a content slot.
	 *
	 * @return The added slot.
	 */
	FSlot& AddSlot()
	{
		SConstraintCanvas::FSlot& NewSlot = SConstraintCanvas::Slot();
		this->Children.Add( &NewSlot );
		return NewSlot;
	}

	/**
	 * Removes a particular content slot.
	 *
	 * @param SlotWidget The widget in the slot to remove.
	 */
	int32 RemoveSlot( const TSharedRef<SWidget>& SlotWidget );

	/**
	 * Removes all slots from the panel.
	 */
	void ClearChildren();

public:

	// Begin SWidget overrides

	virtual void OnArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const OVERRIDE;
	
	virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const OVERRIDE;

	virtual FVector2D ComputeDesiredSize() const OVERRIDE;

	virtual FChildren* GetChildren() OVERRIDE;

	// End SWidget overrides

protected:

	/** The ConstraintCanvas widget's children. */
	TPanelChildren< FSlot > Children;
};
