// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "DynamicCastHandler.h"

#define LOCTEXT_NAMESPACE "DynamicCastHandler"

//////////////////////////////////////////////////////////////////////////
// FKCHandler_DynamicCast

void FKCHandler_DynamicCast::RegisterNets(FKismetFunctionContext& Context, UEdGraphNode* Node)
{
	FNodeHandlingFunctor::RegisterNets(Context, Node);

	// Create a term to determine if the cast was successful or not
	FBPTerminal* BoolTerm = new (Context.IsEventGraph() ? Context.EventGraphLocals : Context.Locals) FBPTerminal();
	BoolTerm->Type.PinCategory = CompilerContext.GetSchema()->PC_Boolean;
	BoolTerm->Source = Node;
	BoolTerm->Name = Context.NetNameMap->MakeValidName(Node) + TEXT("_CastSuccess");
	BoolTerm->bIsLocal = true;
	BoolTermMap.Add(Node, BoolTerm);
}

void FKCHandler_DynamicCast::RegisterNet(FKismetFunctionContext& Context, UEdGraphPin* Net)
{
	FBPTerminal* Term = new (Context.IsEventGraph() ? Context.EventGraphLocals : Context.Locals) FBPTerminal();
	Term->CopyFromPin(Net, Context.NetNameMap->MakeValidName(Net));
	Context.NetMap.Add(Net, Term);
}

void FKCHandler_DynamicCast::Compile(FKismetFunctionContext& Context, UEdGraphNode* Node)
{
	const UK2Node_DynamicCast* DynamicCastNode = CastChecked<UK2Node_DynamicCast>(Node);

	if (DynamicCastNode->TargetType == NULL)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("BadCastNoTargetType_Error", "Node @@ has an invalid target type, please delete and recreate it").ToString(), Node);
	}

	// Successful interface cast execution
	const UEdGraphPin* SuccessPin = DynamicCastNode->GetValidCastPin();
	UEdGraphNode* SuccessNode = NULL;
	if (SuccessPin->LinkedTo.Num() > 0)
	{
		SuccessNode = SuccessPin->LinkedTo[0]->GetOwningNode();
	}

	// Failed interface cast execution
	const UEdGraphPin* FailurePin = DynamicCastNode->GetInvalidCastPin();
	UEdGraphNode* FailureNode = NULL;
	if (FailurePin->LinkedTo.Num() > 0)
	{
		FailureNode = FailurePin->LinkedTo[0]->GetOwningNode();
	}

	// Self Pin
	UEdGraphPin* SourceObjectPin = DynamicCastNode->GetCastSourcePin();
	UEdGraphPin* PinToTry = FEdGraphUtilities::GetNetFromPin(SourceObjectPin);
	FBPTerminal** ObjectToCast = Context.NetMap.Find(PinToTry);

	if (!ObjectToCast)
	{
		ObjectToCast = Context.LiteralHackMap.Find(PinToTry);

		if (!ObjectToCast)
		{
			CompilerContext.MessageLog.Error(*LOCTEXT("InvalidConnectionOnNode_Error", "Node @@ has an invalid connection on @@").ToString(), Node, SourceObjectPin);
			return;
		}
	}

	// Output pin
	const UEdGraphPin* InterfacePin = DynamicCastNode->GetCastResultPin();
	if( !InterfacePin )
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("InvalidDynamicCastClass_Error", "Node @@ has an invalid target class").ToString(), Node);
		return;
	}

	FBPTerminal** InterfaceTerm = Context.NetMap.Find(InterfacePin);

	// Create a literal term from the class specified in the node
	FBPTerminal* ClassTerm = new (Context.IsEventGraph() ? Context.EventGraphLocals : Context.Locals) FBPTerminal();
	ClassTerm->Name = DynamicCastNode->TargetType->GetName();
	ClassTerm->bIsLiteral = true;
	ClassTerm->Source = DynamicCastNode->TargetType;
	ClassTerm->ObjectLiteral = DynamicCastNode->TargetType;

	// Find the boolean intermediate result term, so we can track whether the cast was successful
	FBPTerminal* BoolTerm = *BoolTermMap.Find(DynamicCastNode);

	// Cast Statement
	FBlueprintCompiledStatement& CastStatement = Context.AppendStatementForNode(Node);
	CastStatement.Type = CastType;
	CastStatement.LHS = *InterfaceTerm;
	CastStatement.RHS.Add(ClassTerm);
	CastStatement.RHS.Add(*ObjectToCast);

	// Check result of cast statement
	FBlueprintCompiledStatement& CheckResultStatement = Context.AppendStatementForNode(Node);
	const UClass* SubObjectClass = Cast<UClass>((*InterfaceTerm)->Type.PinSubCategoryObject.Get());
	const bool bIsInterfaceCast = (SubObjectClass != NULL && SubObjectClass->HasAnyClassFlags(CLASS_Interface));

	CheckResultStatement.Type = KCST_ObjectToBool;
	CheckResultStatement.LHS = BoolTerm;
	CheckResultStatement.RHS.Add(*InterfaceTerm);

	// Failure condition...skip to the failed output
	FBlueprintCompiledStatement& FailCastGoto = Context.AppendStatementForNode(Node);
	FailCastGoto.Type = KCST_GotoIfNot;
	FailCastGoto.LHS = BoolTerm;
	Context.GotoFixupRequestMap.Add(&FailCastGoto, FailureNode);

	// Successful cast...hit the success output node
	FBlueprintCompiledStatement& SuccessCastGoto = Context.AppendStatementForNode(Node);
	SuccessCastGoto.Type = KCST_UnconditionalGoto;
	SuccessCastGoto.LHS = BoolTerm;
	Context.GotoFixupRequestMap.Add(&SuccessCastGoto, SuccessNode);
}

#undef LOCTEXT_NAMESPACE
