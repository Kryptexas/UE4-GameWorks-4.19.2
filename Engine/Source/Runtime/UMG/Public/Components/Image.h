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
	UPROPERTY(EditDefaultsOnly, Category=Appearance, meta=( DisplayThumbnail = "true" ))
	USlateBrushAsset* Image;

	/** Color and opacity */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FLinearColor ColorAndOpacity;

	UPROPERTY(EditDefaultsOnly, Category=Events)
	FOnPointerEvent OnMouseButtonDownEvent;

	/**  */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetColorAndOpacity(FLinearColor InColorAndOpacity);

	/**  */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetImage(USlateBrushAsset* InImage);

	// UWidget interface
	virtual void SyncronizeProperties() override;
	// End of UWidget interface

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface

	const FSlateBrush* GetImageBrush() const;

	FReply HandleMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent);

protected:
	TSharedPtr<SImage> MyImage;
};
