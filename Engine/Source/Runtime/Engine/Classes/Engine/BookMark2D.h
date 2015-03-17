// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BookMark2D.generated.h"


/**
 * Simple class to store 2D camera information.
 */
UCLASS(hidecategories=Object)
class UBookMark2D
	: public UObject
{
	GENERATED_BODY()
public:
	UBookMark2D(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Zoom of the camera */
	UPROPERTY(EditAnywhere, Category=BookMark2D)
	float Zoom2D;

	/** Location of the camera */
	UPROPERTY(EditAnywhere, Category=BookMark2D)
	FIntPoint Location;
};
