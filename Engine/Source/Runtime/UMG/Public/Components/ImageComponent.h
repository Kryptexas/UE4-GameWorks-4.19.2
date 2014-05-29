// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ImageComponent.generated.h"

/** Image widget */
UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class UMG_API UImageComponent : public UWidget
{
	GENERATED_UCLASS_BODY()

	/** Image to draw */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance, meta=( DisplayThumbnail = "true" ))
	USlateBrushAsset* Image;

	/** Color and opacity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
	FSlateColor ColorAndOpacity;

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of UWidget interface

	const FSlateBrush* GetImageBrush() const;
	FSlateColor GetColorAndOpacity() const;
};
