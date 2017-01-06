// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "NiagaraNodeInput.h"
#include "UObject/UnrealType.h"
#include "INiagaraCompiler.h"
#include "NiagaraGraph.h"

#include "NiagaraNodeOutput.h"
#include "NiagaraNodeOp.h"
#include "NiagaraNodeFunctionCall.h"
#include "EdGraphSchema_Niagara.h"
#include "NiagaraDataInterface.h"


#define LOCTEXT_NAMESPACE "NiagaraNodeInput"


UNiagaraNodeInput::UNiagaraNodeInput(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, DataInterface(nullptr)
	, Usage(ENiagaraInputNodeUsage::Undefined)	
	, CallSortPriority(0)
{
}

void UNiagaraNodeInput::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property != nullptr)
	{
		if (UClass* Class = const_cast<UClass*>(Input.GetType().GetClass()))
		{
			check(Class->IsChildOf(UNiagaraDataInterface::StaticClass()));
			if (DataInterface)
			{
				if (DataInterface->GetClass() != Class)
				{
					//Class has changed so clear this out and allocate pins will create a new instance of the correct type.
					//Should we preserve old objects somewhere so settings aren't lost when switching around types?
					DataInterface = nullptr;
				}
				else
				{
					//Keep it with the same name as the input 
					if (PropertyChangedEvent.Property->GetName() == TEXT("Input"))
					{
						DataInterface->Rename(*Input.GetName().ToString());
					}
				}
			}
		}
		else
		{
			DataInterface = nullptr;
		}

		ReallocatePins();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UNiagaraNodeInput::AllocateDefaultPins()
{
	if (UClass* Class = const_cast<UClass*>(Input.GetType().GetClass()))
	{
		check(Class->IsChildOf(UNiagaraDataInterface::StaticClass()));
		if (!DataInterface)
		{
			DataInterface = NewObject<UNiagaraDataInterface>(this, Class);
		}
	}

	const UEdGraphSchema_Niagara* Schema = GetDefault<UEdGraphSchema_Niagara>();

	//If we're a parameter node for a funciton or a module the we allow a "default" input pin.
	UNiagaraScript* OwnerScript = GetTypedOuter<UNiagaraScript>();
	check(OwnerScript);
	if ((!IsRequired() && IsExposed()) && DataInterface == nullptr && Usage == ENiagaraInputNodeUsage::Parameter && (OwnerScript->Usage == ENiagaraScriptUsage::Function || OwnerScript->Usage == ENiagaraScriptUsage::Module))
	{
		UEdGraphPin* NewPin = CreatePin(EGPD_Input, Schema->TypeDefinitionToPinType(Input.GetType()), TEXT("Default"));
		NewPin->bDefaultValueIsIgnored = true;
	}

	CreatePin(EGPD_Output, Schema->TypeDefinitionToPinType(Input.GetType()), TEXT("Input"));
}

FText UNiagaraNodeInput::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromName(Input.GetName());
}

FLinearColor UNiagaraNodeInput::GetNodeTitleColor() const
{
	const UEdGraphSchema_Niagara* Schema = CastChecked<UEdGraphSchema_Niagara>(GetSchema());
	switch (Usage)
	{
	case ENiagaraInputNodeUsage::Parameter:
		return Schema->NodeTitleColor_Constant;
	case ENiagaraInputNodeUsage::SystemConstant:
		return Schema->NodeTitleColor_SystemConstant;
	case ENiagaraInputNodeUsage::Attribute:
		return Schema->NodeTitleColor_Attribute;
	default:
		// TODO: Do something better here.
		return FLinearColor::Black;
	}
}

void UNiagaraNodeInput::AutowireNewNode(UEdGraphPin* FromPin)
{
	if (FromPin != nullptr)
	{
		const UEdGraphSchema_Niagara* Schema = CastChecked<UEdGraphSchema_Niagara>(GetSchema());
		check(Schema);
		if (Usage == ENiagaraInputNodeUsage::Parameter)
		{
			FNiagaraTypeDefinition Type = Input.GetType();
			if (Type == FNiagaraTypeDefinition::GetGenericNumericDef())
			{
				//Try to get a real type if we've been set to numeric
				Type = Schema->PinToTypeDefinition(FromPin);
			}

			if (UNiagaraNodeOp* OpNode = Cast<UNiagaraNodeOp>(FromPin->GetOwningNode()))
			{
				const FNiagaraOpInfo* OpInfo = FNiagaraOpInfo::GetOpInfo(OpNode->OpName);
				check(OpInfo);
				Input.SetType(Type);
				Input.SetName(*(OpInfo->FriendlyName.ToString() + TEXT(" ") + FromPin->PinName));
			}
			else if (UNiagaraNodeFunctionCall* FuncNode = Cast<UNiagaraNodeFunctionCall>(FromPin->GetOwningNode()))
			{
				Input.SetType(Type);
				Input.SetName(*(FuncNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString() + TEXT(" ") + FromPin->PinName));
			}
			else
			{
				Input.SetType(Type);
				Input.SetName(*FromPin->PinName);
			}
			ReallocatePins();
		}

		TArray<UEdGraphPin*> OutPins;
		GetOutputPins(OutPins);
		check(OutPins.Num() == 1 && OutPins[0] != NULL);
		
		if (GetSchema()->TryCreateConnection(FromPin, OutPins[0]))
		{
			FromPin->GetOwningNode()->NodeConnectionListChanged();
		}
	}
}

void UNiagaraNodeInput::NotifyInputTypeChanged()
{
	ReallocatePins();
}

void UNiagaraNodeInput::Compile(class INiagaraCompiler* Compiler, TArray<int32>& Outputs)
{
	int32 FunctionParam = INDEX_NONE;
	if (IsExposed() && Compiler->GetFunctionParameter(Input, FunctionParam))
	{
		//If we're in a function and this parameter hasn't been provided, compile the local default.
		if (FunctionParam == INDEX_NONE)
		{
			TArray<UEdGraphPin*> InputPins;
			GetInputPins(InputPins);
			int32 Default = InputPins.Num() > 0 ? Compiler->CompilePin(InputPins[0]) : INDEX_NONE;
			if (Default == INDEX_NONE)
			{
				//We failed to compile the default pin so just use the value of the input.
				Default = Compiler->GetConstant(Input);
			}
			Outputs.Add(Default);
			return;
		}
	}

	switch (Usage)
	{
	case ENiagaraInputNodeUsage::Parameter:
		if (DataInterface)
		{
			check(Input.GetType().GetClass());
			Outputs.Add(Compiler->RegisterDataInterface(Input, DataInterface)); break;
		}
		else
		{
			Outputs.Add(Compiler->GetParameter(Input)); break;
		}
	case ENiagaraInputNodeUsage::SystemConstant:
		Outputs.Add(Compiler->GetParameter(Input)); break;
	case ENiagaraInputNodeUsage::Attribute:
		Outputs.Add(Compiler->GetAttribute(Input)); break;
	default:
		check(false);
	}
}


#undef LOCTEXT_NAMESPACE
