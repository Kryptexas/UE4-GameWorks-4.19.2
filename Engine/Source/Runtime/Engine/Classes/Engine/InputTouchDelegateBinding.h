// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "InputTouchDelegateBinding.generated.h"

USTRUCT()
struct ENGINE_API FBlueprintInputTouchDelegateBinding
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TEnumAsByte<EInputEvent> InputKeyEvent;

	UPROPERTY()
	uint32 bConsumeInput:1;

	UPROPERTY()
	uint32 bExecuteWhenPaused:1;

	UPROPERTY()
	uint32 bOverrideParentBinding:1;

	UPROPERTY()
	FName FunctionNameToBind;

	FBlueprintInputTouchDelegateBinding()
		: InputKeyEvent(IE_Pressed)
		, bConsumeInput(true)
		, bExecuteWhenPaused(false)
		, bOverrideParentBinding(true)
		, FunctionNameToBind(NAME_None)
	{
	}
};


UCLASS()
class ENGINE_API UInputTouchDelegateBinding : public UInputDelegateBinding
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TArray<FBlueprintInputTouchDelegateBinding> InputTouchDelegateBindings;

	// Begin UInputDelegateBinding interface
	virtual void BindToInputComponent(UInputComponent* InputComponent) const OVERRIDE;
	// End UInputDelegateBinding interface
};
