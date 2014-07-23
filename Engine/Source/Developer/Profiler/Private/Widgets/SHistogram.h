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
		BinCount = FPlatformMath::CeilToInt( (MaxValue - MinValue) / Interval )+1;	// +1 for data beyond the max
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

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override;

	virtual int32 OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const override;

	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	
	virtual FReply OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	
	virtual FReply OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	
	virtual void OnMouseEnter( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	
	virtual void OnMouseLeave( const FPointerEvent& MouseEvent ) override;
	
	virtual FReply OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;

	virtual FReply OnMouseButtonDoubleClick( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;

	virtual FCursorReply OnCursorQuery( const FGeometry& MyGeometry, const FPointerEvent& CursorEvent ) const override;

	/** Restores default state for the data graph by removing all inner graphs. */
	void RestoreDefaultState();

	virtual FVector2D ComputeDesiredSize() const override
	{
		//return SCompoundWidget::ComputeDesiredSize();
		return FVector2D(16.0f,16.0f);
	}

protected:
	FHistogramDescription Description;
};