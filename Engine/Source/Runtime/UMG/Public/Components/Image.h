// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Image.generated.h"

class USlateBrushAsset;

/**
 * The image widget allows you to display a Slate Brush, or texture or material in the UI.
 */
UCLASS(meta=( Category="Common" ), ClassGroup=UserInterface)
class UMG_API UImage : public UWidget
{
	GENERATED_UCLASS_BODY()

public:

	/** Image to draw */
	UPROPERTY(EditDefaultsOnly, Category=Appearance, meta=( DisplayThumbnail = "true" ))
	USlateBrushAsset* Image;

	/** A bindable delegate for the Image. */
	UPROPERTY()
	FGetSlateBrushAsset ImageDelegate;

	/** Color and opacity */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FLinearColor ColorAndOpacity;

	/** A bindable delegate for the ColorAndOpacity. */
	UPROPERTY()
	FGetSlateColor ColorAndOpacityDelegate;

	UPROPERTY(EditDefaultsOnly, Category=Events)
	FOnPointerEvent OnMouseButtonDownEvent;

public:

	/**  */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetColorAndOpacity(FLinearColor InColorAndOpacity);

	/**  */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetOpacity(float InOpacity);

	/**  */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetImage(USlateBrushAsset* InImage);

	/**  */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetImageFromBrush(FSlateBrush Brush);

	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetImageFromTexture(UTexture2D* Texture);

	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetImageFromMaterial(UMaterialInterface* Material);

	/**  */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	UMaterialInstanceDynamic* GetDynamicMaterial();

	// UWidget interface
	virtual void SynchronizeProperties() override;
	// End of UWidget interface

	virtual void ReleaseNativeWidget() override;

#if WITH_EDITOR
	// UWidget interface
	virtual const FSlateBrush* GetEditorIcon() override;
	// End UWidget interface
#endif

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface

	const FSlateBrush* GetImageBrush() const;

	const FSlateBrush* ConvertImage(TAttribute<USlateBrushAsset*> InImageAsset) const;

	FReply HandleMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent);

protected:
	TSharedPtr<SImage> MyImage;

	TOptional<FSlateBrush> DynamicBrush;
};
