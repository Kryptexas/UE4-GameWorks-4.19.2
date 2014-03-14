// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "GraphEditorCommon.h"

UEdGraphPin* FGraphEditorDragDropAction::GetHoveredPin() const
{
	return HoveredPin.IsValid() ? HoveredPin->GetPinObj() : NULL;
}

UEdGraphNode* FGraphEditorDragDropAction::GetHoveredNode() const
{
	return HoveredNode.IsValid() ? HoveredNode->GetNodeObj() : NULL;
}

UEdGraph* FGraphEditorDragDropAction::GetHoveredGraph() const
{
	// Note: We always want to report a graph even when hovering over a node or pin;
	// the same is not true for nodes when hovering over a pin (at least right now)
	if (HoveredGraph.IsValid())
	{
		return HoveredGraph->GetGraphObj();
	}
	else if (UEdGraphNode* Node = GetHoveredNode())
	{
		return Node->GetGraph();
	}
	else if (UEdGraphPin* Pin = GetHoveredPin())
	{
		return Pin->GetOwningNode()->GetGraph();
	}
	
	return NULL;
}

void FGraphEditorDragDropAction::SetHoveredPin(const TSharedPtr<SGraphPin>& InPin)
{
	if (HoveredPin != InPin)
	{
		HoveredPin = InPin;
		HoverTargetChanged();
	}
}

void FGraphEditorDragDropAction::SetHoveredNode(const TSharedPtr<SGraphNode>& InNode)
{
	if (HoveredNode != InNode)
	{
		HoveredNode = InNode;
		HoverTargetChanged();
	}
}

void FGraphEditorDragDropAction::SetHoveredGraph(const TSharedPtr<SGraphPanel>& InGraph)
{
	if (HoveredGraph != InGraph)
	{
		HoveredGraph = InGraph;
		HoverTargetChanged();
	}
}

void FGraphEditorDragDropAction::SetHoveredCategoryName(const FString& InHoverCategoryName)
{
	if(HoveredCategoryName != InHoverCategoryName)
	{
		HoveredCategoryName = InHoverCategoryName;
		HoverTargetChanged();
	}
}

void FGraphEditorDragDropAction::SetHoveredAction(TSharedPtr<struct FEdGraphSchemaAction> Action)
{
	if(HoveredAction.Pin().Get() != Action.Get())
	{
		HoveredAction = Action;
		HoverTargetChanged();
	}
}


void FGraphEditorDragDropAction::Construct()
{
	// Create the drag-drop decorator window
	CursorDecoratorWindow = SWindow::MakeCursorDecorator();
	const bool bShowImmediately = false;
	FSlateApplication::Get().AddWindow(CursorDecoratorWindow.ToSharedRef(), bShowImmediately);

	HoverTargetChanged();
}

void FGraphEditorDragDropAction::SetFeedbackMessage(const TSharedPtr<SWidget>& Message)
{
	if (Message.IsValid())
	{
		CursorDecoratorWindow->ShowWindow();
		CursorDecoratorWindow->SetContent
		(
			SNew(SBorder)
			. BorderImage(FEditorStyle::GetBrush("Graph.ConnectorFeedback.Border"))
			[
				Message.ToSharedRef()
			]				
		);
	}
	else
	{
		CursorDecoratorWindow->HideWindow();
		CursorDecoratorWindow->SetContent(SNullWidget::NullWidget);
	}
}

void FGraphEditorDragDropAction::SetSimpleFeedbackMessage(const FSlateBrush* Icon, const FSlateColor& IconColor, const FString& Message)
{
	// Let the user know the status of making this connection.
	SetFeedbackMessage(
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(3.0f)
		[
			SNew(SImage) 
			.Image( Icon )
			.ColorAndOpacity( IconColor )
		]
		+SHorizontalBox::Slot()
		.AutoWidth()
		.MaxWidth(500)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock) 
			.Text( Message )
		]
	);
}

////////////////////////////////////////////////////////////

void FGraphSchemaActionDragDropAction::HoverTargetChanged()
{
	if(ActionNode.IsValid())
	{
		const FSlateBrush* StatusSymbol = FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.NewNode"));

		//Create feedback message with the function name.
		SetSimpleFeedbackMessage(StatusSymbol, FLinearColor::White, ActionNode->MenuDescription);
	}
}

FReply FGraphSchemaActionDragDropAction::DroppedOnPanel( const TSharedRef< SWidget >& Panel, FVector2D ScreenPosition, FVector2D GraphPosition, UEdGraph& Graph)
{
	if(ActionNode.IsValid())
	{
		TArray<UEdGraphPin*> DummyPins;
		ActionNode->PerformAction(&Graph, DummyPins, GraphPosition);

		return FReply::Handled();
	}
	return FReply::Unhandled();
}
