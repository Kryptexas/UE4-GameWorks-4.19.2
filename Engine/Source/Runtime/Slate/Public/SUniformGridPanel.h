// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** A panel that evenly divides up available space between all of its children. */
class SLATE_API SUniformGridPanel : public SPanel
{
public:
	/** Stores the per-child info for this panel type */
	struct FSlot : public TSupportsOneChildMixin<SWidget, FSlot>, public TSupportsContentAlignmentMixin<FSlot>
	{
		FSlot( int32 InColumn, int32 InRow )
		: TSupportsContentAlignmentMixin<FSlot>(HAlign_Fill, VAlign_Fill)
		, Column( InColumn )
		, Row( InRow )
		{
		}

		int32 Column;
		int32 Row;
	};

	/**
	 * Used by declarative syntax to create a Slot in the specified Column, Row.
	 */
	static FSlot& Slot( int32 Column, int32 Row )
	{
		return *(new FSlot( Column, Row ));
	}

	SLATE_BEGIN_ARGS( SUniformGridPanel )
		: _SlotPadding( FMargin(0.0f) )
		, _MinDesiredSlotWidth( 0.0f )
		, _MinDesiredSlotHeight( 0.0f )
		{
			_Visibility = EVisibility::SelfHitTestInvisible;
		}

		/** Slot type supported by this panel */
		SLATE_SUPPORTS_SLOT(FSlot)
		
		/** Padding given to each slot */
		SLATE_ATTRIBUTE(FMargin, SlotPadding)

		/** The minimum desired width of the slots */
		SLATE_ATTRIBUTE(float, MinDesiredSlotWidth)

		/** The minimum desired height of the slots */
		SLATE_ATTRIBUTE(float, MinDesiredSlotHeight)

	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs );

	// BEGIN SPanel INTERFACE	
	virtual void ArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const OVERRIDE;
	virtual FVector2D ComputeDesiredSize() const OVERRIDE;
	virtual FChildren* GetChildren() OVERRIDE;
	// END SPanel INTERFACE

	/**
	 * Dynamically add a new slot to the UI at specified Column and Row.
	 *
	 * @return A reference to the newly-added slot
	 */
	FSlot& AddSlot( int32 Column, int32 Row );

	/** Removes all slots from the panel */
	void ClearChildren();

private:
	TPanelChildren<FSlot> Children;
	TAttribute<FMargin> SlotPadding;
	int32 NumColumns;
	int32 NumRows;
	TAttribute<float> MinDesiredSlotWidth;
	TAttribute<float> MinDesiredSlotHeight;
};