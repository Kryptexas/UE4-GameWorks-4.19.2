// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"

#include "CompilerResultsLog.h"

UK2Node_InputTouchEvent::UK2Node_InputTouchEvent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bConsumeInput = true;
	bOverrideParentBinding = true;
}

UClass* UK2Node_InputTouchEvent::GetDynamicBindingClass() const
{
	return UInputTouchDelegateBinding::StaticClass();
}

void UK2Node_InputTouchEvent::RegisterDynamicBinding(UDynamicBlueprintBinding* BindingObject) const
{
	UInputTouchDelegateBinding* InputTouchBindingObject = CastChecked<UInputTouchDelegateBinding>(BindingObject);

	FBlueprintInputTouchDelegateBinding Binding;
	Binding.InputKeyEvent = InputKeyEvent;
	Binding.bConsumeInput = bConsumeInput;
	Binding.bExecuteWhenPaused = bExecuteWhenPaused;
	Binding.bOverrideParentBinding = bOverrideParentBinding;
	Binding.FunctionNameToBind = CustomFunctionName;

	InputTouchBindingObject->InputTouchDelegateBindings.Add(Binding);
}
