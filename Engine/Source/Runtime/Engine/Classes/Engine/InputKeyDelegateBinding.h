// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "InputKeyDelegateBinding.generated.h"

USTRUCT()
struct ENGINE_API FBlueprintInputKeyDelegateBinding
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FInputChord InputChord;

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

	FBlueprintInputKeyDelegateBinding()
		: InputKeyEvent(IE_Pressed)
		, bConsumeInput(true)
		, bExecuteWhenPaused(false)
		, bOverrideParentBinding(true)
		, FunctionNameToBind(NAME_None)
	{
	}
};

UCLASS()
class ENGINE_API UInputKeyDelegateBinding : public UInputDelegateBinding
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TArray<FBlueprintInputKeyDelegateBinding> InputKeyDelegateBindings;

	// Begin UInputDelegateBinding interface
	virtual void BindToInputComponent(UInputComponent* InputComponent) const OVERRIDE;
	// End UInputDelegateBinding interface
};
