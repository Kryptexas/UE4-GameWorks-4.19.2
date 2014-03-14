// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BehaviorTreeEditorPrivatePCH.h"

UBehaviorTreeDecoratorGraphNode::UBehaviorTreeDecoratorGraphNode(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	bAllowModifingInputs = true;
}

UEdGraphPin* UBehaviorTreeDecoratorGraphNode::GetInputPin(int32 InputIndex) const
{
	check(InputIndex >= 0);

	for (int32 PinIndex = 0, FoundInputs = 0; PinIndex < Pins.Num(); PinIndex++)
	{
		if (Pins[PinIndex]->Direction == EGPD_Input)
		{
			if (InputIndex == FoundInputs)
			{
				return Pins[PinIndex];
			}
			else
			{
				FoundInputs++;
			}
		}
	}

	return NULL;
}

UEdGraphPin* UBehaviorTreeDecoratorGraphNode::GetOutputPin(int32 InputIndex) const
{
	check(InputIndex >= 0);

	for (int32 PinIndex = 0, FoundInputs = 0; PinIndex < Pins.Num(); PinIndex++)
	{
		if (Pins[PinIndex]->Direction == EGPD_Output)
		{
			if (InputIndex == FoundInputs)
			{
				return Pins[PinIndex];
			}
			else
			{
				FoundInputs++;
			}
		}
	}

	return NULL;
}

UBehaviorTreeDecoratorGraph* UBehaviorTreeDecoratorGraphNode::GetDecoratorGraph()
{
	return CastChecked<UBehaviorTreeDecoratorGraph>(GetGraph());
}

EBTDecoratorLogic::Type UBehaviorTreeDecoratorGraphNode::GetOperationType() const
{
	return EBTDecoratorLogic::Invalid;
}

void UBehaviorTreeDecoratorGraphNode::AutowireNewNode(UEdGraphPin* FromPin)
{
	Super::AutowireNewNode(FromPin);

	if (FromPin != NULL)
	{
		if (GetSchema()->TryCreateConnection(FromPin, FromPin->Direction == EGPD_Input ? GetOutputPin() : GetInputPin()))
		{
			FromPin->GetOwningNode()->NodeConnectionListChanged();
		}
	}
}

void UBehaviorTreeDecoratorGraphNode::NodeConnectionListChanged()
{
	const UBehaviorTreeDecoratorGraph* MyGraph = CastChecked<UBehaviorTreeDecoratorGraph>(GetGraph());
	
	UBehaviorTreeGraphNode_CompositeDecorator* CompositeDecorator = Cast<UBehaviorTreeGraphNode_CompositeDecorator>(MyGraph->GetOuter());
	CompositeDecorator->OnInnerGraphChanged();
}

bool UBehaviorTreeDecoratorGraphNode::CanCreateUnderSpecifiedSchema(const UEdGraphSchema* DesiredSchema) const
{
	return DesiredSchema->GetClass()->IsChildOf(UEdGraphSchema_BehaviorTreeDecorator::StaticClass());
}

