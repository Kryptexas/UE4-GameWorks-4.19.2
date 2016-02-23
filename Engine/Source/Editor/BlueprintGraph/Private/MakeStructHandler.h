// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "KismetCompilerMisc.h"

//////////////////////////////////////////////////////////////////////////
// FKCHandler_MakeStruct

class FKCHandler_MakeStruct : public FNodeHandlingFunctor
{
public:
	FKCHandler_MakeStruct(FKismetCompilerContext& InCompilerContext);

	virtual UEdGraphPin* FindStructPinChecked(UEdGraphNode* Node) const;

	virtual void RegisterNets(FKismetFunctionContext& Context, UEdGraphNode* InNode) override;

	virtual void RegisterNet(FKismetFunctionContext& Context, UEdGraphPin* Net) override;
	virtual FBPTerminal* RegisterLiteral(FKismetFunctionContext& Context, UEdGraphPin* Net) override;

	virtual void Compile(FKismetFunctionContext& Context, UEdGraphNode* InNode) override;
};