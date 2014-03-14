// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "../../../Runtime/Engine/Classes/Kismet/KismetMathLibrary.h"

UK2Node_SwitchInteger::UK2Node_SwitchInteger(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	StartIndex = 0;

	FunctionName = TEXT("NotEqual_IntInt");
	FunctionClass = UKismetMathLibrary::StaticClass();
}

FString UK2Node_SwitchInteger::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("K2Node", "Switch_Interger", "Switch on Int").ToString();
}

FString UK2Node_SwitchInteger::GetTooltip() const
{
	return NSLOCTEXT("K2Node", "SwitchInt_ToolTip", "Selects an output that matches the input value").ToString();
}

void UK2Node_SwitchInteger::CreateSelectionPin()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	CreatePin(EGPD_Input, K2Schema->PC_Int, TEXT(""), NULL, false, false, TEXT("Selection"));
}

FString UK2Node_SwitchInteger::GetPinNameGivenIndex(int32 Index)
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	return FString::Printf(TEXT("%d"), Index);
}

FString UK2Node_SwitchInteger::GetUniquePinName()
{
	FString NewPinName;
	int32 i = StartIndex;
	while (true)
	{
		NewPinName = GetPinNameGivenIndex(i++);
		if (!FindPin(NewPinName))
		{
			break;
		}
	}
	return NewPinName;
}

void UK2Node_SwitchInteger::CreateCasePins()
{
	// Do nothing. By default nothing to do
	// and during realloc we will add new pins
}

void UK2Node_SwitchInteger::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	Super::ReallocatePinsDuringReconstruction(OldPins);

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	int32 ExecOutPinCount = StartIndex;

	UEdGraphPin* DefaultPin = GetDefaultPin();

	for (int32 i = bHasDefaultPin ? 1 : 0; i < OldPins.Num(); ++i)
	{
		UEdGraphPin* TestPin = OldPins[i];
		if (K2Schema->IsExecPin(*TestPin) && (TestPin->Direction == EGPD_Output))
		{
			// Skip the default pin to avoid creating an extra output pin in the case where the default pin has been toggled off
			if(TestPin->PinName != TEXT("Default"))
			{
				FString NewPinName = GetPinNameGivenIndex(ExecOutPinCount);
				ExecOutPinCount++;

				// Make sure the old pin and new pin names match
				TestPin->PinName = NewPinName;

				// Create the new output pin to match
				CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, NewPinName);
			}
		}
	}
}

void UK2Node_SwitchInteger::RemovePin(UEdGraphPin* /*TargetPin*/)
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	int32 PinIndex = (StartIndex >= 0) ? StartIndex : 0;

	UEdGraphPin* DefaultPin = GetDefaultPin();

	for (int32 i = 0; i < Pins.Num(); ++i)
	{
		UEdGraphPin* PotentialPin = Pins[i];
		if (K2Schema->IsExecPin(*PotentialPin) && (PotentialPin->Direction == EGPD_Output) && (PotentialPin != DefaultPin))
		{
			PotentialPin->PinName = GetPinNameGivenIndex(PinIndex);
			++PinIndex;
		}
	}
}

void UK2Node_SwitchInteger::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == TEXT("StartIndex"))
	{
		ReconstructNode();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
