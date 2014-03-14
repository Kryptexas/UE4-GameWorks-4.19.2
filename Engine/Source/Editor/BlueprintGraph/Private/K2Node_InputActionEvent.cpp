// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"

#include "EngineLevelScriptClasses.h"

UK2Node_InputActionEvent::UK2Node_InputActionEvent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bConsumeInput = true;
	bOverrideParentBinding = true;
}

UClass* UK2Node_InputActionEvent::GetDynamicBindingClass() const
{
	return UInputActionDelegateBinding::StaticClass();
}

void UK2Node_InputActionEvent::RegisterDynamicBinding(UDynamicBlueprintBinding* BindingObject) const
{
	UInputActionDelegateBinding* InputActionBindingObject = CastChecked<UInputActionDelegateBinding>(BindingObject);

	FBlueprintInputActionDelegateBinding Binding;
	Binding.InputActionName = InputActionName;
	Binding.InputKeyEvent = InputKeyEvent;
	Binding.bConsumeInput = bConsumeInput;
	Binding.bExecuteWhenPaused = bExecuteWhenPaused;
	Binding.bOverrideParentBinding = bOverrideParentBinding;
	Binding.FunctionNameToBind = CustomFunctionName;

	InputActionBindingObject->InputActionDelegateBindings.Add(Binding);
}