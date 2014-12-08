// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"
#include "KismetCompiler.h"

#define LOCTEXT_NAMESPACE "K2Node_FunctionEntry"

//////////////////////////////////////////////////////////////////////////
// FKCHandler_FunctionEntry

class FKCHandler_FunctionEntry : public FNodeHandlingFunctor
{
public:
	FKCHandler_FunctionEntry(FKismetCompilerContext& InCompilerContext)
		: FNodeHandlingFunctor(InCompilerContext)
	{
	}

	void RegisterFunctionInput(FKismetFunctionContext& Context, UEdGraphPin* Net, UFunction* Function)
	{
		// This net is a parameter into the function
		FBPTerminal* Term = new (Context.Parameters) FBPTerminal();
		Term->CopyFromPin(Net, Net->PinName);

		// Flag pass by reference parameters specially
		//@TODO: Still doesn't handle/allow users to declare new pass by reference, this only helps inherited functions
		if( Function )
		{
			if (UProperty* ParentProperty = FindField<UProperty>(Function, FName(*(Net->PinName))))
			{
				if (ParentProperty->HasAnyPropertyFlags(CPF_ReferenceParm))
				{
					Term->bPassedByReference = true;
				}
			}
		}


		Context.NetMap.Add(Net, Term);
	}

	virtual void RegisterNets(FKismetFunctionContext& Context, UEdGraphNode* Node) override
	{
		UK2Node_FunctionEntry* EntryNode = CastChecked<UK2Node_FunctionEntry>(Node);

		UFunction* Function = FindField<UFunction>(EntryNode->SignatureClass, EntryNode->SignatureName);

		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin->ParentPin == nullptr && !CompilerContext.GetSchema()->IsMetaPin(*Pin))
			{
				UEdGraphPin* Net = FEdGraphUtilities::GetNetFromPin(Pin);

				if (Context.NetMap.Find(Net) == NULL)
				{
					// New net, resolve the term that will be used to construct it
					FBPTerminal* Term = NULL;

					check(Net->Direction == EGPD_Output);

					RegisterFunctionInput(Context, Pin, Function);
				}
			}
		}
	}

	virtual void Compile(FKismetFunctionContext& Context, UEdGraphNode* Node) override
	{
		UK2Node_FunctionEntry* EntryNode = CastChecked<UK2Node_FunctionEntry>(Node);
		//check(EntryNode->SignatureName != NAME_None);
		if (EntryNode->SignatureName == CompilerContext.GetSchema()->FN_ExecuteUbergraphBase)
		{
			UEdGraphPin* EntryPointPin = Node->FindPin(CompilerContext.GetSchema()->PN_EntryPoint);
			FBPTerminal** pTerm = Context.NetMap.Find(EntryPointPin);
			if ((EntryPointPin != NULL) && (pTerm != NULL))
			{
				FBlueprintCompiledStatement& ComputedGotoStatement = Context.AppendStatementForNode(Node);
				ComputedGotoStatement.Type = KCST_ComputedGoto;
				ComputedGotoStatement.LHS = *pTerm;
			}
			else
			{
				CompilerContext.MessageLog.Error(*LOCTEXT("NoEntryPointPin_Error", "Expected a pin named EntryPoint on @@").ToString(), Node);
			}
		}
		else
		{
			// Generate the output impulse from this node
			GenerateSimpleThenGoto(Context, *Node);
		}
	}
};

struct FFunctionEntryHelper
{
	static const FString& GetWorldContextPinName()
	{
		static const FString WorldContextPinName(TEXT("__WorldContext"));
		return WorldContextPinName;
	}

	static bool RequireWorldContextParameter(const UK2Node_FunctionEntry* Node)
	{
		const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
		return K2Schema->IsStaticFunctionGraph(Node->GetGraph());
	}
};

UK2Node_FunctionEntry::UK2Node_FunctionEntry(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Enforce const-correctness by default
	bEnforceConstCorrectness = true;
}

void UK2Node_FunctionEntry::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsLoading())
	{
		if (Ar.UE4Ver() < VER_UE4_BLUEPRINT_ENFORCE_CONST_IN_FUNCTION_OVERRIDES)
		{
			// Allow legacy implementations to violate const-correctness
			bEnforceConstCorrectness = false;
		}
	}
}

FText UK2Node_FunctionEntry::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	UEdGraph* Graph = GetGraph();
	FGraphDisplayInfo DisplayInfo;
	Graph->GetSchema()->GetGraphDisplayInformation(*Graph, DisplayInfo);

	return DisplayInfo.DisplayName;
}

