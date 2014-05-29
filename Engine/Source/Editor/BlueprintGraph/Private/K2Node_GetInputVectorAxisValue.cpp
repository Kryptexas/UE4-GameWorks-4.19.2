// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BlueprintGraphPrivatePCH.h"
#include "K2Node_GetInputVectorAxisValue.h"

#include "CompilerResultsLog.h"

UK2Node_GetInputVectorAxisValue::UK2Node_GetInputVectorAxisValue(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bConsumeInput = true;
}

void UK2Node_GetInputVectorAxisValue::Initialize(const FKey AxisKey)
{
	InputAxisKey = AxisKey;
	SetFromFunction(AActor::StaticClass()->FindFunctionByName(TEXT("GetInputVectorAxisValue")));
}

FString UK2Node_GetInputVectorAxisValue::GetTooltip() const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("AxisKey"), InputAxisKey.GetDisplayName());
	return FText::Format(NSLOCTEXT("K2Node", "GetInputVectorAxis_Tooltip", "Returns the current value of input axis key {AxisKey}.  If input is disabled for the actor the value will be (0, 0, 0)."), Args).ToString();
}

void UK2Node_GetInputVectorAxisValue::ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const
{
	// Skip GetInputKeyAxisValue validation
	UK2Node_CallFunction::ValidateNodeDuringCompilation(MessageLog);

	if (!InputAxisKey.IsValid())
	{
		MessageLog.Warning(*FText::Format(NSLOCTEXT("KismetCompiler", "Invalid_GetInputVectorAxis_Warning", "GetInputVectorAxis Value specifies invalid FKey'{0}' for @@"), FText::FromString(InputAxisKey.ToString())).ToString(), this);
	}
	else if (!InputAxisKey.IsVectorAxis())
	{
		MessageLog.Warning(*FText::Format(NSLOCTEXT("KismetCompiler", "NotAxis_GetInputVectorAxis_Warning", "GetInputVectorAxis Value specifies FKey'{0}' which is not a vector axis for @@"), FText::FromString(InputAxisKey.ToString())).ToString(), this);
	}
	else if (!InputAxisKey.IsBindableInBlueprints())
	{
		MessageLog.Warning(*FText::Format(NSLOCTEXT("KismetCompiler", "NotBindanble_GetInputVectorAxis_Warning", "GetInputVectorAxis Value specifies FKey'{0}' that is not blueprint bindable for @@"), FText::FromString(InputAxisKey.ToString())).ToString(), this);
	}
}

UClass* UK2Node_GetInputVectorAxisValue::GetDynamicBindingClass() const
{
	return UInputVectorAxisDelegateBinding::StaticClass();
}

void UK2Node_GetInputVectorAxisValue::RegisterDynamicBinding(UDynamicBlueprintBinding* BindingObject) const
{
	UInputVectorAxisDelegateBinding* InputVectorAxisBindingObject = CastChecked<UInputVectorAxisDelegateBinding>(BindingObject);

	FBlueprintInputAxisKeyDelegateBinding Binding;
	Binding.AxisKey = InputAxisKey;
	Binding.bConsumeInput = bConsumeInput;
	Binding.bExecuteWhenPaused = bExecuteWhenPaused;

	InputVectorAxisBindingObject->InputAxisKeyDelegateBindings.Add(Binding);
}