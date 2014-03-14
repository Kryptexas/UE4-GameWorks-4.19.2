// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "KismetCompiler.h"

//////////////////////////////////////////////////////////////////////////
// FKCHandler_DynamicCast

class FKCHandler_DynamicCast : public FNodeHandlingFunctor
{
protected:
	TMap<UEdGraphNode*, FBPTerminal*> BoolTermMap;
	EKismetCompiledStatementType CastType;

public:
	FKCHandler_DynamicCast(FKismetCompilerContext& InCompilerContext, EKismetCompiledStatementType InCastType)
		: FNodeHandlingFunctor(InCompilerContext)
		, CastType(InCastType)
	{
	}

	virtual void RegisterNets(FKismetFunctionContext& Context, UEdGraphNode* Node) OVERRIDE;
	virtual void RegisterNet(FKismetFunctionContext& Context, UEdGraphPin* Net) OVERRIDE;
	virtual void Compile(FKismetFunctionContext& Context, UEdGraphNode* Node) OVERRIDE;
};
