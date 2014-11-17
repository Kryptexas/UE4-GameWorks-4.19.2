// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"

#include "CompilerResultsLog.h"

UK2Node_InputVectorAxisEvent::UK2Node_InputVectorAxisEvent(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
	EventSignatureName = TEXT("InputVectorAxisHandlerDynamicSignature__DelegateSignature");
}

void UK2Node_InputVectorAxisEvent::ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const
{
	// Skip AxisKeyEvent validation
	UK2Node_Event::ValidateNodeDuringCompilation(MessageLog);

	if (!AxisKey.IsValid())
	{
		MessageLog.Warning(*FText::Format(NSLOCTEXT("KismetCompiler", "Invalid_InputVectorAxis_Warning", "InputVectorAxis Event specifies invalid FKey'{0}' for @@"), FText::FromString(AxisKey.ToString())).ToString(), this);
	}
	else if (!AxisKey.IsVectorAxis())
	{
		MessageLog.Warning(*FText::Format(NSLOCTEXT("KismetCompiler", "NotAxis_InputVectorAxis_Warning", "InputVectorAxis Event specifies FKey'{0}' which is not a vector axis for @@"), FText::FromString(AxisKey.ToString())).ToString(), this);
	}
	else if (!AxisKey.IsBindableInBlueprints())
	{
		MessageLog.Warning(*FText::Format(NSLOCTEXT("KismetCompiler", "NotBindable_InputVectorAxis_Warning", "InputVectorAxis Event specifies FKey'{0}' that is not blueprint bindable for @@"), FText::FromString(AxisKey.ToString())).ToString(), this);
	}
}

UClass* UK2Node_InputVectorAxisEvent::GetDynamicBindingClass() const
{
	return UInputVectorAxisDelegateBinding::StaticClass();
}

void UK2Node_InputVectorAxisEvent::RegisterDynamicBinding(UDynamicBlueprintBinding* BindingObject) const
{
	UInputVectorAxisDelegateBinding* InputVectorAxisBindingObject = CastChecked<UInputVectorAxisDelegateBinding>(BindingObject);

	FBlueprintInputAxisKeyDelegateBinding Binding;
	Binding.AxisKey = AxisKey;
	Binding.bConsumeInput = bConsumeInput;
	Binding.bExecuteWhenPaused = bExecuteWhenPaused;
	Binding.bOverrideParentBinding = bOverrideParentBinding;
	Binding.FunctionNameToBind = CustomFunctionName;

	InputVectorAxisBindingObject->InputAxisKeyDelegateBindings.Add(Binding);
}