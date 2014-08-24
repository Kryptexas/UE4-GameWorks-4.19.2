// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"
#include "KismetCompiler.h"
#include "BlueprintNodeSpawner.h"
#include "EditorCategoryUtils.h"
#include "BlueprintActionDatabaseRegistrar.h"

#define LOCTEXT_NAMESPACE "K2Node_Self"

class FKCHandler_Self : public FNodeHandlingFunctor
{
public:
	FKCHandler_Self(FKismetCompilerContext& InCompilerContext)
		: FNodeHandlingFunctor(InCompilerContext)
	{
	}

	virtual void RegisterNets(FKismetFunctionContext& Context, UEdGraphNode* Node) override
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
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
	CreatePin(EGPD_Output, K2Schema->PC_Object, K2Schema->PSC_Self, NULL, false, false, K2Schema->PN_Self);

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

FText UK2Node_Self::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FText NodeTitle = NSLOCTEXT("K2Node", "SelfReferenceName", "Self-Reference");
	if (TitleType == ENodeTitleType::ListView)
	{
		NodeTitle = LOCTEXT("ListTitle", "Get a reference to self");
	}
	return NodeTitle;	
}

FNodeHandlingFunctor* UK2Node_Self::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_Self(CompilerContext);
}

void UK2Node_Self::ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const
{
	Super::ValidateNodeDuringCompilation(MessageLog);
	const UBlueprint* Blueprint = GetBlueprint();
	const UClass* MyClass = Blueprint ? Blueprint->GeneratedClass : NULL;
	const bool bValidClass = !MyClass || !MyClass->IsChildOf(UBlueprintFunctionLibrary::StaticClass());
	if (!bValidClass)
	{
		MessageLog.Warning(*NSLOCTEXT("K2Node", "InvalidSelfNode", "Self node @@ cannot be used in static library.").ToString(), this);
	}
}

void UK2Node_Self::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
	check(NodeSpawner != nullptr);

	ActionRegistrar.AddBlueprintAction(NodeSpawner);
}

FText UK2Node_Self::GetMenuCategory() const
{
	return FEditorCategoryUtils::GetCommonCategory(FCommonEditorCategory::Variables);
}

#undef LOCTEXT_NAMESPACE
