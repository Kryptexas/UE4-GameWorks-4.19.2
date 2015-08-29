// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class FSequencer;
class SSequencerTrackOutliner;
class SSequencerTrackArea;

/** Class responsible for creating widgets for the sequencer track outliner and track area */
class FSequencerTrackLaneFactory
{
public:

	/** Construct this facory */
	FSequencerTrackLaneFactory(
		TSharedRef<SSequencerTrackOutliner>	InTrackOutliner,
		TSharedRef<SSequencerTrackArea>		InTrackArea,
		TSharedRef<FSequencer>				InSequencer
		);

	/** Repopulate the supplied sequencer widgets with the specified node tree */
	void Repopulate(const FSequencerNodeTree& NodeTree);

private:

	/**
	 * Generates a widget for the specified node and puts it into the scroll box
	 *
	 * @param Node	The node to generate a widget for
	 */
	void GenerateWidgetForNode( FSequencerDisplayNode& Node );
	
	/**
	 * Recursively generates widgets for each layout node
	 *
	 * @param Nodes to generate widgets for.  Will recurse into each node's children
	 */
	void GenerateLayoutNodeWidgetsRecursive( const TArray< TSharedRef<FSequencerDisplayNode> >& Nodes );

private:
	/** Pointer to the sequencer track outliner */
	TSharedRef<SSequencerTrackOutliner>	TrackOutliner;
	/** Pointer to the sequencer track area */
	TSharedRef<SSequencerTrackArea>		TrackArea;
	/** Pointer to the sequencer itself */
	TSharedRef<FSequencer>				Sequencer;
};