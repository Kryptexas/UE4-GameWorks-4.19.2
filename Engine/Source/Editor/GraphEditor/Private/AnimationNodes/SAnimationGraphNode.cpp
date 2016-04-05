// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "GraphEditorCommon.h"
#include "SAnimationGraphNode.h"
#include "AnimGraphNode_Base.h"
#include "IDocumentation.h"

void SAnimationGraphNode::Construct(const FArguments& InArgs, UAnimGraphNode_Base* InNode)
{
	this->GraphNode = InNode;

	this->SetCursor(EMouseCursor::CardinalCross);

	this->UpdateGraphNode();

	const FSlateBrush* ImageBrush = FEditorStyle::Get().GetBrush(TEXT("Graph.AnimationFastPathIndicator"));

	IndicatorWidget =
		SNew(SImage)
		.Image(ImageBrush)
		.ToolTip(IDocumentation::Get()->CreateToolTip(NSLOCTEXT("AnimationGraphNode", "AnimGraphNodeIndicatorTooltip", "Fast path enabled: This node is not using any Blueprint calls to update its data."), NULL, TEXT("Shared/GraphNodes/Animation"), TEXT("GraphNode_FastPathInfo")))
		.Visibility(EVisibility::Visible);
}

TArray<FOverlayWidgetInfo> SAnimationGraphNode::GetOverlayWidgets(bool bSelected, const FVector2D& WidgetSize) const
{
	TArray<FOverlayWidgetInfo> Widgets;

	if (GraphNode != nullptr && CastChecked<UAnimGraphNode_Base>(GraphNode)->BlueprintUsage == EBlueprintUsage::DoesNotUseBlueprint)
	{
		const FSlateBrush* ImageBrush = FEditorStyle::Get().GetBrush(TEXT("Graph.AnimationFastPathIndicator"));

		FOverlayWidgetInfo Info;
		Info.OverlayOffset = FVector2D(WidgetSize.X - (ImageBrush->ImageSize.X * 0.5f), -(ImageBrush->ImageSize.Y * 0.5f));
		Info.Widget = IndicatorWidget;

		Widgets.Add(Info);
	}

	return Widgets;
}