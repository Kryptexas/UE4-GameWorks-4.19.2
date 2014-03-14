// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "../../../Runtime/Engine/Classes/Kismet/KismetMathLibrary.h"

UK2Node_SwitchEnum::UK2Node_SwitchEnum(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

	bHasDefaultPin = false;

	FunctionName = TEXT("NotEqual_ByteByte");
	FunctionClass = UKismetMathLibrary::StaticClass();
}

void UK2Node_SwitchEnum::SetEnum(UEnum* InEnum)
{
	Enum = InEnum;

	// regenerate enum name list
	EnumEntries.Empty();
	EnumFriendlyNames.Empty();

	if (Enum)
	{
		PreloadObject(Enum);
		for (int32 EnumIndex = 0; EnumIndex < Enum->NumEnums() - 1; ++EnumIndex)
		{
			bool const bShouldBeHidden = Enum->HasMetaData(TEXT("Hidden"), EnumIndex ) || Enum->HasMetaData(TEXT("Spacer"), EnumIndex );
			if (!bShouldBeHidden)
			{
				FString const EnumValueName = Enum->GetEnumName(EnumIndex);
				EnumEntries.Add( FName(*EnumValueName) );

				FString EnumFriendlyName = Enum->GetDisplayNameText(EnumIndex).ToString();
				if (EnumFriendlyName.Len() == 0)
				{
					EnumFriendlyName = EnumValueName;
				}
				EnumFriendlyNames.Add( EnumFriendlyName );
			}
		}
	}
}

FString UK2Node_SwitchEnum::GetNodeTitle(ENodeTitleType::Type TitleType) const 
{
	FString EnumName = (Enum != NULL) ? Enum->GetName() : TEXT("(bad enum)");

	return FString::Printf(*NSLOCTEXT("K2Node", "Switch_Enum", "Switch on %s").ToString(), *EnumName);
}

FString UK2Node_SwitchEnum::GetTooltip() const
{
	return NSLOCTEXT("K2Node", "SwitchEnum_ToolTip", "Selects an output that matches the input value").ToString();
}

void UK2Node_SwitchEnum::CreateSelectionPin()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	CreatePin(EGPD_Input, K2Schema->PC_Byte, TEXT(""), NULL, false, false, TEXT("Selection"));
}

FString UK2Node_SwitchEnum::GetUniquePinName()
{
	FString NewPinName;
	for (auto EnumIt = EnumEntries.CreateConstIterator(); EnumIt; ++EnumIt)
	{
		FName EnumEntry = *EnumIt;
		if ( !FindPin(EnumEntry.ToString()) )
		{
			break;
		}
	}
	return NewPinName;
}

void UK2Node_SwitchEnum::CreateCasePins()
{
	if(NULL != Enum)
	{
		SetEnum(Enum);
	}

	const bool bShouldUseAdvancedView = (EnumEntries.Num() > 5);
	if(bShouldUseAdvancedView && (ENodeAdvancedPins::NoPins == AdvancedPinDisplay))
	{
		AdvancedPinDisplay = ENodeAdvancedPins::Hidden;
	}

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	for (auto EnumIt = EnumEntries.CreateConstIterator(); EnumIt; ++EnumIt)
	{
		FName EnumEntry = *EnumIt;
		UEdGraphPin * NewPin = CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, EnumEntry.ToString());
		int32 Index = EnumIt.GetIndex();
		if (EnumFriendlyNames.IsValidIndex(Index))
		{
			NewPin->PinFriendlyName = EnumFriendlyNames[Index];
		}
		
		if(bShouldUseAdvancedView && (EnumIt.GetIndex() > 2))
		{
			NewPin->bAdvancedView = true;
		}
	}
}

UK2Node::ERedirectType UK2Node_SwitchEnum::DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex) const
{
	UK2Node::ERedirectType ReturnValue = Super::DoPinsMatchForReconstruction(NewPin, NewPinIndex, OldPin, OldPinIndex);
	if (ReturnValue == UK2Node::ERedirectType_None && Enum && OldPinIndex > 2 && NewPinIndex > 2)
	{
		int32 OldIndex = Enum->FindEnumIndex(FName(*OldPin->PinName));
		int32 NewIndex = Enum->FindEnumIndex(FName(*NewPin->PinName));
		// This handles redirects properly
		if (OldIndex == NewIndex && OldIndex != INDEX_NONE)
		{
			ReturnValue = UK2Node::ERedirectType_Name;
		}
	}
	return ReturnValue;
}

void UK2Node_SwitchEnum::AddPinToSwitchNode()
{
	// first try to restore unconnected pin, since connected one is always visible
	for(auto PinIt = Pins.CreateIterator(); PinIt; ++PinIt)
	{
		UEdGraphPin* Pin = *PinIt;
		if(Pin && (0 == Pin->LinkedTo.Num()) && Pin->bAdvancedView)
		{
			Pin->Modify();
			Pin->bAdvancedView = false;
			return;
		}
	}

	for(auto PinIt = Pins.CreateIterator(); PinIt; ++PinIt)
	{
		UEdGraphPin* Pin = *PinIt;
		if(Pin && Pin->bAdvancedView)
		{
			Pin->Modify();
			Pin->bAdvancedView = false;
			return;
		}
	}
}

void UK2Node_SwitchEnum::RemovePinFromSwitchNode(UEdGraphPin* Pin)
{
	if(Pin)
	{
		if(!Pin->bAdvancedView)
		{
			Pin->Modify();
			Pin->bAdvancedView = true;
		}
		Pin->BreakAllPinLinks();
	}
}