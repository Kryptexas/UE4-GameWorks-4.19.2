// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CircularThrobber.generated.h"

class USlateBrushAsset;

/**  A throbber widget that orients images in a spinning circle. */
UCLASS(meta=( Category="Misc" ), ClassGroup=UserInterface)
class UMG_API UCircularThrobber : public UWidget
{
	GENERATED_UCLASS_BODY()

protected:

	/** How many pieces there are */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance, meta=(ClampMin = "1", UIMin = "1", UIMax = "8"))
	int32 NumberOfPieces;

	/** The amount of time for a full circle (in seconds) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	float Period;

	/** The radius of the circle */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	float Radius;

	/** Image to use for each segment of the throbber */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance, meta=( DisplayThumbnail = "true" ))
	USlateBrushAsset* PieceImage;

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface
};
