// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"

#include "K2Node_BreakStruct.h"
#include "KismetCompiler.h"

#define LOCTEXT_NAMESPACE "K2Node_BreakStruct"

//////////////////////////////////////////////////////////////////////////
// FKCHandler_BreakStruct

class FKCHandler_BreakStruct : public FNodeHandlingFunctor
{
public:
	FKCHandler_BreakStruct(FKismetCompilerContext& InCompilerContext)
		: FNodeHandlingFunctor(InCompilerContext)
	{
	}

	FBPTerminal* RegisterInputTerm(FKismetFunctionContext& Context, UK2Node_BreakStruct* Node)
	{
		check(NULL != Node);

		if(NULL == Node->StructType)
		{
			CompilerContext.MessageLog.Error(*LOCTEXT("BreakStruct_UnknownStructure_Error", "Unknown structure to break for @@").ToString(), Node);
			return NULL;
		}

		//Find input pin
		UEdGraphPin* InputPin = NULL;
		for (int32 PinIndex = 0; PinIndex < Node->Pins.Num(); ++PinIndex)
		{
			UEdGraphPin* Pin = Node->Pins[PinIndex];
			if(Pin && (EGPD_Input == Pin->Direction))
			{
				InputPin = Pin;
				break;
			}
		}
		check(NULL != InputPin);

		//Find structure source net
		UEdGraphPin* Net = FEdGraphUtilities::GetNetFromPin(InputPin);
		check(NULL != Net);

		FBPTerminal** FoundTerm = Context.NetMap.Find(Net);
		FBPTerminal* Term = FoundTerm ? *FoundTerm : NULL;
		if(NULL == Term)
		{
			// Dont allow literal
			if ((Net->Direction == EGPD_Input) && (Net->LinkedTo.Num() == 0))
			{
				CompilerContext.MessageLog.Error(*LOCTEXT("InvalidNoInputStructure_Error", "No input structure to break for @@").ToString(), Net);
				return NULL;
			}
			// standard register net
			else
			{
				Term = new (Context.IsEventGraph() ? Context.EventGraphLocals : Context.Locals) FBPTerminal();
				Term->CopyFromPin(Net, Context.NetNameMap->MakeValidName(Net));
			}
			Context.NetMap.Add(Net, Term);
		}
		UStruct* StructInTerm = Cast<UStruct>(Term->Type.PinSubCategoryObject.Get());
		if(NULL == StructInTerm || !StructInTerm->IsChildOf(Node->StructType))
		{
			CompilerContext.MessageLog.Error(*LOCTEXT("BreakStruct_NoMatch_Error", "Structures don't match for @@").ToString(), Node);
		}

		return Term;
	}

	void RegisterOutputTerm(FKismetFunctionContext& Context, UScriptStruct* StructType, UEdGraphPin* Net, FBPTerminal* ContextTerm)
	{
		UProperty* BoundProperty = FindField<UProperty>(StructType, *(Net->PinName));

		if (BoundProperty != NULL)
		{
			FBPTerminal* Term = new (Context.IsEventGraph() ? Context.EventGraphLocals : Context.Locals) FBPTerminal();
			Term->CopyFromPin(Net, Net->PinName);
			Term->AssociatedVarProperty = BoundProperty;
			Context.NetMap.Add(Net, Term);
			Term->Context = ContextTerm;

			if (BoundProperty->HasAnyPropertyFlags(CPF_BlueprintReadOnly))
			{
				Term->bIsConst = true;
			}
		}
		else
		{
			CompilerContext.MessageLog.Error(TEXT("Failed to find a struct member for @@"), Net);
		}
	}

