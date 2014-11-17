// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "InputActionDelegateBinding.generated.h"

USTRUCT()
struct ENGINE_API FBlueprintInputActionDelegateBinding : public FBlueprintInputDelegateBinding
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FName InputActionName;

	UPROPERTY()
	TEnumAsByte<EInputEvent> InputKeyEvent;

	UPROPERTY()
	FName FunctionNameToBind;

	FBlueprintInputActionDelegateBinding()
		: FBlueprintInputDelegateBinding()
		, InputActionName(NAME_None)
		, InputKeyEvent(IE_Pressed)
		, FunctionNameToBind(NAME_None)
	{
	}
};

UCLASS()
class ENGINE_API UInputActionDelegateBinding : public UInputDelegateBinding
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TArray<FBlueprintInputActionDelegateBinding> InputActionDelegateBindings;

	// Begin UInputDelegateBinding interface
	virtual void BindToInputComponent(UInputComponent* InputComponent) const OVERRIDE;
	// End UInputDelegateBinding interface
};
