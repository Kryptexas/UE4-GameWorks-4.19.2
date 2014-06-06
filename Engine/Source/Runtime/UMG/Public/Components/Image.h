// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Image.generated.h"

class USlateBrushAsset;

/** Image widget */
UCLASS(meta=( Category="Common" ), ClassGroup=UserInterface)
class UMG_API UImage : public UWidget
{
	GENERATED_UCLASS_BODY()

public:

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
