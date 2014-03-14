// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"

#include "EngineLevelScriptClasses.h"

UK2Node_InputKeyEvent::UK2Node_InputKeyEvent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bConsumeInput = true;
	bOverrideParentBinding = true;
}

UClass* UK2Node_InputKeyEvent::GetDynamicBindingClass() const
{
	return UInputKeyDelegateBinding::StaticClass();
}

void UK2Node_InputKeyEvent::RegisterDynamicBinding(UDynamicBlueprintBinding* BindingObject) const
{
	UInputKeyDelegateBinding* InputKeyBindingObject = CastChecked<UInputKeyDelegateBinding>(BindingObject);

	FBlueprintInputKeyDelegateBinding Binding;
	Binding.InputChord = InputChord;
	Binding.InputKeyEvent = InputKeyEvent;
	Binding.bConsumeInput = bConsumeInput;
	Binding.bExecuteWhenPaused = bExecuteWhenPaused;
	Binding.bOverrideParentBinding = bOverrideParentBinding;
	Binding.FunctionNameToBind = CustomFunctionName;

	InputKeyBindingObject->InputKeyDelegateBindings.Add(Binding);
}
