// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * The area where tracks( rows of sections ) are displayed
 */
class SSequencerTrackArea : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnValueChanged, float)

	SLATE_BEGIN_ARGS( SSequencerTrackArea )
	{}
		/** The range of time being viewed */
		SLATE_ATTRIBUTE( TRange<float>, ViewRange )
		/** Percentage of horizontal space that the animation outliner takes up */
		SLATE_ATTRIBUTE( float, OutlinerFillPercentage )
		/** Percentage of horizontal space that the section area takes up. */
		SLATE_ATTRIBUTE( float, SectionFillPercentage )
		/** Called whenever the outliner fill percentage is changed by a splitter. */
		SLATE_EVENT( FOnValueChanged, OnOutlinerFillPercentageChanged )
		/** Called whenever the section fill percentage is changed by a splitter. */
		SLATE_EVENT( FOnValueChanged, OnSectionFillPercentageChanged )
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, TSharedRef<FSequencer> InSequencer );

	/**
	 * Updates the track area with new nodes
	 */
	virtual void Update( TSharedPtr<FSequencerNodeTree> InSequencerNodeTree );

	TSharedPtr<SSequencerCurveEditor> GetCurveEditor() { return CurveEditor; }

private:
	/**
	 * Generates a widget for the specified node and puts it into the scroll box
	 *
	 * @param Node	The node to generate a widget for
	 */
	void GenerateWidgetForNode( TSharedRef<FSequencerDisplayNode>& Node );
	
	/**
	 * Recursively generates widgets for each layout node
	 *
	 * @param Nodes to generate widgets for.  Will recurse into each node's children
	 */
	void GenerateLayoutNodeWidgetsRecursive( const TArray< TSharedRef<FSequencerDisplayNode> >& Nodes );

	/** Gets the visibility of the curve editor based on the the current settings. */
	EVisibility GetCurveEditorVisibility() const;
	/** Gets the visibility of the default section controls based on the current settings. */
	EVisibility GetSectionControlVisibility() const;

	/** Gets the fill value for the slot which contains the scrollbar.  This allows the scroll bar to move depending
	  * whether or not the curve editor is visible. */
	float GetScrollBarFillPercentage() const;
	/** Gets the fill value for the spacer slot which is to the right of the scrollbar.  This allows the scroll bar 
	  * to move depending whether or not the curve editor is visible. */
	float GetScrollBarSpacerFillPercentage() const;

	FMargin GetOutlinerPadding() const;

	/** Called whenever the outliner fill percentage is changed by a splitter. */
	void OutlinerFillPercentageChanged(float Value);
	/** Called whenever the section fill percentage is changed by a splitter. */
	void SectionFillPercentageChanged(float Value);

private:
	/** Scrollable area to display widgets */
	TSharedPtr<SScrollBox> ScrollBox;
	/** The current view range */
	TAttribute< TRange<float> > ViewRange;
	/** The fill percentage of the animation outliner */
	TAttribute<float> OutlinerFillPercentage;
	/** The fill percentage of the section area */
	TAttribute<float> SectionFillPercentage;
	/** Delegate to be called whenever the outliner fill percentage is changed */
	FOnValueChanged OnOutlinerFillPercentageChanged;
	/** Delegate to be called whenever the section fill percentage is changed */
	FOnValueChanged OnSectionFillPercentageChanged;
	/** Attribute representing the scrollbar fill percentage */
	TAttribute<float> ScrollbarFillPercentage;
	/** Attribute representing the spacer fill percentage which positions the scrollbar correctly. */
	TAttribute<float> ScrollbarSpacerFillPercentage;
	/** The main sequencer interface */
	TWeakPtr<FSequencer> Sequencer;
	/** The curve editor. */
	TSharedPtr<SSequencerCurveEditor> CurveEditor;
};
