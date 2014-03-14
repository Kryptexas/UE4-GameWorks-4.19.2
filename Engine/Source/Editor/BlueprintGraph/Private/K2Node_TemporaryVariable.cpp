// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"
#include "KismetCompiler.h"

class FKCHandler_TemporaryVariable : public FNodeHandlingFunctor
{
public:
	FKCHandler_TemporaryVariable(FKismetCompilerContext& InCompilerContext)
		: FNodeHandlingFunctor(InCompilerContext)
	{
	}

	virtual void RegisterNet(FKismetFunctionContext& Context, UEdGraphPin* Net) OVERRIDE
	{
		// This net is an anonymous temporary variable
		FBPTerminal* Term = new (Context.IsEventGraph() ? Context.EventGraphLocals : Context.Locals) FBPTerminal();

		FString NetName = Context.NetNameMap->MakeValidName(Net);

		Term->CopyFromPin(Net, NetName);

		UK2Node_TemporaryVariable* TempVarNode = CastChecked<UK2Node_TemporaryVariable>(Net->GetOwningNode());
		Term->bIsSavePersistent = TempVarNode->bIsPersistent;

		Context.NetMap.Add(Net, Term);
	}
};

UK2Node_TemporaryVariable::UK2Node_TemporaryVariable(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, bIsPersistent(false)
{
}

void UK2Node_TemporaryVariable::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* VariablePin = CreatePin(EGPD_Output, TEXT(""), TEXT(""), NULL, false, false, TEXT("Variable"));
	VariablePin->PinType = VariableType;

	Super::AllocateDefaultPins();
}

FString UK2Node_TemporaryVariable::GetTooltip() const
{
	return FString::Printf(*NSLOCTEXT("K2Node", "LocalTemporaryVariable", "Local temporary %s variable").ToString(), *UEdGraphSchema_K2::TypeToString(VariableType));
}

FString UK2Node_TemporaryVariable::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FString Result = !bIsPersistent ? NSLOCTEXT("K2Node", "LocalVariable", "Local ").ToString() : NSLOCTEXT("K2Node", "PersistentLocalVariable", "Persistent Local ").ToString();
	Result += UEdGraphSchema_K2::TypeToString(VariableType);
	return Result;
}

bool UK2Node_TemporaryVariable::IsNodePure() const
{
	return true;
}

FString UK2Node_TemporaryVariable::GetDescriptiveCompiledName() const
{
	FString Result = NSLOCTEXT("K2Node", "TempPinCategory", "Temp_").ToString() + VariableType.PinCategory;
		
	if (!NodeComment.IsEmpty())
	{
		Result += TEXT("_");
		Result += NodeComment;
	}

	// If this node is persistent, we need to add the NodeGuid, which should be propagated from the macro that created this, in order to ensure persistence 
	if (bIsPersistent)
	{
		Result += TEXT("_");
		Result += NodeGuid.ToString();
	}

	return Result;
}

// get variable pin
UEdGraphPin* UK2Node_TemporaryVariable::GetVariablePin()
{
	return FindPin(TEXT("Variable"));
}

FNodeHandlingFunctor* UK2Node_TemporaryVariable::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_TemporaryVariable(CompilerContext);
}
