// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "GameplayEffectCalculation.h"

UGameplayEffectCalculation::UGameplayEffectCalculation(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

const TArray<FGameplayEffectAttributeCaptureDefinition>& UGameplayEffectCalculation::GetAttributeCaptureDefinitions() const
{
	return RelevantAttributesToCapture;
}
