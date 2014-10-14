// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WidgetNavigation.generated.h"

/**
 *
 */
USTRUCT()
struct FWidgetNavigationData
{
	GENERATED_USTRUCT_BODY()

public:
	//UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Navigation")
	//TEnumAsByte<> Rule;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Navigation")
	FName WidgetToFocus;
};

/**
 * 
 */
UCLASS()
class UMG_API UWidgetNavigation : public UObject
{
	GENERATED_BODY()
	
public:
	/**  */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Navigation")
	FWidgetNavigationData Up;

	/**  */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Navigation")
	FWidgetNavigationData Down;

	/**  */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Navigation")
	FWidgetNavigationData Left;

	/**  */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Navigation")
	FWidgetNavigationData Right;
};
