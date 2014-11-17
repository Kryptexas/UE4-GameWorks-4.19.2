// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WidgetBlueprintGeneratedClass.generated.h"

USTRUCT()
struct FDelegateRuntimeBinding
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString ObjectName;

	UPROPERTY()
	FName PropertyName;

	UPROPERTY()
	FName FunctionName;
	
	/*UFunction* UK2Node_CallFunction::GetTargetFunction() const
	{
	UFunction* Function = FunctionReference.ResolveMember<UFunction>(this);
	return Function;
	}*/
};


UCLASS()
class UMG_API UWidgetBlueprintGeneratedClass : public UBlueprintGeneratedClass
{
	GENERATED_UCLASS_BODY()

	/** A tree of the widget templates to be created */
	UPROPERTY()
	class UWidgetTree* WidgetTree;

	UPROPERTY()
	TArray< FDelegateRuntimeBinding > Bindings;

	/** This is transient data calculated at link time. */
	TArray<UStructProperty*> WidgetNodeProperties;

	virtual void CreateComponentsForActor(AActor* Actor) const OVERRIDE;

	virtual void Link(FArchive& Ar, bool bRelinkExistingProperties) OVERRIDE;
};