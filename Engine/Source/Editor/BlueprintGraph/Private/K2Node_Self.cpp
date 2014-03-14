// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"
#include "KismetCompiler.h"

class FKCHandler_Self : public FNodeHandlingFunctor
{
public:
	FKCHandler_Self(FKismetCompilerContext& InCompilerContext)
		: FNodeHandlingFunctor(InCompilerContext)
	{
	}

	virtual void RegisterNets(FKismetFunctionContext& Context, UEdGraphNode* Node) OVERRIDE
	{
		UK2Node_Self* SelfNode = CastChecked<UK2Node_Self>(Node);
		const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

		UEdGraphPin* VarPin = SelfNode->FindPin(Schema->PN_Self);
		check( VarPin );

		FBPTerminal* Term = new (Context.Literals) FBPTerminal();
		Term->CopyFromPin(VarPin, VarPin->PinName);
		Term->bIsLiteral = true;
		Context.NetMap.Add(VarPin, Term);		
	}
};

UK2Node_Self::UK2Node_Self(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UK2Node_Self::AllocateDefaultPins()
{
	SelfClass = GetBlueprint()->GeneratedClass;

	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	CreatePin(EGPD_Output, K2Schema->PC_Object, K2Schema->PSC_Self, SelfClass, false, false, K2Schema->PN_Self);

	Super::AllocateDefaultPins();
}

FString UK2Node_Self::GetTooltip() const
{
	return FString::Printf(*NSLOCTEXT("K2Node", "GetSelfReference", "Gets a reference to this instance of the blueprint").ToString());
}

FString UK2Node_Self::GetKeywords() const
{
	return TEXT("This");
}

FString UK2Node_Self::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FString::Printf(*NSLOCTEXT("K2Node", "SelfReferenceName", "Self-Reference").ToString());
}

FNodeHandlingFunctor* UK2Node_Self::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_Self(CompilerContext);
}
