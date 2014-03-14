// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GraphEditorCommon.h"
#include "SGraphNodeK2Base.h"
#include "SGraphNodeFormatText.h"
#include "KismetPins/SGraphPinExec.h"
#include "SGraphNodeK2Sequence.h"
#include "NodeFactory.h"

#include "ScopedTransaction.h"
#include "BlueprintEditorUtils.h"

//////////////////////////////////////////////////////////////////////////
// SGraphNodeFormatText

void SGraphNodeFormatText::Construct(const FArguments& InArgs, UK2Node_FormatText* InNode)
{
	this->GraphNode = InNode;

	this->SetCursor( EMouseCursor::CardinalCross );

	this->UpdateGraphNode();
}

void SGraphNodeFormatText::CreatePinWidgets()
{
	// Create Pin widgets for each of the pins, except for the default pin
	for (auto PinIt = GraphNode->Pins.CreateConstIterator(); PinIt; ++PinIt)
	{
		UEdGraphPin* CurrentPin = *PinIt;
		if ((!CurrentPin->bHidden))
		{
			TSharedPtr<SGraphPin> NewPin = FNodeFactory::CreatePinWidget(CurrentPin);
			check(NewPin.IsValid());
			NewPin->SetIsEditable(IsEditable);

			this->AddPin(NewPin.ToSharedRef());
		}
	}
}

void SGraphNodeFormatText::UpdateGraphNode()
{
	SGraphNodeK2Base::UpdateGraphNode();

	TSharedRef<SButton> AddPinButton = SNew(SButton)
		.ContentPadding(0.0f)
		.ButtonStyle( FEditorStyle::Get(), "NoBorder" )
		.OnClicked( this, &SGraphNodeFormatText::OnAddPin )
		.ToolTipText(NSLOCTEXT("FormatTextNode", "FormatTextNodeAddPinButton_Tooltip", "Adds an argument to the node"))
		.Visibility(this, &SGraphNodeFormatText::IsAddPinButtonVisible)
		[
			FAddPinButtonHelper::AddPinButtonContent(false)
		];	
		
	AddPinButton->SetCursor( EMouseCursor::Hand );

	//Add buttons to the format text node.
	LeftNodeBox->AddSlot()
	.AutoHeight()
	.VAlign(VAlign_Center)
	.Padding(10,10,10,4)
	[
		AddPinButton
	];
}

EVisibility SGraphNodeFormatText::IsAddPinButtonVisible() const
{
	EVisibility VisibilityState = EVisibility::Collapsed;
	if(NULL != Cast<UK2Node_FormatText>(GraphNode))
	{
		VisibilityState = FAddPinButtonHelper::IsAddPinButtonVisible(OwnerGraphPanelPtr);
		if(VisibilityState == EVisibility::Visible)
		{
			UK2Node_FormatText* FormatNode = CastChecked<UK2Node_FormatText>(GraphNode);
			VisibilityState = FormatNode->CanEditArguments()? EVisibility::Visible : EVisibility::Collapsed;
		}
	}
	return VisibilityState;
}

FReply SGraphNodeFormatText::OnAddPin()
{
	if(UK2Node_FormatText* FormatText = Cast<UK2Node_FormatText>(GraphNode))
	{
		FormatText->AddArgumentPin();
	}
	return FReply::Handled();
}