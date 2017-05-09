// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "K2Node_GenericCreateObject.h"
#include "Kismet/GameplayStatics.h"
#include "K2Node_CallFunction.h"
#include "KismetCompilerMisc.h"
#include "KismetCompiler.h"

#define LOCTEXT_NAMESPACE "K2Node_GenericCreateObject"

void UK2Node_GenericCreateObject::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	UK2Node_CallFunction* CallCreateNode = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallCreateNode->FunctionReference.SetExternalMember(GET_FUNCTION_NAME_CHECKED(UGameplayStatics, SpawnObject), UGameplayStatics::StaticClass());
	CallCreateNode->AllocateDefaultPins();

	bool bSucceeded = true;
	//connect exe
	{
		UEdGraphPin* SpawnExecPin = GetExecPin();
		UEdGraphPin* CallExecPin = CallCreateNode->GetExecPin();
		bSucceeded &= SpawnExecPin && CallExecPin && CompilerContext.MovePinLinksToIntermediate(*SpawnExecPin, *CallExecPin).CanSafeConnect();
	}

	//connect class
	{
		UEdGraphPin* SpawnClassPin = GetClassPin();
		UEdGraphPin* CallClassPin = CallCreateNode->FindPin(TEXT("ObjectClass"));
		bSucceeded &= SpawnClassPin && CallClassPin && CompilerContext.MovePinLinksToIntermediate(*SpawnClassPin, *CallClassPin).CanSafeConnect();
	}
		
	//connect outer
	{
		UEdGraphPin* SpawnOuterPin = GetOuterPin();
		UEdGraphPin* CallOuterPin = CallCreateNode->FindPin(TEXT("Outer"));
		bSucceeded &= SpawnOuterPin && CallOuterPin && CompilerContext.MovePinLinksToIntermediate(*SpawnOuterPin, *CallOuterPin).CanSafeConnect();
	}

	UEdGraphPin* CallResultPin = nullptr;
	//connect result
	{
		UEdGraphPin* SpawnResultPin = GetResultPin();
		CallResultPin = CallCreateNode->GetReturnValuePin();

		// cast HACK. It should be safe. The only problem is native code generation.
		if (SpawnResultPin && CallResultPin)
		{
			CallResultPin->PinType = SpawnResultPin->PinType;
		}
		bSucceeded &= SpawnResultPin && CallResultPin && CompilerContext.MovePinLinksToIntermediate(*SpawnResultPin, *CallResultPin).CanSafeConnect();
	}

	//assign exposed values and connect then
	{
		UEdGraphPin* LastThen = FKismetCompilerUtilities::GenerateAssignmentNodes(CompilerContext, SourceGraph, CallCreateNode, this, CallResultPin, GetClassToSpawn());
		UEdGraphPin* SpawnNodeThen = GetThenPin();
		bSucceeded &= SpawnNodeThen && LastThen && CompilerContext.MovePinLinksToIntermediate(*SpawnNodeThen, *LastThen).CanSafeConnect();
	}

	BreakAllNodeLinks();

	if (!bSucceeded)
	{
		CompilerContext.MessageLog.Error(*LOCTEXT("GenericCreateObject_Error", "ICE: GenericCreateObject error @@").ToString(), this);
	}
}

void UK2Node_GenericCreateObject::EarlyValidation(class FCompilerResultsLog& MessageLog) const
{
	Super::EarlyValidation(MessageLog);
	UEdGraphPin* ClassPin = GetClassPin(&Pins);
	const bool bAllowAbstract = ClassPin && ClassPin->LinkedTo.Num();
	UClass* ClassToSpawn = GetClassToSpawn();
	if (!UGameplayStatics::CanSpawnObjectOfClass(ClassToSpawn, bAllowAbstract))
	{
		MessageLog.Error(*FString::Printf(*LOCTEXT("GenericCreateObject_WrongClass", "Wrong class to spawn '%s' in @@").ToString(), *GetPathNameSafe(ClassToSpawn)), this);
	}

	UEdGraphPin* OuterPin = GetOuterPin();
	if (!OuterPin || (!OuterPin->DefaultObject && !OuterPin->LinkedTo.Num()))
	{
		MessageLog.Error(*LOCTEXT("GenericCreateObject_NoOuter", "Outer object is required in @@").ToString(), this);
	}
}

bool UK2Node_GenericCreateObject::IsCompatibleWithGraph(const UEdGraph* TargetGraph) const
{
	return UK2Node::IsCompatibleWithGraph(TargetGraph);
}

#undef LOCTEXT_NAMESPACE
