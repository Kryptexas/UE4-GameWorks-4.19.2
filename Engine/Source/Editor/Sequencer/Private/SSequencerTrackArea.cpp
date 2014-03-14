// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "SSequencerTrackArea.h"
#include "ISequencerSection.h"
#include "MovieSceneSection.h"
#include "SSequencer.h"
#include "TimeSliderController.h"
#include "SSequencerSectionAreaView.h"


void SSequencerTrackArea::Update( const FSequencerNodeTree& InSequencerNodeTree )
{
	// @todo Sequencer: Currently need to recreate the scroll box to remove all its children
	ChildSlot
	[
		SAssignNew( ScrollBox, SScrollBox )
	];

	const TArray< TSharedRef<FSequencerDisplayNode> >& RootNodes = InSequencerNodeTree.GetRootNodes();

	for( int32 RootNodeIndex = 0; RootNodeIndex < RootNodes.Num(); ++RootNodeIndex )
	{
		TSharedRef<FSequencerDisplayNode> RootNode = RootNodes[RootNodeIndex];

		// Generate a widget for each root node
		GenerateWidgetForNode( RootNode );

		// Generate a node for each children of the root node
		GenerateLayoutNodeWidgetsRecursive( RootNode->GetChildNodes() );
	}
}

void SSequencerTrackArea::GenerateLayoutNodeWidgetsRecursive( const TArray< TSharedRef<FSequencerDisplayNode> >& Nodes )
{
	for( int32 NodeIndex = 0; NodeIndex < Nodes.Num(); ++NodeIndex )
	{
		TSharedRef<FSequencerDisplayNode> Node = Nodes[NodeIndex];

		// The section area encompasses multiple logical child nodes so we do not generate children that arent section areas (they would not be top level nodes in that case)
		if( Node->GetType() == ESequencerNode::Track )
		{
			GenerateWidgetForNode( Node );	

			GenerateLayoutNodeWidgetsRecursive( Node->GetChildNodes() );
		}
	}
}

void SSequencerTrackArea::GenerateWidgetForNode( TSharedRef<FSequencerDisplayNode>& Node )
{
	ScrollBox->AddSlot()
	[
		SNew( SHorizontalBox )
		+ SHorizontalBox::Slot()
		.FillWidth( OutlinerFillPercent )
		.Padding( FMargin(0.0f, 2.0f ) )
		[
			// Generate a widget for the animation outliner portion of the node
			Node->GenerateWidgetForOutliner( Sequencer.Pin().ToSharedRef() )
		]
		+ SHorizontalBox::Slot()
		.FillWidth( 1.0f )
		.Padding( FMargin(0.0f, 2.0f) )
		[
			// Generate a widget for the section area portion of the node
			Node->GenerateWidgetForSectionArea( ViewRange )
		]
	];
}
