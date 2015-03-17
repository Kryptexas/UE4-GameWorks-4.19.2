// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/**
 *
 * Helper class to force object references for various reasons.
 */

#pragma once
#include "ObjectReferencer.generated.h"

UCLASS()
class UObjectReferencer : public UObject
{
	GENERATED_BODY()
public:
	UObjectReferencer(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Array of objects being referenced. */
	UPROPERTY(EditAnywhere, Category=ObjectReferencer)
	TArray<class UObject*> ReferencedObjects;

};

