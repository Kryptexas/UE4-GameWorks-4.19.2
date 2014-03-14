// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GraphEditorCommon.h"
#include "SGraphNodeK2Base.h"
#include "SGraphNodeSwitchStatement.h"
#include "KismetPins/SGraphPinExec.h"
#include "SGraphNodeK2Sequence.h"
#include "NodeFactory.h"

#include "ScopedTransaction.h"
#include "BlueprintEditorUtils.h"

//////////////////////////////////////////////////////////////////////////
// SGraphPinSwitchNodeDefaultCaseExec

class SGraphPinSwitchNodeDefaultCaseExec : public SGraphPinExec
{
public:
	SLATE_BEGIN_ARGS(SGraphPinSwitchNodeDefaultCaseExec)	{}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InPin)
	{
		SGraphPin::Construct(SGraphPin::FArguments().PinLabelStyle(FName("Graph.Node.DefaultPinName")), InPin);
	}
};

//////////////////////////////////////////////////////////////////////////
// SGraphNodeSwitchStatement

void SGraphNodeSwitchStatement::Construct(const FArguments& InArgs, UK2Node_Switch* InNode)
{
	this->GraphNode = InNode;

	this->SetCursor( EMouseCursor::CardinalCross );

	this->UpdateGraphNode();
}

void SGraphNodeSwitchStatement::CreatePinWidgets()
{
	UK2Node_Switch* SwitchNode = CastChecked<UK2Node_Switch>(GraphNode);
	UEdGraphPin* DefaultPin = SwitchNode->GetDefaultPin();

	// Create Pin widgets for each of the pins, except for the default pin
	for (auto PinIt = GraphNode->Pins.CreateConstIterator(); PinIt; ++PinIt)
	{
		UEdGraphPin* CurrentPin = *PinIt;
		if ((!CurrentPin->bHidden) && (CurrentPin != DefaultPin))
		{
			TSharedPtr<SGraphPin> NewPin = FNodeFactory::CreatePinWidget(CurrentPin);
			check(NewPin.IsValid());
			NewPin->SetIsEditable(IsEditable);

			this->AddPin(NewPin.ToSharedRef());
		}
	}

	// Handle the default pin
	if (DefaultPin != NULL)
	{
		// Create some padding
		RightNodeBox->AddSlot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			.Padding(1.0f)
			[
				SNew(SImage)
				.Image(FEditorStyle::GetBrush("Graph.Pin.DefaultPinSeparator"))
			];

		// Create the pin itself
		TSharedPtr<SGraphPin> NewPin = SNew(SGraphPinSwitchNodeDefaultCaseExec, DefaultPin);
		NewPin->SetIsEditable(IsEditable);

		this->AddPin(NewPin.ToSharedRef());
	}
}

void SGraphNodeSwitchStatement::UpdateGraphNode()
{
	SGraphNodeK2Base::UpdateGraphNode();

	TSharedRef<SButton> AddPinButton = SNew(SButton)
		.ContentPadding(0.0f)
		.ButtonStyle( FEditorStyle::Get(), "NoBorder" )
		.OnClicked( this, &SGraphNodeSwitchStatement::OnAddPin )
		.ToolTipText(NSLOCTEXT("SwitchStatementNode", "SwitchStatementNodeAddPinButton", "Add new pin"))
		.Visibility(this, &SGraphNodeSwitchStatement::IsAddPinButtonVisible)
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

EVisibility SGraphNodeSwitchStatement::IsAddPinButtonVisible() const
{
	if(NULL != Cast<UK2Node_SwitchInteger>(GraphNode))
	{
		return FAddPinButtonHelper::IsAddPinButtonVisible(OwnerGraphPanelPtr);
	}
	return EVisibility::Collapsed;
}

FReply SGraphNodeSwitchStatement::OnAddPin()
{
	if(UK2Node_SwitchInteger* SwitchNode = Cast<UK2Node_SwitchInteger>(GraphNode))
	{
		const FScopedTransaction Transaction( NSLOCTEXT("Kismet", "AddExecutionPin", "Add Execution Pin") );
		SwitchNode->Modify();

		SwitchNode->AddPinToSwitchNode();
		FBlueprintEditorUtils::MarkBlueprintAsModified(SwitchNode->GetBlueprint());

		UpdateGraphNode();
		GraphNode->GetGraph()->NotifyGraphChanged();
	}
	return FReply::Handled();
}