// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


#include "GameplayEffectCalculation.generated.h"

#if GE_REFACTOR

struct FGameplayEffectSpec;
struct FGameplayEffectAttributeCaptureDefinition;

class UAbilitySystemComponent;

UCLASS(abstract)
class GAMEPLAYABILITIES_API UGameplayEffectCalculation : public UObject
{
	GENERATED_UCLASS_BODY()

	virtual void Execute(FGameplayEffectSpec& Spec, int32 ExecutionIdx, UAbilitySystemComponent* TargetAbilitySystemComponent) const;

	virtual void GetCaptureDefinitions(OUT TArray<FGameplayEffectAttributeCaptureDefinition>& Definitions) const;
};

// yuck?

#define GE_REQ_SRC(SET, PROP, SNAPSHOT) \
{ \
	static UProperty* ThisProperty = FindFieldChecked<UProperty>(SET::StaticClass(), GET_MEMBER_NAME_CHECKED(SET, PROP)); \
	Definitions.Add(FGameplayEffectAttributeCaptureDefinition(FGameplayAttribute(ThisProperty), EGameplayEffectAttributeCaptureSource::Source, SNAPSHOT)); \
}

#define GE_REQ_TARGET(SET, PROP, SNAPSHOT) \
{ \
	static UProperty* ThisProperty = FindFieldChecked<UProperty>(SET::StaticClass(), GET_MEMBER_NAME_CHECKED(SET, PROP)); \
	Definitions.Add(FGameplayEffectAttributeCaptureDefinition(FGameplayAttribute(ThisProperty), EGameplayEffectAttributeCaptureSource::Target, SNAPSHOT)); \
}


#endif