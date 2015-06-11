// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "SSequencerTrackArea.h"
#include "ISequencerSection.h"
#include "MovieSceneSection.h"
#include "SSequencer.h"
#include "TimeSliderController.h"
#include "SSequencerSectionAreaView.h"

void SSequencerTrackArea::Construct( const FArguments& InArgs, TSharedRef<FSequencer> InSequencer )
{
	ViewRange = InArgs._ViewRange;
	OutlinerFillPercentage = InArgs._OutlinerFillPercentage;
	SectionFillPercentage = InArgs._SectionFillPercentage;
	OnOutlinerFillPercentageChanged = InArgs._OnOutlinerFillPercentageChanged;
	OnSectionFillPercentageChanged = InArgs._OnSectionFillPercentageChanged;
	Sequencer = InSequencer;

	ScrollbarFillPercentage.BindRaw(this, &SSequencerTrackArea::GetScrollBarFillPercentage);
	ScrollbarSpacerFillPercentage.BindRaw(this, &SSequencerTrackArea::GetScrollBarSpacerFillPercentage);

	TSharedRef<SScrollBar> ScrollBar =
		SNew(SScrollBar)
		.Thickness(FVector2D(5.0f, 5.0f));

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SAssignNew(ScrollBox, SScrollBox)
			.ExternalScrollbar(ScrollBar)
		]
		+ SOverlay::Slot()
		[
			SNew(SSplitter)
			.Style(FEditorStyle::Get(), "Sequencer.AnimationOutliner.Splitter")
			.Visibility(EVisibility::SelfHitTestInvisible)
			+ SSplitter::Slot()
			.Value(OutlinerFillPercentage)
			.OnSlotResized(SSplitter::FOnSlotResized::CreateSP(this, &SSequencerTrackArea::OutlinerFillPercentageChanged))
			[
				SNew(SSpacer)
			]
			+ SSplitter::Slot()
			.Value(SectionFillPercentage)
			.OnSlotResized(SSplitter::FOnSlotResized::CreateSP(this, &SSequencerTrackArea::SectionFillPercentageChanged))
			[
				SAssignNew(CurveEditor, SSequencerCurveEditor, InSequencer)
				.Visibility( this, &SSequencerTrackArea::GetCurveEditorVisibility )
				.ViewRange(ViewRange)
			]
		]
		+ SOverlay::Slot()
		[
			SNew(SSplitter)
			.Style(FEditorStyle::Get(), "Sequencer.AnimationOutliner.Splitter")
			.Visibility(EVisibility::SelfHitTestInvisible)
			+ SSplitter::Slot()
			.Value(ScrollbarFillPercentage)
			[
				SNew(SBox)
				.HAlign(HAlign_Right)
				[
					ScrollBar
				]
			]
			+ SSplitter::Slot()
			.Value(ScrollbarSpacerFillPercentage)
			[
				SNew( SSpacer )
			]
		]
	];
}

void SSequencerTrackArea::Update( TSharedPtr<FSequencerNodeTree> InSequencerNodeTree )
{
	ScrollBox->ClearChildren();

	const TArray< TSharedRef<FSequencerDisplayNode> >& RootNodes = InSequencerNodeTree->GetRootNodes();

	for( int32 RootNodeIndex = 0; RootNodeIndex < RootNodes.Num(); ++RootNodeIndex )
	{
		TSharedRef<FSequencerDisplayNode> RootNode = RootNodes[RootNodeIndex];

		// Generate a widget for each root node
		GenerateWidgetForNode( RootNode );

		// Generate a node for each children of the root node
		GenerateLayoutNodeWidgetsRecursive( RootNode->GetChildNodes() );
	}

	// @todo Sequencer - Remove this expensive operation
	ScrollBox->SlatePrepass();

	CurveEditor->SetSequencerNodeTree(InSequencerNodeTree);
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
		SNew( SSplitter )
		.Style(FEditorStyle::Get(), "Sequencer.AnimationOutliner.Splitter")
		+ SSplitter::Slot()
		.Value(OutlinerFillPercentage)
		.OnSlotResized(SSplitter::FOnSlotResized::CreateSP(this, &SSequencerTrackArea::OutlinerFillPercentageChanged))
		[
			SNew(SBox)
			.Padding(this, &SSequencerTrackArea::GetOutlinerPadding)
			[
				// Generate a widget for the animation outliner portion of the node
				Node->GenerateContainerWidgetForOutliner()
			]
		]
		+ SSplitter::Slot()
		.Value(SectionFillPercentage)
		.OnSlotResized(SSplitter::FOnSlotResized::CreateSP(this, &SSequencerTrackArea::SectionFillPercentageChanged))
		[
			SNew(SBox)
			.Visibility(this, &SSequencerTrackArea::GetSectionControlVisibility)
			[
				// Generate a widget for the section area portion of the node
				Node->GenerateWidgetForSectionArea( ViewRange )
			]
		]
	];
}

EVisibility SSequencerTrackArea::GetCurveEditorVisibility() const
{
	return GetDefault<USequencerSettings>()->GetShowCurveEditor() ? EVisibility::Visible : EVisibility::Hidden;
}

EVisibility SSequencerTrackArea::GetSectionControlVisibility() const
{
	return GetDefault<USequencerSettings>()->GetShowCurveEditor() ? EVisibility::Hidden : EVisibility::Visible;
}

float SSequencerTrackArea::GetScrollBarFillPercentage() const
{
	return GetDefault<USequencerSettings>()->GetShowCurveEditor() ? OutlinerFillPercentage.Get() : 1.0f;
}

float SSequencerTrackArea::GetScrollBarSpacerFillPercentage() const
{
	return GetDefault<USequencerSettings>()->GetShowCurveEditor() ? SectionFillPercentage.Get() : 0.0f;
}

FMargin SSequencerTrackArea::GetOutlinerPadding() const
{
	return GetDefault<USequencerSettings>()->GetShowCurveEditor() ? FMargin(0, 0, 10, SequencerLayoutConstants::NodePadding) : FMargin(0, 0, 0, SequencerLayoutConstants::NodePadding);
}

void SSequencerTrackArea::OutlinerFillPercentageChanged(float Value)
{
	OnOutlinerFillPercentageChanged.ExecuteIfBound(Value);
}

void SSequencerTrackArea::SectionFillPercentageChanged(float Value)
{
	OnSectionFillPercentageChanged.ExecuteIfBound(Value);
}
