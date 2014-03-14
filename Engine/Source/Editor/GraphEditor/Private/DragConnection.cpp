// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "GraphEditorCommon.h"
#include "ScopedTransaction.h"

TSharedRef<FDragConnection> FDragConnection::New(const TSharedRef<SGraphPanel>& InGraphPanel, const TSharedRef<SGraphPin>& InStartingPin)
{
	TSharedRef<FDragConnection> Operation = MakeShareable(new FDragConnection);
	FSlateApplication::GetDragDropReflector().RegisterOperation<FDragConnection>(Operation);
	Operation->GraphPanel = InGraphPanel;
	Operation->StartingPins.Add(InStartingPin);

	UEdGraphPin* PinObj = InStartingPin->GetPinObj();

	// adjust the decorator away from the current mouse location a small amount based on cursor size
	Operation->DecoratorAdjust =  (PinObj->Direction == EGPD_Input)
		? FSlateApplication::Get().GetCursorSize() * FVector2D(-1.0f, 1.0f)
		: FSlateApplication::Get().GetCursorSize();

	Operation->Init(Operation->GraphPanel.ToSharedRef(), Operation->StartingPins);
	Operation->Construct();
	return Operation;
}

TSharedRef<FDragConnection> FDragConnection::New(const TSharedRef<SGraphPanel>& InGraphPanel, const TArray< TSharedRef<SGraphPin> >& InStartingPins, bool bIsMovingLinks)
{
	TSharedRef<FDragConnection> Operation = MakeShareable(new FDragConnection);
	FSlateApplication::GetDragDropReflector().RegisterOperation<FDragConnection>(Operation);
	Operation->GraphPanel = InGraphPanel;
	Operation->StartingPins = InStartingPins;
	Operation->DecoratorAdjust = FSlateApplication::Get().GetCursorSize();
	Operation->Init(Operation->GraphPanel.ToSharedRef(), Operation->StartingPins);
	Operation->Construct();
	return Operation;
}

void FDragConnection::OnDrop( bool bDropWasHandled, const FPointerEvent& MouseEvent )
{
	GraphPanel->OnStopMakingConnection();

	Super::OnDrop(bDropWasHandled, MouseEvent);
}

UEdGraphPin* FDragConnection::GetGraphPinForSPin(TSharedRef<SGraphPin>& SPin)
{
	return SPin->GetPinObj();
}

void FDragConnection::OnDragged(const class FDragDropEvent& DragDropEvent)
{
	FVector2D TargetPosition = DragDropEvent.GetScreenSpacePosition();

	// Reposition the info window wrt to the drag
	CursorDecoratorWindow->MoveWindowTo(DragDropEvent.GetScreenSpacePosition() + DecoratorAdjust);
	// Request the active panel to scroll if required
	GraphPanel->RequestDeferredPan(TargetPosition);
}

void FDragConnection::HoverTargetChanged()
{
	TArray<FPinConnectionResponse> UniqueMessages;

	UEdGraphPin* TargetPinObj = GetHoveredPin();

	if (TargetPinObj != NULL)
	{
		ValidateGraphPinList();

		// Check the schema for connection responses
		for (TArray< TSharedRef<SGraphPin> >::TIterator PinIterator(StartingPins); PinIterator; ++PinIterator)
		{
			UEdGraphPin* StartingPinObj = GetGraphPinForSPin(*PinIterator);
			if (TargetPinObj != StartingPinObj)
			{
				// The Graph object in which the pins reside.
				UEdGraph* GraphObj = StartingPinObj->GetOwningNode()->GetGraph();

				// Determine what the schema thinks about the wiring action
				const FPinConnectionResponse Response = GraphObj->GetSchema()->CanCreateConnection( StartingPinObj, TargetPinObj );

				if (Response.Response == ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW)
				{
					 TSharedPtr<SGraphNode> NodeWidget = TargetPinObj->GetOwningNode()->NodeWidget.Pin();
					 if (NodeWidget.IsValid())
					 {
						 NodeWidget->NotifyDisallowedPinConnection(StartingPinObj, TargetPinObj);
					 }
				}

				UniqueMessages.AddUnique(Response);
			}
		}
	}

	// Let the user know the status of dropping now
	if (UniqueMessages.Num() == 0)
	{
		// Display the place a new node icon, we're not over a valid pin
		SetSimpleFeedbackMessage(
			FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.NewNode")),
			FLinearColor::White,
			NSLOCTEXT("GraphEditor.Feedback", "PlaceNewNode", "Place a new node.").ToString());
	}
	else
	{
		// Take the unique responses and create visual feedback for it
		TSharedRef<SVerticalBox> FeedbackBox = SNew(SVerticalBox);
		for (auto ResponseIt = UniqueMessages.CreateConstIterator(); ResponseIt; ++ResponseIt)
		{
			// Determine the icon
			const FSlateBrush* StatusSymbol = NULL;

			switch (ResponseIt->Response)
			{
			case CONNECT_RESPONSE_MAKE:
			case CONNECT_RESPONSE_BREAK_OTHERS_A:
			case CONNECT_RESPONSE_BREAK_OTHERS_B:
			case CONNECT_RESPONSE_BREAK_OTHERS_AB:
				StatusSymbol = FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OK"));
				break;

			case CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE:
				StatusSymbol = FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.ViaCast"));
				break;

			case CONNECT_RESPONSE_DISALLOW:
			default:
				StatusSymbol = FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.Error"));
				break;
			}

			// Add a new message row
			FeedbackBox->AddSlot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(3.0f)
				[
					SNew(SImage) .Image( StatusSymbol )
				]
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock) .Text( ResponseIt->Message )
				]
			];
		}

		SetFeedbackMessage(FeedbackBox);
	}
}

