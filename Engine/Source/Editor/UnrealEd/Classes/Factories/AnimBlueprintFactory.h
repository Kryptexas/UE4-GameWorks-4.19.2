// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 *
 */

#pragma once
#include "AnimBlueprintFactory.generated.h"

UCLASS(HideCategories=Object,MinimalAPI)
class UAnimBlueprintFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	// The type of blueprint that will be created
	UPROPERTY(EditAnywhere, Category=AnimBlueprintFactory)
	TEnumAsByte<enum EBlueprintType> BlueprintType;

	// The parent class of the created blueprint
	UPROPERTY(EditAnywhere, Category=AnimBlueprintFactory, meta=(AllowAbstract = ""))
	TSubclassOf<class UAnimInstance> ParentClass;

	// The kind of skeleton that animation graphs compiled from the blueprint will animate
	UPROPERTY(EditAnywhere, Category=AnimBlueprintFactory)
	class USkeleton* TargetSkeleton;

	// Begin UFactory Interface
	virtual bool ConfigureProperties() OVERRIDE;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext) OVERRIDE;
	virtual UObject* FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn) OVERRIDE;
	// Begin UFactory Interface	
};

