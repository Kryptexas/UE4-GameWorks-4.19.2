// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "DynamicCastHandler.h"

UK2Node_CastToInterface::UK2Node_CastToInterface(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FNodeHandlingFunctor* UK2Node_CastToInterface::CreateNodeHandler(FKismetCompilerContext& CompilerContext) const
{
	return new FKCHandler_DynamicCast(CompilerContext, KCST_CastToInterface);
}
