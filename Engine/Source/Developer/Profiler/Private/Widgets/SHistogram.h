// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/*-----------------------------------------------------------------------------
	Type definitions
-----------------------------------------------------------------------------*/

/** Type definition for shared pointers to instances of FGraphDataSource. */
typedef TSharedPtr<class IHistogramDataSource> FHistogramDataSourcePtr;

/** Type definition for shared references to instances of FGraphDataSource. */
typedef TSharedRef<class IHistogramDataSource> FHistogramDataSourceRef;


/*-----------------------------------------------------------------------------
	Delegates
-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------
	Basic structures
-----------------------------------------------------------------------------*/
class FHistogramDescription
{
public:
	FHistogramDescription()
	{}

	/**
	 * Constructor
	 *
	 * @param InDataSource   - data source for the histogram
	 * @param InBinInterval  - interval for each bin
	 * @param InMinValue     - minimum value to use for the bins
	 * @param InMaxValue     - maximum value to use for the bins
	 * @param InBinNormalize - whether or not to normalize the bins to the total data count
	 *
	 */
	FHistogramDescription(const FHistogramDataSourceRef& InDataSource, float InBinInterval, float InMinValue, float InMaxValue, bool InBinNormalize = false)
		: HistogramDataSource(InDataSource)
		, Interval(InBinInterval)
		, MinValue(InMinValue)
		, MaxValue(InMaxValue)
		, Normalize(InBinNormalize)
	{
		BinCount = FPlatformMath::Ceil( (MaxValue - MinValue) / Interval )+1;	// +1 for data beyond the max
	}

	/** Retrieves the bin count */
	int32 GetBinCount() const
	{
		return BinCount;
	}

	/** Retrieves the count for the specified bin */
	int32 GetCount(int32 Bin) const
	{
		float MinVal = MinValue + Bin * Interval;
		float MaxVal = MinValue + (Bin+1) * Interval;
		return HistogramDataSource->GetCount(MinVal, MaxVal);
	}

	/** Retrieves the total count */
	int32 GetTotalCount() const
	{
		return HistogramDataSource->GetTotalCount();
	}

	/** Data source for the histogram */
	FHistogramDataSourcePtr HistogramDataSource;

	/** Bin interval */
	float Interval;

	/** Min value of the graph */
	float MinValue;

	/** Max value of the graph */
	float MaxValue;

	/** Normalize the bins */
	bool Normalize;

	int32 BinCount;
};

/*-----------------------------------------------------------------------------
	Declarations
-----------------------------------------------------------------------------*/

/** A custom widget used to display a histogram. */
class SHistogram : public SCompoundWidget
{
public:
	/** Default constructor. */
	SHistogram();

	/** Virtual destructor. */
	virtual ~SHistogram();

	SLATE_BEGIN_ARGS( SHistogram )
		: _Description()
		{}

		/** Analyzer reference */
		SLATE_ATTRIBUTE( FHistogramDescription, Description )

	SLATE_END_ARGS()

	/**
	 * Construct this widget
	 *
	 * @param	InArgs	The declaration data for this widget
	 */
	void Construct( const FArguments& InArgs );

	/**
	 * Ticks this widget.  Override in derived classes, but always call the parent implementation.
	 *
	 * @param  AllottedGeometry The space allotted for this widget
	 * @param  InCurrentTime  Current absolute real time
	 * @param  InDeltaTime  Real time passed since last tick
	 */
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;

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
	 * The system calls this method to notify the widget that a mouse button was pressed within it. This event is bubbled.
	 *
	 * @param MyGeometry The Geometry of the widget receiving the event
	 * @param MouseEvent Information about the input event
	 *
	 * @return Whether the event was handled along with possible requests for the system to take action.
	 */
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	
	/**
	 * The system calls this method to notify the widget that a mouse button was release within it. This event is bubbled.
	 *
	 * @param MyGeometry The Geometry of the widget receiving the event
	 * @param MouseEvent Information about the input event
	 *
	 * @return Whether the event was handled along with possible requests for the system to take action.
	 */
	virtual FReply OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	
	/**
	 * The system calls this method to notify the widget that a mouse moved within it. This event is bubbled.
	 *
	 * @param MyGeometry The Geometry of the widget receiving the event
	 * @param MouseEvent Information about the input event
	 *
	 * @return Whether the event was handled along with possible requests for the system to take action.
	 */
	virtual FReply OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	
	/**
	 * The system will use this event to notify a widget that the cursor has entered it. This event is NOT bubbled.
	 *
	 * @param MyGeometry The Geometry of the widget receiving the event
	 * @param MouseEvent Information about the input event
	 */
	virtual void OnMouseEnter( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;
	
	/**
	 * The system will use this event to notify a widget that the cursor has left it. This event is NOT bubbled.
	 *
	 * @param MouseEvent Information about the input event
	 */
	virtual void OnMouseLeave( const FPointerEvent& MouseEvent ) OVERRIDE;
	
	/**
	 * Called when the mouse wheel is spun. This event is bubbled.
	 *
	 * @param  MouseEvent  Mouse event
	 *
	 * @return  Returns whether the event was handled, along with other possible actions
	 */
	virtual FReply OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;

	/**
	 * Called when a mouse button is double clicked.  Override this in derived classes.
	 *
	 * @param  InMyGeometry  Widget geometry
	 * @param  InMouseEvent  Mouse button event
	 *
	 * @return  Returns whether the event was handled, along with other possible actions
	 */
	virtual FReply OnMouseButtonDoubleClick( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) OVERRIDE;

	/**
	 * Called when the system wants to know which cursor to display for this Widget.  This event is bubbled.
	 *
	 * @return  The cursor requested (can be None.)
	 */
	virtual FCursorReply OnCursorQuery( const FGeometry& MyGeometry, const FPointerEvent& CursorEvent ) const OVERRIDE;

	/** Restores default state for the data graph by removing all inner graphs. */
	void RestoreDefaultState();

	virtual FVector2D ComputeDesiredSize() const OVERRIDE
	{
		//return SCompoundWidget::ComputeDesiredSize();
		return FVector2D(16.0f,16.0f);
	}

protected:
	FHistogramDescription Description;
};