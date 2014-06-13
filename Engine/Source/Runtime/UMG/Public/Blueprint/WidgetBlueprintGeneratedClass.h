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

class UMovieScene;

UCLASS()
class UMG_API UWidgetBlueprintGeneratedClass : public UBlueprintGeneratedClass
{
	GENERATED_UCLASS_BODY()

	/** A tree of the widget templates to be created */
	UPROPERTY()
	class UWidgetTree* WidgetTree;

	UPROPERTY()
	TArray< FDelegateRuntimeBinding > Bindings;

	UPROPERTY()
	TArray< UMovieScene* > AnimationData;

	/** This is transient data calculated at link time. */
	TArray<UStructProperty*> WidgetNodeProperties;

	virtual void Link(FArchive& Ar, bool bRelinkExistingProperties) override;

	void InitializeWidget(class UUserWidget* Actor) const;
};