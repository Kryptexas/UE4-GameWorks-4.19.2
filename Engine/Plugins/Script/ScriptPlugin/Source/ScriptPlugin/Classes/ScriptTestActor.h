// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "ScriptTestActor.generated.h"

/** Definition of a specific ability that is applied to a character. Exists as part of a Trait. */
UCLASS(BlueprintType, MinimalAPI)
class AScriptTestActor : public AActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Category = Test, EditAnywhere, BlueprintReadWrite)
	FString TestString;

	UPROPERTY(Category = Test, EditAnywhere, BlueprintReadWrite)
	float TestValue;

	UPROPERTY(Category = Test, EditAnywhere, BlueprintReadWrite)
	bool TestBool;

	UFUNCTION()
	float TestFunction(float InValue, float InFactor, bool bMultiply);
};