void FDragConnection::Init( const TSharedRef<SGraphPanel>& InGraphPanel, const TArray< TSharedRef<SGraphPin> >& InStartingPins )
{
	for (TArray< TSharedRef<SGraphPin> >::TConstIterator PinIterator(InStartingPins); PinIterator; ++PinIterator)
	{
		InGraphPanel->OnBeginMakingConnection( *PinIterator );
	}
}

FReply FDragConnection::DroppedOnPin(FVector2D ScreenPosition, FVector2D GraphPosition)
{
	ValidateGraphPinList();

	const FScopedTransaction Transaction( NSLOCTEXT("UnrealEd", "GraphEd_CreateConnection", "Create Pin Link") );

	UEdGraphPin* PinA = GetHoveredPin();
	bool bError = false;
	TSet<UEdGraphNode*> NodeList;
	for (TArray< TSharedRef<SGraphPin> >::TIterator PinIterator(StartingPins); !bError && PinIterator; ++PinIterator)
	{
		UEdGraphPin* PinB = GetGraphPinForSPin(*PinIterator);

		if ((PinA != NULL) && (PinB != NULL))
		{
			UEdGraph* MyGraphObj = PinA->GetOwningNode()->GetGraph();

			if (MyGraphObj->GetSchema()->TryCreateConnection(PinA, PinB))
			{
				NodeList.Add(PinA->GetOwningNode());
				NodeList.Add(PinB->GetOwningNode());
			}
		}
		else
		{
			bError = true;
		}
	}

	// Send all nodes that received a new pin connection a notification
	for (auto It = NodeList.CreateConstIterator(); It; ++It)
	{
		UEdGraphNode* Node = (*It);
		Node->NodeConnectionListChanged();
	}

	if (bError)
	{
		return FReply::Unhandled();
	}

	return FReply::Handled();
}

FReply FDragConnection::DroppedOnPanel( const TSharedRef< SWidget >& Panel, FVector2D ScreenPosition, FVector2D GraphPosition, UEdGraph& Graph)
{
	ValidateGraphPinList();

	// Gather any source drag pins
	TArray<UEdGraphPin*> PinObjects;
	for (auto PinIt = StartingPins.CreateConstIterator(); PinIt; ++PinIt)
	{
		UEdGraphPin* GraphPin = (*PinIt)->GetPinObj();
		PinObjects.Add(GraphPin);
	}

	// Create a context menu
	TSharedPtr<SWidget> WidgetToFocus = GraphPanel->SummonContextMenu(ScreenPosition, GraphPosition, NULL, NULL, PinObjects);

	// Give the context menu focus
	return (WidgetToFocus.IsValid())
		? FReply::Handled().SetKeyboardFocus( WidgetToFocus.ToSharedRef(), EKeyboardFocusCause::SetDirectly )
		: FReply::Handled();
}


void FDragConnection::ValidateGraphPinList( )
{
	for (TArray< TSharedRef<SGraphPin> >::TIterator PinIterator(StartingPins); PinIterator; ++PinIterator)
	{
		UEdGraphPin* StartingPinObj = GetGraphPinForSPin(*PinIterator);

		if( StartingPinObj )
		{
			//Check whether the list contains updated pin object references by checking its outer class type
			if( StartingPinObj->GetOuter() == NULL || !StartingPinObj->GetOuter()->IsA( UEdGraphNode::StaticClass() ) )
			{
				//This pin object reference is old. So remove it from the list.
				TSharedRef<SGraphPin> PinPtr = *PinIterator;
				StartingPins.Remove( PinPtr );
			}
		}
	}
}

void FDragConnection::OnDragBegin( const TSharedRef<class SGraphPin>& InPin)
{
	if( !StartingPins.Contains( InPin ) )
	{
		StartingPins.Add( InPin );
		GraphPanel->OnBeginMakingConnection( InPin );
	}
}
