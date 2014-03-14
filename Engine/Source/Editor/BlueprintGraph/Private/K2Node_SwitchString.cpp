// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "../../../Runtime/Engine/Classes/Kismet/KismetMathLibrary.h"

UK2Node_SwitchString::UK2Node_SwitchString(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

	bIsCaseSensitive = false;
	FunctionName = TEXT("NotEqual_StrStr");
	FunctionClass = UKismetStringLibrary::StaticClass();
}

void UK2Node_SwitchString::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	bool bIsDirty = false;
	FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == TEXT("PinNames"))
	{
		bIsDirty = true;
	}
	else if (PropertyName == TEXT("bIsCaseSensitive"))
	{
		FunctionName = (bIsCaseSensitive == true)
			?  TEXT("NotEqual_StrStr")
			: TEXT("NotEqual_StriStri");

		FunctionClass = UKismetStringLibrary::StaticClass();
		bIsDirty = true;
	}
	 
	if (bIsDirty)
	{
		ReconstructNode();
		GetGraph()->NotifyGraphChanged();
	}
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

FString UK2Node_SwitchString::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("K2Node", "Switch_String", "Switch on String").ToString();
}

FString UK2Node_SwitchString::GetTooltip() const
{
	return NSLOCTEXT("K2Node", "SwitchString_ToolTip", "Selects an output that matches the input value").ToString();
}

void UK2Node_SwitchString::CreateSelectionPin()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	CreatePin(EGPD_Input, K2Schema->PC_String, TEXT(""), NULL, false, false, TEXT("Selection"));
}

FString UK2Node_SwitchString::GetPinNameGivenIndex(int32 Index)
{
	check(Index);
	return PinNames[Index];
}


void UK2Node_SwitchString::CreateCasePins()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	for( TArray<FString>::TIterator it(PinNames); it; ++it )
	{
		CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, *it);
	}
}

FString UK2Node_SwitchString::GetUniquePinName()
{
	FString NewPinName;
	int32 Index = 0;
	while (true)
	{
		NewPinName =  FString::Printf(TEXT("Case_%d"), Index++);
		if (!FindPin(NewPinName))
		{
			break;
		}
	}
	return NewPinName;
}

void UK2Node_SwitchString::AddPinToSwitchNode()
{
	FString PinName = GetUniquePinName();
	PinNames.Add(PinName);

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, PinName);
}

void UK2Node_SwitchString::RemovePin(UEdGraphPin* TargetPin)
{
	checkSlow(TargetPin);

	// Clean-up pin name array
	PinNames.Remove(TargetPin->PinName);
}
