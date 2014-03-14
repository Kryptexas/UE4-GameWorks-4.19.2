// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"
#include "KismetCompiler.h"
#include "../../../Runtime/Engine/Classes/Kismet/KismetNodeHelperLibrary.h"
#include "../../../Runtime/Engine/Classes/Kismet/KismetSystemLibrary.h"

FString UK2Node_GetEnumeratorName::EnumeratorPinName = TEXT("Enumerator");

UK2Node_GetEnumeratorName::UK2Node_GetEnumeratorName(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UK2Node_GetEnumeratorName::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	CreatePin(EGPD_Input, Schema->PC_Byte, TEXT(""), NULL, false, false, EnumeratorPinName);
	CreatePin(EGPD_Output, Schema->PC_Name, TEXT(""), NULL, false, false, Schema->PN_ReturnValue);
}

FString UK2Node_GetEnumeratorName::GetTooltip() const
{
	return NSLOCTEXT("K2Node", "GetEnumeratorName_Tooltip", "Returns name of enumerator").ToString();
}

FString UK2Node_GetEnumeratorName::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("K2Node", "GetNode_Title", "Enum to Name").ToString();
}

FString UK2Node_GetEnumeratorName::GetCompactNodeTitle() const
{
	return TEXT("\x2022");
}

UEnum* UK2Node_GetEnumeratorName::GetEnum() const
{
	const UEdGraphPin* InputPin = FindPinChecked(EnumeratorPinName);
	const UEdGraphPin* EnumPin = InputPin->LinkedTo.Num() ? InputPin->LinkedTo[0] : InputPin;
	return EnumPin ? Cast<UEnum>(EnumPin->PinType.PinSubCategoryObject.Get()) : NULL;
}

void UK2Node_GetEnumeratorName::ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	const UEdGraphPin* OutputPin = FindPinChecked(Schema->PN_ReturnValue); 
	/*Don't validate isolated nodes */
	if (0 != OutputPin->LinkedTo.Num())
	{
		EarlyValidation(MessageLog);
	}
}

void UK2Node_GetEnumeratorName::EarlyValidation(class FCompilerResultsLog& MessageLog) const
{
	const UEnum* Enum = GetEnum();
	if (NULL == Enum)
	{
		MessageLog.Error(*NSLOCTEXT("K2Node", "GetNumEnumEntries_NoIntput_Error", "@@ Must have non-default Enum input").ToString(), this);
	}
}

bool UK2Node_GetEnumeratorName::IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	const UEdGraphPin* InputPin = FindPinChecked(EnumeratorPinName);
	if((InputPin == MyPin) && OtherPin && (OtherPin->PinType.PinCategory == Schema->PC_Byte))
	{
		if(NULL == Cast<UEnum>(OtherPin->PinType.PinSubCategoryObject.Get()))
		{
			OutReason = NSLOCTEXT("K2Node", "GetNumEnumEntries_NotEnum_Msg", "Input is not an Enum.").ToString();
			return true;
		}
	}

	return false;
}

FName UK2Node_GetEnumeratorName::GetFunctionName() const
{
	const FName FunctionName = GET_FUNCTION_NAME_CHECKED(UKismetNodeHelperLibrary, GetEnumeratorName);
	return FunctionName;
}

void UK2Node_GetEnumeratorName::UpdatePinType()
{
	UEdGraphPin* EnumPin = FindPinChecked(EnumeratorPinName);
	UEnum* Enum = GetEnum();
	if (EnumPin->PinType.PinSubCategoryObject != Enum)
	{
		EnumPin->PinType.PinSubCategoryObject = Enum;
		PinTypeChanged(EnumPin);
	}
}

void UK2Node_GetEnumeratorName::PostReconstructNode()
{
	Super::PostReconstructNode();
	UpdatePinType();
}

void UK2Node_GetEnumeratorName::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	if (Pin == FindPinChecked(EnumeratorPinName))
	{
		UpdatePinType();
	}
}

void UK2Node_GetEnumeratorName::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	if (CompilerContext.bIsFullCompile)
	{
		UEnum* Enum = GetEnum();
		if(NULL == Enum)
		{
			CompilerContext.MessageLog.Error(*FString::Printf(*NSLOCTEXT("K2Node", "GetEnumeratorNam_Error_MustHaveValidName", "@@ must have a valid enum defined").ToString()), this);
			return;
		}

		const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
		
		const UFunction* Function = UKismetNodeHelperLibrary::StaticClass()->FindFunctionByName( GetFunctionName() );
		check(NULL != Function);
		UK2Node_CallFunction* CallGetName = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph); 
		CallGetName->SetFromFunction(Function);
		CallGetName->AllocateDefaultPins();
		check(CallGetName->IsNodePure());
		
		//OPUTPUT PIN
		UEdGraphPin* OrgReturnPin = FindPinChecked(Schema->PN_ReturnValue);
		UEdGraphPin* NewReturnPin = CallGetName->GetReturnValuePin();
		check(NULL != NewReturnPin);
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*OrgReturnPin, *NewReturnPin), this);

		//ENUM PIN
		UEdGraphPin* EnumPin = CallGetName->FindPinChecked(TEXT("Enum"));
		Schema->TrySetDefaultObject(*EnumPin, Enum);
		check(EnumPin->DefaultObject == Enum);

		//INDEX PIN
		UEdGraphPin* OrgInputPin = FindPinChecked(EnumeratorPinName);
		UEdGraphPin* IndexPin = CallGetName->FindPinChecked(TEXT("EnumeratorIndex"));
		check(EGPD_Input == IndexPin->Direction && Schema->PC_Byte == IndexPin->PinType.PinCategory);
		CompilerContext.CheckConnectionResponse(Schema->MovePinLinks(*OrgInputPin, *IndexPin), this);

		if (!IndexPin->LinkedTo.Num())
		{
			//MAKE LITERAL BYTE FROM LITERAL ENUM
			const FString EnumLiteral = IndexPin->GetDefaultAsString();
			const int32 NumericValue = Enum->FindEnumIndex(*EnumLiteral);
			if (NumericValue == INDEX_NONE) 
			{
				CompilerContext.MessageLog.Error(*FString::Printf(*NSLOCTEXT("K2Node", "GetEnumeratorNam_Error_InvalidName", "@@ has invalid enum value '%s'").ToString(), *EnumLiteral), this);
				return;
			}
			const FString DefaultByteValue = FString::FromInt(NumericValue);

			// LITERAL BYTE FUNCTION
			const FName FunctionName = GET_FUNCTION_NAME_CHECKED(UKismetSystemLibrary, MakeLiteralByte);
			UK2Node_CallFunction* MakeLiteralByte = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph); 
			MakeLiteralByte->SetFromFunction(UKismetSystemLibrary::StaticClass()->FindFunctionByName(FunctionName));
			MakeLiteralByte->AllocateDefaultPins();

			UEdGraphPin* MakeLiteralByteReturnPin = MakeLiteralByte->FindPinChecked(Schema->PN_ReturnValue);
			Schema->TryCreateConnection(MakeLiteralByteReturnPin, IndexPin);

			UEdGraphPin* MakeLiteralByteInputPin = MakeLiteralByte->FindPinChecked(TEXT("Value"));
			MakeLiteralByteInputPin->DefaultValue = DefaultByteValue;
		}

		BreakAllNodeLinks();
	}
}