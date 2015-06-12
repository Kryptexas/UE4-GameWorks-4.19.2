// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"

#include "SequencerTrackLaneFactory.h"
#include "SSequencerTrackArea.h"
#include "SSequencerTrackOutliner.h"
#include "SequencerSettings.h"

struct FWidgetSizeLinker
{
	FVector2D GetMinSize() const
	{
		TOptional<FVector2D> MinSize;

		for (auto Widget : Widgets)
		{
			auto Pinned = Widget.Pin();
			if (Pinned.IsValid())
			{
				FVector2D DesiredSize = Pinned->GetDesiredSize();
				if (MinSize.IsSet())
				{
					MinSize->X = FMath::Min(MinSize->X, DesiredSize.X);
				}
				else
				{
					MinSize = DesiredSize;
				}
			}
		}

		return MinSize.IsSet() ? MinSize.GetValue() : FVector2D::ZeroVector;
	}

	FVector2D GetMaxSize() const
	{
		TOptional<FVector2D> MaxSize;

		for (auto Widget : Widgets)
		{
			auto Pinned = Widget.Pin();
			if (Pinned.IsValid())
			{
				FVector2D DesiredSize = Pinned->GetDesiredSize();
				if (MaxSize.IsSet())
				{
					MaxSize->X = FMath::Max(MaxSize->X, DesiredSize.X);
				}
				else
				{
					MaxSize = DesiredSize;
				}
			}
		}

		return MaxSize.IsSet() ? MaxSize.GetValue() : FVector2D::ZeroVector;
	}

	TArray<TWeakPtr<SWidget>> Widgets;
};

FSequencerTrackLaneFactory::FSequencerTrackLaneFactory(
	TSharedRef<SSequencerTrackOutliner>	InTrackOutliner,
	TSharedRef<SSequencerTrackArea>		InTrackArea,
	TSharedRef<FSequencer>				InSequencer
	)
	: TrackOutliner( MoveTemp( InTrackOutliner ) )
	, TrackArea( MoveTemp( InTrackArea ) )
	, Sequencer( MoveTemp( InSequencer ) )
{

}

void FSequencerTrackLaneFactory::Repopulate(const FSequencerNodeTree& NodeTree)
{
	TrackArea->ClearChildren();
	TrackOutliner->ClearChildren();

	for ( TSharedRef<FSequencerDisplayNode> RootNode : NodeTree.GetRootNodes() )
	{
		GenerateWidgetForNode( *RootNode );
		GenerateLayoutNodeWidgetsRecursive( RootNode->GetChildNodes() );
	}
}

void FSequencerTrackLaneFactory::GenerateWidgetForNode( FSequencerDisplayNode& Node)
{
	TAttribute<FAnimatedRange> ViewRange;
	ViewRange.Bind(TAttribute<FAnimatedRange>::FGetter::CreateSP(Sequencer, &FSequencer::GetViewRange));

	TSharedRef<SWidget> OutlinerWidget = Node.GenerateContainerWidgetForOutliner();
	TSharedRef<SWidget> TrackWidget = Node.GenerateWidgetForSectionArea( FAnimatedRange::WrapAttribute(ViewRange) );

	// Link the height of the widets using a size linker
	FWidgetSizeLinker Link;
	Link.Widgets.Add(OutlinerWidget);
	Link.Widgets.Add(TrackWidget);

	auto GetMaxSize = [=]() -> float {
		return Link.GetMaxSize().Y;
	};
	auto IsVisible = []() -> EVisibility{
		return GetDefault<USequencerSettings>()->GetShowCurveEditor() ? EVisibility::Hidden : EVisibility::Visible;
	};

	TrackOutliner->AddSlot()
	.AutoHeight()
	[
		SNew(SBox)
		.HeightOverride_Lambda(GetMaxSize)
		[
			OutlinerWidget
		]
	];

	TrackArea->AddSlot()
	.AutoHeight()
	[
		SNew(SBox)
		.HeightOverride_Lambda(GetMaxSize)
		.Visibility_Lambda(IsVisible)
		[
			TrackWidget
		]
	];
}

void FSequencerTrackLaneFactory::GenerateLayoutNodeWidgetsRecursive( const TArray< TSharedRef<FSequencerDisplayNode> >& Nodes )
{
	for (auto& SharedNode : Nodes)
	{
		FSequencerDisplayNode& Node = *SharedNode;

		// The section area encompasses multiple logical child nodes so we do not generate children that arent section areas (they would not be top level nodes in that case)
		if( Node.GetType() == ESequencerNode::Track )
		{
			GenerateWidgetForNode( Node );
			GenerateLayoutNodeWidgetsRecursive( Node.GetChildNodes() );
		}
	}
}