// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 *
 * Helper class to force object references for various reasons.
 */

#pragma once
#include "ObjectReferencer.generated.h"

UCLASS()
class UObjectReferencer : public UObject
{
	GENERATED_UCLASS_BODY()

	/** Array of objects being referenced. */
	UPROPERTY(EditAnywhere, Category=ObjectReferencer)
	TArray<class UObject*> ReferencedObjects;

};