	virtual void RegisterNets(FKismetFunctionContext& Context, UEdGraphNode* InNode) OVERRIDE
	{
		UK2Node_BreakStruct* Node = Cast<UK2Node_BreakStruct>(InNode);
		check(NULL != Node);

		if(!UK2Node_BreakStruct::CanBeBroken(Node->StructType))
		{
			CompilerContext.MessageLog.Warning(*LOCTEXT("BreakStruct_NoBreak_Error", "The structure cannot be broken using generic 'break' node @@. Try use specialized 'break' function if available.").ToString(), Node);
		}

		if(FBPTerminal* StructContextTerm = RegisterInputTerm(Context, Node))
		{
			for (int32 PinIndex = 0; PinIndex < Node->Pins.Num(); ++PinIndex)
			{
				UEdGraphPin* Pin = Node->Pins[PinIndex];
				if(NULL != Pin && EGPD_Output == Pin->Direction)
				{
					RegisterOutputTerm(Context, Node->StructType, Pin, StructContextTerm);
				}
			}
		}
	}
};


UK2Node_BreakStruct::UK2Node_BreakStruct(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
}

bool UK2Node_BreakStruct::CanBeBroken(const UScriptStruct* Struct)
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	if(Struct && Schema && !Struct->GetBoolMetaData(TEXT("HasNativeMakeBreak")))
	{
		for (TFieldIterator<UProperty> It(Struct); It; ++It)
		{
			if(const UProperty* Property = *It)
			{
				FEdGraphPinType DumbGraphPinType;
				const bool bConvertable = Schema->ConvertPropertyToPinType(Property, /*out*/ DumbGraphPinType);
				const bool bVisible = Property->HasAnyPropertyFlags(CPF_BlueprintVisible|CPF_Edit);
				if(bVisible && bConvertable)
				{
					return true;
				}
			}
		}
	}
	return false;
}

void UK2Node_BreakStruct::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	if(Schema && StructType)
	{
		CreatePin(EGPD_Input, Schema->PC_Struct, TEXT(""), StructType, false, false, StructType->GetName());
		
		UK2Node_StructMemberGet::AllocateDefaultPins();

		// When struct has a lot of fields, mark their pins as advanced
		if(Pins.Num() > 5)
		{
			if(ENodeAdvancedPins::NoPins == AdvancedPinDisplay)
			{
				AdvancedPinDisplay = ENodeAdvancedPins::Hidden;
			}

			for(int32 PinIndex = 3; PinIndex < Pins.Num(); ++PinIndex)
			{
				if(UEdGraphPin * EdGraphPin = Pins[PinIndex])
				{
					EdGraphPin->bAdvancedView = true;
				}
			}
		}
	}
}

FString UK2Node_BreakStruct::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FString(TEXT("Break ")) + (StructType ? StructType->GetName() : FString());
}

FString UK2Node_BreakStruct::GetTooltip() const
{
	return FString::Printf(
		*LOCTEXT("MakeStruct_Tooltip", "Adds a node that breaks a '%s' into its member fields").ToString(),
		*(StructType ? StructType->GetName() : FString())
		);
}

void UK2Node_BreakStruct::ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const
{
	if(!StructType)
	{
		MessageLog.Error(*LOCTEXT("NoStruct_Error", "No Struct in @@").ToString(), this);
	}
}

FLinearColor UK2Node_BreakStruct::GetNodeTitleColor() const 
{
	if(const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>())
	{
		FEdGraphPinType PinType;
		PinType.PinCategory = K2Schema->PC_Struct;
		PinType.PinSubCategoryObject = StructType;
		return K2Schema->GetPinTypeColor(PinType);
	}
	return UK2Node::GetNodeTitleColor();
}

UK2Node::ERedirectType UK2Node_BreakStruct::DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex)  const
{
	ERedirectType Result = UK2Node::DoPinsMatchForReconstruction(NewPin, NewPinIndex, OldPin, OldPinIndex);
	if ((ERedirectType_None == Result) && NewPin && OldPin && (EGPD_Input == NewPin->Direction) && (EGPD_Input == OldPin->Direction))
	{
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
		if (K2Schema->ArePinTypesCompatible( NewPin->PinType, OldPin->PinType))
		{
			Result = ERedirectType_Custom;
		}
	}
	return Result;
}

FNodeHandlingFunctor* UK2Node_BreakStruct::CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_BreakStruct(CompilerContext);
}

#undef LOCTEXT_NAMESPACE
