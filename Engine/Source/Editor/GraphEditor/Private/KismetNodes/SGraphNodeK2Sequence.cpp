// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GraphEditorCommon.h"
#include "SGraphNodeK2Base.h"
#include "SGraphNodeK2Sequence.h"


EVisibility FAddPinButtonHelper::IsAddPinButtonVisible(TWeakPtr<SGraphPanel> OwnerGraphPanelPtr)
{
	bool bIsHidden = false;
	auto OwnerGraphPanel = OwnerGraphPanelPtr.Pin();
	if(OwnerGraphPanel.IsValid())
	{
		bIsHidden |= (SGraphEditor::EPinVisibility::Pin_Show != OwnerGraphPanel->GetPinVisibility());
		bIsHidden |= (OwnerGraphPanel->GetCurrentLOD() <= EGraphRenderingLOD::LowDetail);
	}

	return bIsHidden ? EVisibility::Collapsed : EVisibility::Visible;
}

TSharedRef<SWidget> FAddPinButtonHelper::AddPinButtonContent(bool bRightSide/* = true*/)
{
	if(bRightSide)
	{
		return SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Left)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("SequencerNode", "SequencerNodeAddPinButton", "Add pin"))
				.ColorAndOpacity(FLinearColor::White)
			]

			+SHorizontalBox::Slot()
			.AutoWidth()
			. VAlign(VAlign_Center)
			. Padding( 7,0,0,0 )
			[
				SNew(SImage)
				.Image(FEditorStyle::GetBrush(TEXT("PropertyWindow.Button_AddToArray")))
			];
	}
	else
	{
		return SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.AutoWidth()
			. VAlign(VAlign_Center)
			. Padding( 0,0,7,0 )
			[
				SNew(SImage)
				.Image(FEditorStyle::GetBrush(TEXT("PropertyWindow.Button_AddToArray")))
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Left)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("SequencerNode", "SequencerNodeAddPinButton", "Add pin"))
				.ColorAndOpacity(FLinearColor::White)
			];
	}
}

void SGraphNodeK2Sequence::Construct( const FArguments& InArgs, UK2Node* InNode )
{
	GraphNode = InNode;

	SetCursor( EMouseCursor::CardinalCross );

	UpdateGraphNode();
}

EVisibility SGraphNodeK2Sequence::IsAddPinButtonVisible() const
{
	return FAddPinButtonHelper::IsAddPinButtonVisible(OwnerGraphPanelPtr);
}

void SGraphNodeK2Sequence::UpdateGraphNode()
{
	SGraphNodeK2Base::UpdateGraphNode();

	TSharedRef<SButton> AddPinButton = SNew(SButton)
		.ContentPadding(0.0f)
		.ButtonStyle( FEditorStyle::Get(), "NoBorder" )
		.OnClicked( this, &SGraphNodeK2Sequence::OnAddPin )
		.ToolTipText(NSLOCTEXT("SequencerNode", "SequencerNodeAddPinButton_ToolTip", "Add new pin"))
		.Visibility(this, &SGraphNodeK2Sequence::IsAddPinButtonVisible)
		[
			FAddPinButtonHelper::AddPinButtonContent()
		];	
		
	AddPinButton->SetCursor( EMouseCursor::Hand );

	//Add buttons to the sequence node.
	RightNodeBox->AddSlot()
	.AutoHeight()
	.VAlign(VAlign_Center)
	.Padding(10,10,10,4)
	[
		AddPinButton
	];
}

FReply SGraphNodeK2Sequence::OnAddPin()
{
	if (UK2Node_ExecutionSequence* SequenceNode = Cast<UK2Node_ExecutionSequence>(GraphNode))
	{
		SequenceNode->AddPinToExecutionNode();
		UpdateGraphNode();
		GraphNode->GetGraph()->NotifyGraphChanged();
	}
	else if (UK2Node_MakeArray* MakeArrayNode = Cast<UK2Node_MakeArray>(GraphNode))
	{
		MakeArrayNode->AddInputPin();
		UpdateGraphNode();
		GraphNode->GetGraph()->NotifyGraphChanged();
	}
	else if (UK2Node_CommutativeAssociativeBinaryOperator* OperatorNode = Cast<UK2Node_CommutativeAssociativeBinaryOperator>(GraphNode))
	{
		OperatorNode->AddInputPin();
		UpdateGraphNode();
		GraphNode->GetGraph()->NotifyGraphChanged();
	}

	return FReply::Handled();
}