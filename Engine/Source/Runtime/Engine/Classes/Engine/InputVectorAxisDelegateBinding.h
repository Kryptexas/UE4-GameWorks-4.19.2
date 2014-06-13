// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "InputVectorAxisDelegateBinding.generated.h"

UCLASS()
class ENGINE_API UInputVectorAxisDelegateBinding : public UInputAxisKeyDelegateBinding
{
	GENERATED_UCLASS_BODY()

	// Begin UInputDelegateBinding interface
	virtual void BindToInputComponent(UInputComponent* InputComponent) const override;
	// End UInputDelegateBinding interface
};
