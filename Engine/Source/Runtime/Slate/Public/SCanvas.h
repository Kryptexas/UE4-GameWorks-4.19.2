// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Canvas is a layout widget that allows you to arbitrary position and size child widgets in a relative coordinate space
 */
class SLATE_API SCanvas : public SPanel
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
		{
		}
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

	FSlot& AddSlot()
	{
		SCanvas::FSlot& NewSlot = SCanvas::Slot();
		this->Children.Add( &NewSlot );
		return NewSlot;
	}

	/**
	 * Panels arrange their children in a space described by the AllottedGeometry parameter. The results of the arrangement
	 * should be returned by appending a FArrangedWidget pair for every child widget. See StackPanel for an example
	 *
	 * @param AllottedGeometry    The geometry allotted for this widget by its parent.
	 * @param ArrangedChildren    The array to which to add the WidgetGeometries that represent the arranged children.
	 */
	virtual void ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const OVERRIDE;
	
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
	virtual int32 OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const OVERRIDE;

	/**
	 * A Panel's desired size in the space required to arrange of its children on the screen while respecting all of
	 * the children's desired sizes and any layout-related options specified by the user. See StackPanel for an example.
	 *
	 * @return The desired size.
	 */
	virtual FVector2D ComputeDesiredSize() const OVERRIDE;

	/** @return  The children of a panel in a slot-agnostic way. */
	virtual FChildren* GetChildren() OVERRIDE;

	/** 
	 * Removes a slot from this box panel which contains the specified SWidget
	 *
	 * @param SlotWidget The widget to match when searching through the slots
	 * @returns The index in the children array where the slot was removed and -1 if no slot was found matching the widget
	 */
	int32 RemoveSlot( const TSharedRef<SWidget>& SlotWidget );

	/** Removes all slots from the panel */
	void ClearChildren();

protected:

	/** The canvas widget's children. */
	TPanelChildren< FSlot > Children;
};
