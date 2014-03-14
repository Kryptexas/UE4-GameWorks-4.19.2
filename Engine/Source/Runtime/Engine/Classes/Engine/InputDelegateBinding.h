// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "InputDelegateBinding.generated.h"

UCLASS(abstract)
class ENGINE_API UInputDelegateBinding : public UDynamicBlueprintBinding
{
	GENERATED_UCLASS_BODY()

	virtual void BindToInputComponent(UInputComponent* InputComponent) const { };

	static void BindInputDelegates(UBlueprintGeneratedClass* BGClass, UInputComponent* InputComponent);
};
