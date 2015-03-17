// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateCore.h"
#include "ButtonStyleAsset.generated.h"

/**
 * An asset describing a button's appearance.
 * Just a wrapper for the struct with real data in it.style factory
 */
UCLASS(hidecategories=Object, MinimalAPI)
class UButtonStyleAsset : public UObject
{
	GENERATED_BODY()
public:
	ENGINE_API UButtonStyleAsset(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:
	/** The actual data describing the button's appearance. */
	UPROPERTY(Category=Appearance, EditAnywhere, meta=(ShowOnlyInnerProperties))
	FButtonStyle ButtonStyle;
};
