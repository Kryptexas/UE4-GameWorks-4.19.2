// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MaterialGraphNode_Base.cpp
=============================================================================*/

#include "UnrealEd.h"

/////////////////////////////////////////////////////
// UMaterialGraphNode_Base

UMaterialGraphNode_Base::UMaterialGraphNode_Base(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

UEdGraphPin* UMaterialGraphNode_Base::GetInputPin(int32 InputIndex) const
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

void UMaterialGraphNode_Base::GetInputPins(TArray<class UEdGraphPin*>& OutInputPins) const
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

UEdGraphPin* UMaterialGraphNode_Base::GetOutputPin(int32 OutputIndex) const
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

void UMaterialGraphNode_Base::GetOutputPins(TArray<class UEdGraphPin*>& OutOutputPins) const
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

void UMaterialGraphNode_Base::ReplaceNode(UMaterialGraphNode_Base* OldNode)
{
	check(OldNode != this);

	// Get Pins from node passed in
	TArray<UEdGraphPin*> OldInputPins;
	TArray<UEdGraphPin*> OldOutputPins;
	OldNode->GetInputPins(OldInputPins);
	OldNode->GetOutputPins(OldOutputPins);

	// Get our Input and Output pins
	TArray<UEdGraphPin*> NewInputPins;
	TArray<UEdGraphPin*> NewOutputPins;
	GetInputPins(NewInputPins);
	GetOutputPins(NewOutputPins);

	// Copy Inputs from old node
	for (int32 PinIndex = 0; PinIndex < OldInputPins.Num(); PinIndex++)
	{
		if (PinIndex < NewInputPins.Num())
		{
			ModifyAndCopyPersistentPinData(*NewInputPins[PinIndex], *OldInputPins[PinIndex]);
		}
	}

	// Copy Outputs from old node
	for (int32 PinIndex = 0; PinIndex < OldOutputPins.Num(); PinIndex++)
	{
		// If we can't find an equivalent output in this node, just use the first
		// The user will have to fix up any issues from the mismatch
		int32 FoundPinIndex = 0;

		// Try to find an equivalent output in this node
		for (int32 NewPinIndex = 0; NewPinIndex < NewOutputPins.Num(); NewPinIndex++)
		{
			if (OldOutputPins[PinIndex]->PinType == NewOutputPins[NewPinIndex]->PinType)
			{
				FoundPinIndex = NewPinIndex;
				break;
			}
		}
		
		if (FoundPinIndex < NewOutputPins.Num())
		{
			ModifyAndCopyPersistentPinData(*NewOutputPins[FoundPinIndex], *OldOutputPins[PinIndex]);
		}
	}

	// Break the original pin links
	for (int32 OldPinIndex = 0; OldPinIndex < OldNode->Pins.Num(); ++OldPinIndex)
	{
		UEdGraphPin* OldPin = OldNode->Pins[OldPinIndex];
		OldPin->Modify();
		OldPin->BreakAllPinLinks();
	}
}

void UMaterialGraphNode_Base::AllocateDefaultPins()
{
	check(Pins.Num() == 0);

	CreateInputPins();
	CreateOutputPins();
}

void UMaterialGraphNode_Base::ReconstructNode()
{
	Modify();

	// Break any links to 'orphan' pins
	for (int32 PinIndex = 0; PinIndex < Pins.Num(); ++PinIndex)
	{
		UEdGraphPin* Pin = Pins[PinIndex];
		TArray<class UEdGraphPin*>& LinkedToRef = Pin->LinkedTo;
		for (int32 LinkIdx=0; LinkIdx < LinkedToRef.Num(); LinkIdx++)
		{
			UEdGraphPin* OtherPin = LinkedToRef[LinkIdx];
			// If we are linked to a pin that its owner doesn't know about, break that link
			if (!OtherPin->GetOwningNode()->Pins.Contains(OtherPin))
			{
				Pin->LinkedTo.Remove(OtherPin);
			}
		}
	}

	// Store the old Input and Output pins
	TArray<UEdGraphPin*> OldInputPins;
	TArray<UEdGraphPin*> OldOutputPins;
	GetInputPins(OldInputPins);
	GetOutputPins(OldOutputPins);

	// Move the existing pins to a saved array
	TArray<UEdGraphPin*> OldPins(Pins);
	Pins.Empty();

	// Recreate the new pins
	AllocateDefaultPins();

	// Get new Input and Output pins
	TArray<UEdGraphPin*> NewInputPins;
	TArray<UEdGraphPin*> NewOutputPins;
	GetInputPins(NewInputPins);
	GetOutputPins(NewOutputPins);

	for (int32 PinIndex = 0; PinIndex < OldInputPins.Num(); PinIndex++)
	{
		if (PinIndex < NewInputPins.Num())
		{
			NewInputPins[PinIndex]->CopyPersistentDataFromOldPin(*OldInputPins[PinIndex]);
		}
	}

	for (int32 PinIndex = 0; PinIndex < OldOutputPins.Num(); PinIndex++)
	{
		if (PinIndex < NewOutputPins.Num())
		{
			NewOutputPins[PinIndex]->CopyPersistentDataFromOldPin(*OldOutputPins[PinIndex]);
		}
	}

	OldInputPins.Empty();
	OldOutputPins.Empty();

	// Throw away the original pins
	for (int32 OldPinIndex = 0; OldPinIndex < OldPins.Num(); ++OldPinIndex)
	{
		UEdGraphPin* OldPin = OldPins[OldPinIndex];
		OldPin->Modify();
		OldPin->BreakAllPinLinks();

#if 0
		UEdGraphNode::ReturnPinToPool(OldPin);
#else
		OldPin->Rename(NULL, GetTransientPackage(), REN_None);
		OldPin->RemoveFromRoot();
		OldPin->MarkPendingKill();
#endif
	}
	OldPins.Empty();

	GetGraph()->NotifyGraphChanged();
}

void UMaterialGraphNode_Base::AutowireNewNode(UEdGraphPin* FromPin)
{
	if (FromPin != NULL)
	{
		const UMaterialGraphSchema* Schema = CastChecked<UMaterialGraphSchema>(GetSchema());

		if (FromPin->Direction == EGPD_Input)
		{
			TArray<class UEdGraphPin*> OutputPins;
			GetOutputPins(OutputPins);
			if (OutputPins.Num() > 0)
			{
				Schema->TryCreateConnection(FromPin, OutputPins[0]);
			}
		}
		else
		{
			TArray<class UEdGraphPin*> InputPins;
			GetInputPins(InputPins);
			if (InputPins.Num() > 0)
			{
				Schema->TryCreateConnection(FromPin, InputPins[0]);
			}
		}
	}
}

bool UMaterialGraphNode_Base::CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const
{
	return Schema->IsA(UMaterialGraphSchema::StaticClass());
}

void UMaterialGraphNode_Base::ModifyAndCopyPersistentPinData(UEdGraphPin& TargetPin, const UEdGraphPin& SourcePin) const
{
	if (SourcePin.LinkedTo.Num() > 0)
	{
		TargetPin.Modify();

		for (int32 LinkIndex = 0; LinkIndex < SourcePin.LinkedTo.Num(); ++LinkIndex)
		{
			UEdGraphPin* OtherPin = SourcePin.LinkedTo[LinkIndex];
			OtherPin->Modify();
		}
	}

	TargetPin.CopyPersistentDataFromOldPin(SourcePin);
}
