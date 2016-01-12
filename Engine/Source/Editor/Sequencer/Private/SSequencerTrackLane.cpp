// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "SSequencerTrackLane.h"
#include "SSequencerTreeView.h"


void SSequencerTrackLane::Construct(const FArguments& InArgs, const TSharedRef<FSequencerDisplayNode>& InDisplayNode, const TSharedRef<SSequencerTreeView>& InTreeView)
{
	DisplayNode = InDisplayNode;
	TreeView = InTreeView;

	ChildSlot
		.HAlign(HAlign_Fill)
		.Padding(0, DisplayNode->GetNodePadding().Top, 0, 0)
		[
			InArgs._Content.Widget
		];
}


void DrawLaneRecursive(const TSharedRef<FSequencerDisplayNode>& DisplayNode, const FGeometry& AllottedGeometry, float& YOffs, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle)
{
	static const FName HoverBorderName("Sequencer.AnimationOutliner.SelectionBorderHover");
	static const FName SelectionBorderName("Sequencer.AnimationOutliner.SelectionBorder");
	static const FName SelectionBorderInactiveName("Sequencer.AnimationOutliner.SelectionBorderInactive");
	static const FName SelectionColorName("SelectionColor_Pressed");
	static const FName SelectionColorInactiveName("SelectionColor_Inactive");

	if (DisplayNode->IsHidden())
	{
		return;
	}

	float TotalNodeHeight = DisplayNode->GetNodeHeight() + DisplayNode->GetNodePadding().Combined();

	// draw selection border
	FSequencerSelection& SequencerSelection = DisplayNode->GetSequencer().GetSelection();

	if (SequencerSelection.IsSelected(DisplayNode))
	{
		FLinearColor SelectionColor = FEditorStyle::GetSlateColor(
			(SequencerSelection.GetActiveSelection() == FSequencerSelection::EActiveSelection::OutlinerNode)
				? SelectionColorName
				: SelectionColorInactiveName
		).GetColor(InWidgetStyle);

		FName SelectionBrush = (SequencerSelection.GetActiveSelection() == FSequencerSelection::EActiveSelection::OutlinerNode)
			? SelectionBorderName
			: SelectionBorderInactiveName;

		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(
				FVector2D(0, YOffs),
				FVector2D(AllottedGeometry.Size.X, TotalNodeHeight)
			),
			FEditorStyle::GetBrush(SelectionBrush),
			MyClippingRect,
			ESlateDrawEffect::None,
			SelectionColor
		);
	}

	// draw hover border
	else if (DisplayNode->IsHovered())
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(
				FVector2D(0, YOffs),
				FVector2D(AllottedGeometry.Size.X, TotalNodeHeight)
			),
			FEditorStyle::GetBrush(HoverBorderName),
			MyClippingRect,
			ESlateDrawEffect::None,
			FLinearColor(1.0f, 1.0f, 1.0f, 0.1f)
		);
	}

	YOffs += TotalNodeHeight;

/*	// draw lane separator
	static const FName LaneColorName("Sequencer.TrackArea.LaneColor");

	TArray<FVector2D> Points;
	{
		Points.Emplace(0, YOffs);
		Points.Emplace(PaintGeometry.GetLocalSize().X, YOffs);
	}

	FSlateDrawElement::MakeLines( 
		OutDrawElements, 
		LayerId + 1,
		AllottedGeometry.ToPaintGeometry(),
		Points,
		MyClippingRect,
		ESlateDrawEffect::None,
		FEditorStyle::GetSlateColor(LaneColorName).GetColor(InWidgetStyle),
		false //bAntialias
	);*/

	if (DisplayNode->IsExpanded())
	{
		for (const auto& ChildNode : DisplayNode->GetChildNodes())
		{
			DrawLaneRecursive(ChildNode, AllottedGeometry, YOffs, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle);
		}
	}
}


int32 SSequencerTrackLane::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	float YOffs = 0;
	DrawLaneRecursive(DisplayNode.ToSharedRef(), AllottedGeometry, YOffs, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle);

	return SCompoundWidget::OnPaint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId + 1, InWidgetStyle, bParentEnabled);
}


float SSequencerTrackLane::GetPhysicalPosition() const
{
	// Positioning strategy:
	// We know that there must be at least one tree view row around that references something in our subtree (otherwise this widget wouldn't exist)
	// Knowing this, we just need to root it out by traversing the visible tree, and asking for the first rows's offset
	float NegativeOffset = 0.f;
	TOptional<float> Top;

	auto TreeViewPinned = TreeView.Pin();
	
	// Iterate parent first until we find a tree view row we can use for the offset height
	auto Iter = [&](FSequencerDisplayNode& InNode){
		
		auto ChildRowGeometry = TreeViewPinned->GetPhysicalGeometryForNode(InNode.AsShared());
		if (ChildRowGeometry.IsSet())
		{
			Top = ChildRowGeometry->PhysicalTop;
			// Stop iterating
			return false;
		}

		NegativeOffset -= InNode.GetNodeHeight() + InNode.GetNodePadding().Combined();
		return true;
	};

	DisplayNode->TraverseVisible_ParentFirst(Iter);

	if (!Top.IsSet())
	{
		Top = 0.f;
	}

	return NegativeOffset + Top.GetValue();
}
