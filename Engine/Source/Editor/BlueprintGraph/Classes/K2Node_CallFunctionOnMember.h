// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_CallFunctionOnMember.generated.h"


UCLASS(MinimalAPI)
class UK2Node_CallFunctionOnMember : public UK2Node_CallFunction
{
	GENERATED_UCLASS_BODY()

	/** Reference to member variable to call function on */
	UPROPERTY()
	FMemberReference				MemberVariableToCallOn;

	// Begin UK2Node_CallFunction interface
	virtual UEdGraphPin* CreateSelfPin(const UFunction* Function) OVERRIDE;
	virtual FString GetFunctionContextString() const OVERRIDE;
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const OVERRIDE;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) OVERRIDE;
	// End UK2Node_CallFunction interface

};

