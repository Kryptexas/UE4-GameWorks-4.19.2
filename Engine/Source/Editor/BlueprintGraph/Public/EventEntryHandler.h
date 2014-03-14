// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "KismetCompiler.h"

//////////////////////////////////////////////////////////////////////////
// FKCHandler_EventEntry

class FKCHandler_EventEntry : public FNodeHandlingFunctor
{
public:
	FKCHandler_EventEntry(FKismetCompilerContext& InCompilerContext)
		: FNodeHandlingFunctor(InCompilerContext)
	{
	}

	virtual void RegisterNet(FKismetFunctionContext& Context, UEdGraphPin* Net) OVERRIDE;
	virtual void Compile(FKismetFunctionContext& Context, UEdGraphNode* Node) OVERRIDE;
};
