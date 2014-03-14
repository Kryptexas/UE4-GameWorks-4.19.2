// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SpacerComponent.generated.h"

/** A spacer widget; it does not have a visual representation, and just provides padding between other widgets. */
UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class SCWRUNTIME_API USpacerComponent : public USlateWrapperComponent
{
	GENERATED_UCLASS_BODY()

	/** The size of the spacer */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
	FVector2D Size;

protected:
	// USlateWrapperComponent interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of USlateWrapperComponent interface

	FVector2D GetSpacerSize() const;
};