void UK2Node_FunctionEntry::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	CreatePin(EGPD_Output, K2Schema->PC_Exec, TEXT(""), NULL, false, false, K2Schema->PN_Then);

	UFunction* Function = FindField<UFunction>(SignatureClass, SignatureName);
	if (Function != NULL)
	{
		CreatePinsForFunctionEntryExit(Function, /*bIsFunctionEntry=*/ true);
	}

	Super::AllocateDefaultPins();

	if (FFunctionEntryHelper::RequireWorldContextParameter(this) 
		&& ensure(!FindPin(FFunctionEntryHelper::GetWorldContextPinName())))
	{
		UEdGraphPin* WorldContextPin = CreatePin(
			EGPD_Output,
			K2Schema->PC_Object,
			FString(),
			UObject::StaticClass(),
			false,
			false,
			FFunctionEntryHelper::GetWorldContextPinName());
		WorldContextPin->bHidden = true;
	}
}

UEdGraphPin* UK2Node_FunctionEntry::GetAutoWorldContextPin() const
{
	return FFunctionEntryHelper::RequireWorldContextParameter(this) ? FindPin(FFunctionEntryHelper::GetWorldContextPinName()) : NULL;
}

void UK2Node_FunctionEntry::RemoveUnnecessaryAutoWorldContext()
{
	auto WorldContextPin = GetAutoWorldContextPin();
	if (WorldContextPin)
	{
		if (!WorldContextPin->LinkedTo.Num())
		{
			Pins.Remove(WorldContextPin);
		}
	}
}

void UK2Node_FunctionEntry::RemoveOutputPin(UEdGraphPin* PinToRemove)
{
	UK2Node_FunctionEntry* OwningSeq = Cast<UK2Node_FunctionEntry>( PinToRemove->GetOwningNode() );
	if (OwningSeq)
	{
		PinToRemove->BreakAllPinLinks();
		OwningSeq->Pins.Remove(PinToRemove);
	}
}

UEdGraphPin* UK2Node_FunctionEntry::CreatePinFromUserDefinition(const TSharedPtr<FUserPinInfo> NewPinInfo)
{
	UEdGraphPin* NewPin = CreatePin(
		EGPD_Output, 
		NewPinInfo->PinType.PinCategory, 
		NewPinInfo->PinType.PinSubCategory, 
		NewPinInfo->PinType.PinSubCategoryObject.Get(), 
		NewPinInfo->PinType.bIsArray, 
		NewPinInfo->PinType.bIsReference, 
		NewPinInfo->PinName);
	NewPin->DefaultValue = NewPin->AutogeneratedDefaultValue = NewPinInfo->PinDefaultValue;
	return NewPin;
}

FNodeHandlingFunctor* UK2Node_FunctionEntry::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_FunctionEntry(CompilerContext);
}

void UK2Node_FunctionEntry::GetRedirectPinNames(const UEdGraphPin& Pin, TArray<FString>& RedirectPinNames) const
{
	Super::GetRedirectPinNames(Pin, RedirectPinNames);

	if(RedirectPinNames.Num() > 0)
	{
		const FString& OldPinName = RedirectPinNames[0];

		
		// first add functionname.param
		RedirectPinNames.Add(FString::Printf(TEXT("%s.%s"), *SignatureName.ToString(), *OldPinName));
		// if there is class, also add an option for class.functionname.param
		if(SignatureClass!=NULL)
		{
			RedirectPinNames.Add(FString::Printf(TEXT("%s.%s.%s"), *SignatureClass->GetName(), *SignatureName.ToString(), *OldPinName));
		}
	}
}

bool UK2Node_FunctionEntry::IsDeprecated() const
{
	if (UFunction* const Function = FindField<UFunction>(SignatureClass, SignatureName))
	{
		return Function->HasMetaData(FBlueprintMetadata::MD_DeprecatedFunction);
	}

	return false;
}

FString UK2Node_FunctionEntry::GetDeprecationMessage() const
{
	if (UFunction* const Function = FindField<UFunction>(SignatureClass, SignatureName))
	{
		if (Function->HasMetaData(FBlueprintMetadata::MD_DeprecationMessage))
		{
			return FString::Printf(TEXT("%s %s"), *LOCTEXT("FunctionDeprecated_Warning", "@@ is deprecated;").ToString(), *Function->GetMetaData(FBlueprintMetadata::MD_DeprecationMessage));
		}
	}

	return Super::GetDeprecationMessage();
}

#undef LOCTEXT_NAMESPACE