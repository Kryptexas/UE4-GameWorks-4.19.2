// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SpinningImageComponent.generated.h"

/** A widget that displays a spinning image. */
UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class UMG_API USpinningImageComponent : public UImage
{
	GENERATED_UCLASS_BODY()

	/** The amount of time in seconds for a full rotation */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	float Period;

	//TODO UMG Set period dynamically.

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of UWidget interface
};
