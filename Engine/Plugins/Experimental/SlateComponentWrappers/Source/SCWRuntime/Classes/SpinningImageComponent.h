// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SpinningImageComponent.generated.h"

/** A widget that displays a spinning image. */
UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class SCWRUNTIME_API USpinningImageComponent : public UImageComponent
{
	GENERATED_UCLASS_BODY()

	/** The amount of time in seconds for a full rotation */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	float Period;

protected:
	// USlateWrapperComponent interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of USlateWrapperComponent interface
};
