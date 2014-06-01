// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Spacer.generated.h"

/** A spacer widget; it does not have a visual representation, and just provides padding between other widgets. */
UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class UMG_API USpacer : public UWidget
{
	GENERATED_UCLASS_BODY()

	/** The size of the spacer */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
	FVector2D Size;

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of UWidget interface

	FVector2D GetSpacerSize() const;
};
