// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Attribute.h"
#include "Layout/Margin.h"
#include "Layout/Visibility.h"
#include "Types/SlateStructs.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "SlotBase.h"
#include "Widgets/SWidget.h"
#include "Layout/Children.h"
#include "Widgets/SPanel.h"
#include "Widgets/Input/SButton.h"

/**
 * A RadialPanel contains one child and describes how that child should be arranged on the screen.
 */
class SRadialPanel
	: public SPanel
{
public:

	/**
	 * A RadialPanel contains one RadialPanel child and describes how that
	 * child should be arranged on the screen.
	 */
	class FSlot : public TSlotBase<FSlot>
	{
	public:		
		/** Horizontal and Vertical Boxes inherit from FSlot */
		virtual ~FSlot(){}
		/**
		 * How much space this slot should occupy along panel's direction.
		 *   When SizeRule is SizeRule_Auto, the widget's DesiredSize will be used as the space required.
		 *   When SizeRule is SizeRule_Stretch, the available space will be distributed proportionately between
		 *   peer Widgets depending on the Value property. Available space is space remaining after all the
		 *   peers' SizeRule_Auto requirements have been satisfied.
		 */
		FSizeParam SizeParam;
		
		/** Horizontal positioning of child within the allocated slot */
		EHorizontalAlignment HAlignment;
		
		/** Vertical positioning of child within the allocated slot */
		EVerticalAlignment VAlignment;
		
		/** The padding to add around the child. */
		TAttribute<FMargin> SlotPadding;
		
		/** The max size that this slot can be (0 if no max) */
		TAttribute<float> MaxSize;
			
	protected:
		/** Default values for a slot. */
		FSlot()
			: TSlotBase<FSlot>()
			, SizeParam( FStretch(1) )
			, HAlignment( HAlign_Fill )
			, VAlignment( VAlign_Fill )
			, SlotPadding( FMargin(0) )
			, MaxSize( 0.0f )
		{ }
	};

public:

	/** Removes a slot from this box panel which contains the specified SWidget
	 *
	 * @param SlotWidget The widget to match when searching through the slots
	 * @returns The index in the children array where the slot was removed and -1 if no slot was found matching the widget
	 */
	int32 RemoveSlot( const TSharedRef<SWidget>& SlotWidget );

	/**
	 * Removes all children from the box.
	 */
	void ClearChildren();

public:

	// Begin SWidget overrides.

	virtual void OnArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const;
	
	virtual FVector2D ComputeDesiredSize(float) const;

	virtual FChildren* GetChildren();

	// End SWidget overrides.

protected:

	SRadialPanel();

	/** The Radial Panel's children. */
	TPanelChildren<FSlot> Children;

	/** The ratio to adjust the radial menu's radius by (relative to the default)*/
	TAttribute<float> RadiusRatioOverride;
};


/** A Radial Box Panel. See SRadialPanel for more info. */
class SRadialBox : public SRadialPanel
{
public:
	class FSlot : public SRadialPanel::FSlot
	{
		public:
		FSlot()
		: SRadialPanel::FSlot()
		{
		}

		FSlot& AutoWidth()
		{
			SizeParam = FAuto();
			return *this;
		}

		FSlot& MaxWidth( const TAttribute< float >& InMaxWidth )
		{
			MaxSize = InMaxWidth;
			return *this;
		}

		FSlot& FillWidth( const TAttribute< float >& StretchCoefficient )
		{
			SizeParam = FStretch( StretchCoefficient );
			return *this;
		}
		
		FSlot& Padding( float Uniform )
		{
			SlotPadding = FMargin(Uniform);
			return *this;
		}

		FSlot& Padding( float Horizontal, float Vertical )
		{
			SlotPadding = FMargin(Horizontal, Vertical);
			return *this;
		}

		FSlot& Padding( float Left, float Top, float Right, float Bottom )
		{
			SlotPadding = FMargin(Left, Top, Right, Bottom);
			return *this;
		}
		
		FSlot& Padding( TAttribute<FMargin> InPadding )
		{
			SlotPadding = InPadding;
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

		FSlot& operator[]( TSharedRef<SWidget> InWidget )
		{
			SRadialPanel::FSlot::operator[](InWidget);
			return *this;
		}

		FSlot& Expose( FSlot*& OutVarToInit )
		{
			OutVarToInit = this;
			return *this;
		}
	};

	static FSlot& Slot()
	{
		return *(new FSlot());
	}

	SLATE_BEGIN_ARGS( SRadialBox )
	{
		_Visibility = EVisibility::SelfHitTestInvisible,
		/** The ratio to adjust the radial menu's radius by (relative to the default) */
		_RadiusRatio = 1.0f;
	}
	SLATE_ARGUMENT( float, RadiusRatio )
	SLATE_SUPPORTS_SLOT(SRadialBox::FSlot)

	SLATE_END_ARGS()

	FSlot& AddSlot()
	{
		SRadialBox::FSlot& NewSlot = *new SRadialBox::FSlot();
		this->Children.Add( &NewSlot );

		Invalidate(EInvalidateWidget::Layout);

		return NewSlot;
	}

	FSlot& InsertSlot(int32 Index = INDEX_NONE)
	{
		if (Index == INDEX_NONE)
		{
			return AddSlot();
		}
		SRadialBox::FSlot& NewSlot = *new SRadialBox::FSlot();
		this->Children.Insert(&NewSlot, Index);

		Invalidate(EInvalidateWidget::Layout);

		return NewSlot;
	}

	int32 NumSlots() const
	{
		return this->Children.Num();
	}

	int32 NumEntries;

	FORCENOINLINE SRadialBox()
	: SRadialPanel()
	{
		bCanTick = false;
		bCanSupportFocus = false;
	}

	/**
	 * Construct this widget
	 *
	 * @param	InArgs	The declaration data for this widget
	 */
	void Construct( const FArguments& InArgs );


	/**
	* Highlight the widget in a slot based on a given trackpad position
	*
	* @param	TrackpadPosition	The current trackpad or thumbstick position
	*/
	void HighlightSlot(FVector2D& TrackpadPosition);

	/** Simulate a left-mouse click (down) on the currently hovered button */
	void SimulateLeftClickDown();

	/** Simulate a left-mouse click (up) on the currently hovered button */
	void SimulateLeftClickUp();

	const TSharedPtr<SButton>& GetCurrentlyHoveredButton();

	private:
	/** Stores the currently hovered button */
	TSharedPtr<SButton> CurrentlyHoveredButton;
};
