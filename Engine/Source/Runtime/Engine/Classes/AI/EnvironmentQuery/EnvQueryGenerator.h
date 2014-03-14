// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvQueryGenerator.generated.h"

UCLASS(Abstract)
class ENGINE_API UEnvQueryGenerator : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditDefaultsOnly, Category=Option)
	FString OptionName;

	/** type of generated items */
	TSubclassOf<class UEnvQueryItemType> ItemType;

	FGenerateItemsSignature GenerateDelegate;

	/** get description of generator */
	virtual FString GetDescriptionTitle() const;
	virtual FString GetDescriptionDetails() const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif //WITH_EDITOR
};
