// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraNodeIf.h"
#include "NiagaraEditorUtilities.h"
#include "INiagaraCompiler.h"

#define LOCTEXT_NAMESPACE "NiagaraNodeIf"

UNiagaraNodeIf::UNiagaraNodeIf(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UNiagaraNodeIf::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	// @TODO why do we need to have this post-change property here at all? 
	// Doing a null check b/c otherwise if doing a Duplicate via Ctrl-W, we die inside AllocateDefaultPins due to 
	// the point where we get this call not being completely formed.
	if (PropertyChangedEvent.Property != nullptr)
	{
		ReallocatePins();
	}
}

void UNiagaraNodeIf::AllocateDefaultPins()
{
	const UEdGraphSchema_Niagara* Schema = GetDefault<UEdGraphSchema_Niagara>();

	//Add the condition pin.
	CreatePin(EGPD_Input, Schema->TypeDefinitionToPinType(FNiagaraTypeDefinition::GetBoolDef()), TEXT("Condition"));

	//Create the inputs for each path.
	for (FNiagaraVariable& Var : OutputVars)
	{
		FString PathSuffix = TEXT(" A");
		CreatePin(EGPD_Input, Schema->TypeDefinitionToPinType(Var.GetType()), Var.GetName().ToString() + PathSuffix);		
	}

	for (FNiagaraVariable& Var : OutputVars)
	{
		FString PathSuffix = TEXT(" B");
		CreatePin(EGPD_Input, Schema->TypeDefinitionToPinType(Var.GetType()), Var.GetName().ToString() + PathSuffix);
	}

	for (FNiagaraVariable& Var : OutputVars)
	{
		UEdGraphPin* NewPin = CreatePin(EGPD_Output, Schema->TypeDefinitionToPinType(Var.GetType()), Var.GetName().ToString());
		NewPin->PersistentGuid = Var.GetId();
	}

	CreateAddPin(EGPD_Output);
}

void UNiagaraNodeIf::Compile(class INiagaraCompiler* Compiler, TArray<int32>& Outputs)
{
	int32 PinIdx = 0;
	int32 Condition = Compiler->CompilePin(Pins[PinIdx++]);
	TArray<int32> PathA;
	PathA.Reserve(OutputVars.Num());
	for (int32 i = 0; i < OutputVars.Num(); ++i)
	{
		PathA.Add(Compiler->CompilePin(Pins[PinIdx++]));
	}
	TArray<int32> PathB;
	PathB.Reserve(OutputVars.Num());
	for (int32 i = 0; i < OutputVars.Num(); ++i)
	{
		PathB.Add(Compiler->CompilePin(Pins[PinIdx++]));
	}
	Compiler->If(OutputVars, Condition, PathA, PathB, Outputs);
}

void UNiagaraNodeIf::RefreshFromExternalChanges()
{
	ReallocatePins();
}

void UNiagaraNodeIf::OnPinRemoved(UEdGraphPin* PinToRemove)
{
	auto RemovePredicate = [=](const FNiagaraVariable& Variable) { return Variable.GetId() == PinToRemove->PersistentGuid; };
	OutputVars.RemoveAll(RemovePredicate);
	ReallocatePins();
}

void UNiagaraNodeIf::OnNewTypedPinAdded(UEdGraphPin* NewPin)
{
	Super::OnNewTypedPinAdded(NewPin);

	const UEdGraphSchema_Niagara* Schema = GetDefault<UEdGraphSchema_Niagara>();
	FNiagaraTypeDefinition OutputType = Schema->PinToTypeDefinition(NewPin);

	TSet<FName> OutputNames;
	for (const FNiagaraVariable& Output : OutputVars)
	{
		OutputNames.Add(Output.GetName());
	}
	FName OutputName = FNiagaraEditorUtilities::GetUniqueName(*OutputType.GetStruct()->GetDisplayNameText().ToString(), OutputNames);

	FNiagaraVariable NewOutput(OutputType, OutputName);
	NewOutput.SetId(FGuid::NewGuid());
	OutputVars.Add(NewOutput);

	// Update the pin's data too so that it's connection is maintained after reallocating.
	NewPin->PersistentGuid = NewOutput.GetId();

	ReallocatePins();
}

void UNiagaraNodeIf::OnPinRenamed(UEdGraphPin* RenamedPin)
{
	auto FindPredicate = [=](const FNiagaraVariable& Variable) { return Variable.GetId() == RenamedPin->PersistentGuid; };
	FNiagaraVariable* OutputForPin = OutputVars.FindByPredicate(FindPredicate);
	if(OutputForPin != nullptr)
	{
		TSet<FName> OutputNames;
		for (const FNiagaraVariable& Output : OutputVars)
		{
			if (&Output != OutputForPin)
			{
				OutputNames.Add(Output.GetName());
			}
		}
		FName OutputName = FNiagaraEditorUtilities::GetUniqueName(*RenamedPin->PinName, OutputNames);
		OutputForPin->SetName(OutputName);
	}
	ReallocatePins();
}

bool UNiagaraNodeIf::CanRenamePin(const UEdGraphPin* Pin) const
{
	return Super::CanRenamePin(Pin) && Pin->Direction == EGPD_Output;
}

bool UNiagaraNodeIf::CanRemovePin(const UEdGraphPin* Pin) const
{
	return Super::CanRemovePin(Pin) && Pin->Direction == EGPD_Output;
}

#undef LOCTEXT_NAMESPACE
