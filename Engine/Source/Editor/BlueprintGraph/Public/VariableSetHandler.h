// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

//////////////////////////////////////////////////////////////////////////
// FKCHandler_VariableSet

class FKCHandler_VariableSet : public FNodeHandlingFunctor
{
public:
	FKCHandler_VariableSet(FKismetCompilerContext& InCompilerContext)
		: FNodeHandlingFunctor(InCompilerContext)
	{
	}

	virtual void RegisterNet(FKismetFunctionContext& Context, UEdGraphPin* Net) OVERRIDE;
	virtual void RegisterNets(FKismetFunctionContext& Context, UEdGraphNode* Node) OVERRIDE;
	void InnerAssignment(FKismetFunctionContext& Context, UEdGraphNode* Node, UEdGraphPin* VariablePin, UEdGraphPin* ValuePin);
	virtual void Compile(FKismetFunctionContext& Context, UEdGraphNode* Node) OVERRIDE;
	virtual void Transform(FKismetFunctionContext& Context, UEdGraphNode* Node) OVERRIDE;
};
