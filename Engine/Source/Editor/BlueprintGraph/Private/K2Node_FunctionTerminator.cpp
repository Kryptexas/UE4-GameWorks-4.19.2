// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "BlueprintGraphPrivatePCH.h"

UK2Node_FunctionTerminator::UK2Node_FunctionTerminator(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FLinearColor UK2Node_FunctionTerminator::GetNodeTitleColor() const
{
	return GEditor->AccessEditorUserSettings().FunctionTerminatorNodeTitleColor;
}
