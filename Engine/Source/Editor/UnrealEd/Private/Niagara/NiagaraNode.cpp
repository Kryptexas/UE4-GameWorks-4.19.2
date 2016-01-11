// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

UNiagaraNode::UNiagaraNode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UNiagaraNode::AutowireNewNode(UEdGraphPin* FromPin)
{
	Super::AutowireNewNode(FromPin);

	if (FromPin != nullptr)
	{
		const UEdGraphSchema_Niagara* Schema = CastChecked<UEdGraphSchema_Niagara>(GetSchema());
		check(Schema);
		
		ENiagaraDataType FromType = Schema->GetPinType(FromPin);

		//Find first of this nodes pins with the right type and direction.
		UEdGraphPin* FirstPinOfSameType = NULL;
		EEdGraphPinDirection DesiredDirection = FromPin->Direction == EGPD_Output ? EGPD_Input : EGPD_Output;
		for (UEdGraphPin* Pin : Pins)
		{
			ENiagaraDataType ToType = Schema->GetPinType(Pin);
			if (FromType == ToType && Pin->Direction == DesiredDirection)
			{
				FirstPinOfSameType = Pin;
				break;
			}			
		}

		if (FirstPinOfSameType && GetSchema()->TryCreateConnection(FromPin, FirstPinOfSameType))
		{
			FromPin->GetOwningNode()->NodeConnectionListChanged();
		}
	}
}

const UNiagaraGraph* UNiagaraNode::GetNiagaraGraph()const
{
	return CastChecked<UNiagaraGraph>(GetGraph());
}

UNiagaraGraph* UNiagaraNode::GetNiagaraGraph()
{
	return CastChecked<UNiagaraGraph>(GetGraph());
}

UNiagaraScriptSource* UNiagaraNode::GetSource()const
{
	return GetNiagaraGraph()->GetSource();
}

UEdGraphPin* UNiagaraNode::GetInputPin(int32 InputIndex) const
{
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

void UNiagaraNode::GetInputPins(TArray<class UEdGraphPin*>& OutInputPins) const
{
	OutInputPins.Empty();

	for (int32 PinIndex = 0; PinIndex < Pins.Num(); PinIndex++)
	{
		if (Pins[PinIndex]->Direction == EGPD_Input)
		{
			OutInputPins.Add(Pins[PinIndex]);
		}
	}
}

UEdGraphPin* UNiagaraNode::GetOutputPin(int32 OutputIndex) const
{
	for (int32 PinIndex = 0, FoundOutputs = 0; PinIndex < Pins.Num(); PinIndex++)
	{
		if (Pins[PinIndex]->Direction == EGPD_Output)
		{
			if (OutputIndex == FoundOutputs)
			{
				return Pins[PinIndex];
			}
			else
			{
				FoundOutputs++;
			}
		}
	}

	return NULL;
}

void UNiagaraNode::GetOutputPins(TArray<class UEdGraphPin*>& OutOutputPins) const
{
	OutOutputPins.Empty();

	for (int32 PinIndex = 0; PinIndex < Pins.Num(); PinIndex++)
	{
		if (Pins[PinIndex]->Direction == EGPD_Output)
		{
			OutOutputPins.Add(Pins[PinIndex]);
		}
	}
}
