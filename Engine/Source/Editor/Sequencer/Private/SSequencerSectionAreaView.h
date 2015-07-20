// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class FSectionKeyAreaNode;
class FTrackNode;

/**
 * Visualizes a section area and its children                                        
 */
class SSequencerSectionAreaView : public SPanel
{
public:
	SLATE_BEGIN_ARGS( SSequencerSectionAreaView )
	{}		
		/** The view range of the section area */
		SLATE_ATTRIBUTE( TRange<float>, ViewRange )
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, TSharedRef<FSequencerDisplayNode> Node );

private:
	/** SWidget Interface */
	virtual int32 OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const override;

	/** SPanel Interface */
	virtual void OnArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const override;
	virtual FVector2D ComputeDesiredSize(float) const override;
	virtual FChildren* GetChildren() override { return &Children; }
	
	/**
	 * Returns the visibility of a section
	 *
	 * @param SectionObject The section to check for selection
	 * @return The visibility of the section
	 */
	EVisibility GetSectionVisibility( UMovieSceneSection* SectionObject ) const;

	/**
	 * Calculates a time to pixel converter from the allotted geometry
	 *
	 * @param AllottedGeometry	The geometry to use to convert from time to pixels or vice versa
	 * @return A converter used to convert between time and pixels 
	 */
	struct FTimeToPixel GetTimeToPixel( const FGeometry& AllottedGeometry ) const;

	/**
	 * Generates section widgets inside this section area
	 */
	void GenerateSectionWidgets();

	/** @return The sequencer interface */
	FSequencer& GetSequencer() const;
private:

	/** List of all sequencer drag operations going on  in this area */
	/** @todo Sequencer - This list may need to be on the section view itself for multi-select */
	TArray< TSharedPtr< FSequencerDragOperation > > DragOperations;
	/** The node containing the sections we are viewing/manipulating */
	TSharedPtr<FTrackNode> SectionAreaNode;
	/** The current view range */
	TAttribute< TRange<float> > ViewRange;
	/** Background brush of the section area */
	const FSlateBrush* BackgroundBrush;
	/** The distance we have dragged since the mouse was pressed down */
	float DistanceDragged;
	/** Whether or not we are dragging */
	bool bDragging;
	/** All the widgets in the panel */
	TSlotlessChildren<SSection> Children;
};
