// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SCanvas.h: Declares the SCanvas class.
=============================================================================*/

#pragma once


/**
 * Canvas is a layout widget that allows you to arbitrary position and size child widgets in a relative coordinate space
 */
class SLATE_API SCanvas
	: public SPanel
{
public:

	/**
	 * Canvas slots allow child widgets to be positioned and sized
	 */
	class FSlot
	{
	public:		
		FSlot& Position( const TAttribute<FVector2D>& InPosition )
		{
			PositionAttr = InPosition;
			return *this;
		}

		FSlot& Size( const TAttribute<FVector2D>& InSize )
		{
			SizeAttr = InSize;
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

		FSlot& HAlign( EHorizontalAlignment InHAlignment )
		{
			HAlignment = InHAlignment;
			return *this;
		}

		FSlot& VAlign( EVerticalAlignment InVAlignment )
		{
			VAlignment = InVAlignment;
			return *this;
		}

		/** The child widget contained in this slot. */
		TSharedRef<SWidget> Widget;

		/** Position */
		TAttribute<FVector2D> PositionAttr;

		/** Size */
		TAttribute<FVector2D> SizeAttr;

		/** Horizontal Alignment 
		*  Given a top aligned slot, where '+' represents the 
		*  anchor point defined by PositionAttr.
		*  
		*   Left				Center				Right
			+ _ _ _ _            _ _ + _ _          _ _ _ _ +
			|		  |		   | 		   |	  |		    |
			| _ _ _ _ |        | _ _ _ _ _ |	  | _ _ _ _ |
		* 
		*  Note: FILL is NOT supported.
		*/
		EHorizontalAlignment HAlignment;

		/** Vertical Alignment 
		*   Given a left aligned slot, where '+' represents the 
		*   anchor point defined by PositionAttr.
		*  
		*   Top					Center			  Bottom
		*	+_ _ _ _ _          _ _ _ _ _		 _ _ _ _ _ 
		*	|		  |		   | 		 |		|		  |
		*	| _ _ _ _ |        +		 |		|		  |
		*					   | _ _ _ _ |		+ _ _ _ _ |
		* 
		*  Note: FILL is NOT supported.
		*/
		EVerticalAlignment VAlignment;


		/** Default values for a slot. */
		FSlot()
			: Widget( SNullWidget::NullWidget )
			, PositionAttr( FVector2D::ZeroVector )
			, SizeAttr( FVector2D( 1.0f, 1.0f ) )
			, HAlignment(HAlign_Left)
			, VAlignment(VAlign_Top)
		{ }
	};

	SLATE_BEGIN_ARGS( SCanvas )
		{
			_Visibility = EVisibility::SelfHitTestInvisible;
		}

		SLATE_SUPPORTS_SLOT( SCanvas::FSlot )

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
		SCanvas::FSlot& NewSlot = SCanvas::Slot();
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

	virtual void ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const OVERRIDE;
	
	virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const OVERRIDE;

	virtual FVector2D ComputeDesiredSize() const OVERRIDE;

	virtual FChildren* GetChildren() OVERRIDE;

	// End SWidget overrides

protected:

	/** The canvas widget's children. */
	TPanelChildren< FSlot > Children;
};
