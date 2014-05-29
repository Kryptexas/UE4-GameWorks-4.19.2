// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "K2Node_GetInputAxisKeyValue.h"
#include "K2Node_GetInputVectorAxisValue.generated.h"

UCLASS(MinimalAPI)
class UK2Node_GetInputVectorAxisValue : public UK2Node_GetInputAxisKeyValue
{
	GENERATED_UCLASS_BODY()

	// Begin EdGraphNode interface
	virtual FString GetTooltip() const OVERRIDE;
	// End EdGraphNode interface

	// Begin UK2Node interface
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const OVERRIDE;
	virtual bool ShouldShowNodeProperties() const OVERRIDE { return true; }
	virtual UClass* GetDynamicBindingClass() const OVERRIDE;
	virtual void RegisterDynamicBinding(UDynamicBlueprintBinding* BindingObject) const OVERRIDE;
	// End UK2Node interface
	
	void Initialize(const FKey AxisKey);
};