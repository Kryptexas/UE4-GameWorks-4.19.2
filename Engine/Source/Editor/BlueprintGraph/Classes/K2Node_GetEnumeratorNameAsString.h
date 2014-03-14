// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "K2Node_GetEnumeratorName.h"
#include "K2Node_GetEnumeratorNameAsString.generated.h"

UCLASS(MinimalAPI)
class UK2Node_GetEnumeratorNameAsString : public UK2Node_GetEnumeratorName
{
	GENERATED_UCLASS_BODY()

	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const OVERRIDE{ return TEXT("GraphEditor.Enum_16x"); }
	// End UEdGraphNode interface

	virtual FName GetFunctionName() const OVERRIDE;
};

