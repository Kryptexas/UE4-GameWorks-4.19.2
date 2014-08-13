// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CircularThrobber.generated.h"

class USlateBrushAsset;

/**  A throbber widget that orients images in a spinning circle. */
UCLASS(meta=( Category="Misc" ), ClassGroup=UserInterface)
class UMG_API UCircularThrobber : public UWidget
{
	GENERATED_UCLASS_BODY()

public:

	/** Sets how many pieces there are. */
	UFUNCTION(BlueprintCallable, Category = "Appearance")
	void SetNumberOfPieces(int32 InNumberOfPieces);

	/** Sets the amount of time for a full circle (in seconds). */
	UFUNCTION(BlueprintCallable, Category = "Appearance")
	void SetPeriod(float InPeriod);

	/** Sets the radius of the circle. */
	UFUNCTION(BlueprintCallable, Category = "Appearance")
	void SetRadius(float InRadius);

	/** Sets image to use for each segment of the throbber. */
	UFUNCTION(BlueprintCallable, Category = "Appearance")
	void SetPieceImage(USlateBrushAsset* InPieceImage);

	// UWidget interface
	virtual void SynchronizeProperties() override;
	// End of UWidget interface

	virtual void ReleaseNativeWidget() override;

#if WITH_EDITOR
	virtual const FSlateBrush* GetEditorIcon() override;
#endif

protected:

	/** How many pieces there are */
	UPROPERTY(EditDefaultsOnly, Category=Appearance, meta=(ClampMin = "1", UIMin = "1", UIMax = "8"))
	int32 NumberOfPieces;

	/** The amount of time for a full circle (in seconds) */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	float Period;

	/** The radius of the circle */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	float Radius;

	/** Image to use for each segment of the throbber */
	UPROPERTY(EditDefaultsOnly, Category=Appearance, meta=( DisplayThumbnail = "true" ))
	USlateBrushAsset* PieceImage;

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface

private:

	/** Gets the brush used to draw the pieces of the throbber. */
	const FSlateBrush* GetPieceBrush() const;

private:
	/** The CircularThrobber widget managed by this object. */
	TSharedPtr<SCircularThrobber> MyCircularThrobber;
};
