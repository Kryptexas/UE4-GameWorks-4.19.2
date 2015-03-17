// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Engine/InputAxisKeyDelegateBinding.h"
#include "InputVectorAxisDelegateBinding.generated.h"

UCLASS()
class ENGINE_API UInputVectorAxisDelegateBinding : public UInputAxisKeyDelegateBinding
{
	GENERATED_BODY()
public:
	UInputVectorAxisDelegateBinding(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Begin UInputDelegateBinding interface
	virtual void BindToInputComponent(UInputComponent* InputComponent) const override;
	// End UInputDelegateBinding interface
};
