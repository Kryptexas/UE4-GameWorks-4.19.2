// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Gameframework/Actor.h"
#include "CircularDependencyTestActor.generated.h"

UENUM(BlueprintType)
namespace ETestResult
{
	enum Type
	{
		Unknown,
		Failed,
		Succeeded
	};
}

UCLASS(Abstract, Blueprintable, BlueprintType, meta=(ChildCanTick))
class ACircularDependencyTestActor : public AActor
{
	GENERATED_UCLASS_BODY()
public:

	UFUNCTION(BlueprintCallable, Category="Development|Tests")
	bool SetTestState(ETestResult::Type NewState);

	UFUNCTION(BlueprintImplementableEvent, Category="Development|Tests")
	void VisualizeNewTestState(ETestResult::Type OldState, ETestResult::Type NewState);

	UFUNCTION(BlueprintCallable, Category="Development|Tests")
	bool TestVerifyClass(bool bCheckPropertyType = true);

	//UFUNCTION(BlueprintCallable, Category="Development|Tests")
	bool TestVerifyStructMember(UScriptStruct* Struct, uint8* Container, bool bCheckPropertyType = true);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Development|Tests")
	bool RunVerificationTests();

	UFUNCTION(BlueprintCallable, Category="Development|Tests")
	bool TestVerifyIsBlueprintTypeVar(FName VarName, bool bCheckPropertyType);

	UFUNCTION(BlueprintCallable, Category="Development|Tests")
	void ForceResetFailure();

	UFUNCTION(BlueprintCallable, Category="Development|Tests")
	bool CheckObjectValIsBlueprint(UObject* BPObject);

protected:
	UPROPERTY(BlueprintReadOnly, Category="Development|Tests")
	TEnumAsByte<ETestResult::Type> TestState;

private: 
	bool TestVerifyProperty(UProperty* Property, uint8* Container, bool bCheckPropertyType);
};