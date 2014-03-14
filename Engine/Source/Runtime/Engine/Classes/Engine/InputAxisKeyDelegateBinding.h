// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "InputAxisKeyDelegateBinding.generated.h"

USTRUCT()
struct ENGINE_API FBlueprintInputAxisKeyDelegateBinding
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FKey AxisKey;

	UPROPERTY()
	FName FunctionNameToBind;

	UPROPERTY()
	uint32 bConsumeInput:1;

	UPROPERTY()
	uint32 bExecuteWhenPaused:1;

	UPROPERTY()
	uint32 bOverrideParentBinding:1;

	FBlueprintInputAxisKeyDelegateBinding()
		: FunctionNameToBind(NAME_None)
		, bConsumeInput(true)
		, bExecuteWhenPaused(false)
		, bOverrideParentBinding(true)
	{
	}
};

UCLASS()
class ENGINE_API UInputAxisKeyDelegateBinding : public UInputDelegateBinding
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TArray<FBlueprintInputAxisKeyDelegateBinding> InputAxisKeyDelegateBindings;

	// Begin UInputDelegateBinding interface
	virtual void BindToInputComponent(UInputComponent* InputComponent) const OVERRIDE;
	// End UInputDelegateBinding interface
};
