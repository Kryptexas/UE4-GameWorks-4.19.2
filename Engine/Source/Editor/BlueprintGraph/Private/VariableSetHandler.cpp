// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"

#include "KismetCompiler.h"
#include "VariableSetHandler.h"

#define LOCTEXT_NAMESPACE "VariableSetHandler"

//////////////////////////////////////////////////////////////////////////
// FKCHandler_VariableSet

void FKCHandler_VariableSet::RegisterNet(FKismetFunctionContext& Context, UEdGraphPin* Net)
{
	// This net is a variable write
	ResolveAndRegisterScopedTerm(Context, Net, Context.VariableReferences);
}

void FKCHandler_VariableSet::RegisterNets(FKismetFunctionContext& Context, UEdGraphNode* Node)
{
	if(UK2Node_Variable* SetterNode = Cast<UK2Node_Variable>(Node))
	{
		SetterNode->CheckForErrors(CompilerContext.GetSchema(), Context.MessageLog);

		if(FBlueprintEditorUtils::IsPropertyReadOnlyInCurrentBlueprint(Context.Blueprint, SetterNode->GetPropertyForVariable()))
		{
			CompilerContext.MessageLog.Warning(*LOCTEXT("BlueprintReadOnlyOrPrivate_Error", "The property is mark as BlueprintReadOnly or Private. It cannot be modifed in the blueprint. @@").ToString(), Node);
		}
	}

	for (int32 PinIndex = 0; PinIndex < Node->Pins.Num(); ++PinIndex)
	{
		UEdGraphPin* Net = Node->Pins[PinIndex];
		if (!CompilerContext.GetSchema()->IsMetaPin(*Net) && (Net->Direction == EGPD_Input))
		{
			if (ValidateAndRegisterNetIfLiteral(Context, Net))
			{
				RegisterNet(Context, Net);
			}
		}
	}
}

void FKCHandler_VariableSet::InnerAssignment(FKismetFunctionContext& Context, UEdGraphNode* Node, UEdGraphPin* VariablePin, UEdGraphPin* ValuePin)
{
	FBPTerminal** VariableTerm = Context.NetMap.Find(VariablePin);
	if (VariableTerm == NULL)
	{
		VariableTerm = Context.NetMap.Find(FEdGraphUtilities::GetNetFromPin(VariablePin));
	}

	FBPTerminal** ValueTerm = Context.LiteralHackMap.Find(ValuePin);
	if (ValueTerm == NULL)
	{
		ValueTerm = Context.NetMap.Find(FEdGraphUtilities::GetNetFromPin(ValuePin));
	}

	if ((VariableTerm != NULL) && (ValueTerm != NULL))
	{
		FBlueprintCompiledStatement& Statement = Context.AppendStatementForNode(Node);

		Statement.Type = KCST_Assignment;
		Statement.LHS = *VariableTerm;
		Statement.RHS.Add(*ValueTerm);

		if (!(*VariableTerm)->IsTermWritable())
		{
			CompilerContext.MessageLog.Error(*LOCTEXT("WriteConst_Error", "Cannot write to const @@").ToString(), VariablePin);
		}
	}
	else
	{
		if (VariablePin != ValuePin)
		{
			CompilerContext.MessageLog.Error(*LOCTEXT("ResolveValueIntoVariablePin_Error", "Failed to resolve term @@ passed into @@").ToString(), ValuePin, VariablePin);
		}
		else
		{
			CompilerContext.MessageLog.Error(*LOCTEXT("ResolveTermPassed_Error", "Failed to resolve term passed into @@").ToString(), VariablePin);
		}
	}
}

void FKCHandler_VariableSet::Compile(FKismetFunctionContext& Context, UEdGraphNode* Node)
{
	// SubCategory is an object type or "" for the stack frame, default scope is Self
	// Each input pin is the name of a variable

	// Each input pin represents an assignment statement
	for (int32 PinIndex = 0; PinIndex < Node->Pins.Num(); ++PinIndex)
	{
		UEdGraphPin* Pin = Node->Pins[PinIndex];

		if (CompilerContext.GetSchema()->IsMetaPin(*Pin))
		{
		}
		else if (Pin->Direction == EGPD_Input)
		{
			InnerAssignment(Context, Node, Pin, Pin);
		}
		else
		{
			CompilerContext.MessageLog.Error(*FString::Printf(*LOCTEXT("ExpectedOnlyInputPins_Error", "Expected only input pins on @@ but found @@").ToString()), Node, Pin);
		}
	}

	// Generate the output impulse from this node
	GenerateSimpleThenGoto(Context, *Node);
}

void FKCHandler_VariableSet::Transform(FKismetFunctionContext& Context, UEdGraphNode* Node)
{
	// Expands node out to include a (local) call to the RepNotify function if necessary
	UK2Node_VariableSet* SetNotify = Cast<UK2Node_VariableSet>(Node);
	if ((SetNotify != NULL))
	{
		if (SetNotify->ShouldFlushDormancyOnSet())
		{
			// Create CallFuncNode
			UK2Node_CallFunction* CallFuncNode = Node->GetGraph()->CreateBlankNode<UK2Node_CallFunction>();
			CallFuncNode->FunctionReference.SetExternalMember(NAME_FlushNetDormancy, AActor::StaticClass() );
			CallFuncNode->AllocateDefaultPins();

			// Copy self pin
			UEdGraphPin* NewSelfPin = CallFuncNode->FindPinChecked(CompilerContext.GetSchema()->PN_Self);
			UEdGraphPin* OldSelfPin = Node->FindPinChecked(CompilerContext.GetSchema()->PN_Self);
			NewSelfPin->CopyPersistentDataFromOldPin(*OldSelfPin);

			// link new CallFuncNode -> Set Node
			UEdGraphPin* OldExecPin = Node->FindPin(CompilerContext.GetSchema()->PN_Execute);
			check(OldExecPin);

			UEdGraphPin* NewExecPin = CallFuncNode->GetExecPin();
			if (ensure(NewExecPin))
			{
				NewExecPin->CopyPersistentDataFromOldPin(*OldExecPin);
				OldExecPin->BreakAllPinLinks();
				CallFuncNode->GetThenPin()->MakeLinkTo(OldExecPin);
			}
		}

		if (SetNotify->HasLocalRepNotify())
		{
			UK2Node_CallFunction* CallFuncNode = Node->GetGraph()->CreateBlankNode<UK2Node_CallFunction>();
			CallFuncNode->FunctionReference.SetExternalMember(SetNotify->GetRepNotifyName(), SetNotify->GetVariableSourceClass() );
			CallFuncNode->AllocateDefaultPins();

			// Copy self pin
			UEdGraphPin* NewSelfPin = CallFuncNode->FindPinChecked(CompilerContext.GetSchema()->PN_Self);
			UEdGraphPin* OldSelfPin = Node->FindPinChecked(CompilerContext.GetSchema()->PN_Self);
			NewSelfPin->CopyPersistentDataFromOldPin(*OldSelfPin);

			// link Set Node -> new CallFuncNode
			UEdGraphPin* OldThenPin = Node->FindPin(CompilerContext.GetSchema()->PN_Then);
			check(OldThenPin);

			UEdGraphPin* NewThenPin = CallFuncNode->GetThenPin();
			if (ensure(NewThenPin))
			{
				// Link Set Node -> Notify
				NewThenPin->CopyPersistentDataFromOldPin(*OldThenPin);
				OldThenPin->BreakAllPinLinks();
				OldThenPin->MakeLinkTo(CallFuncNode->GetExecPin());
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
