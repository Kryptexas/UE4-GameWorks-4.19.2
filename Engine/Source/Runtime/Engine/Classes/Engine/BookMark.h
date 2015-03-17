// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BookMark.generated.h"


/**
 * A camera position the current level.
 */
UCLASS(hidecategories=Object, MinimalAPI)
class UBookMark : public UObject
{
	GENERATED_BODY()
public:
	ENGINE_API UBookMark(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Camera position */
	UPROPERTY(EditAnywhere, Category=BookMark)
	FVector Location;

	/** Camera rotation */
	UPROPERTY(EditAnywhere, Category=BookMark)
	FRotator Rotation;

	/** Array of levels that are hidden */
	UPROPERTY(EditAnywhere, Category=BookMark)
	TArray<FString> HiddenLevels;
};
