// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Border.generated.h"

class USlateBrushAsset;

/**
 * A border is a container widget that can contain one child widget, providing an opportunity 
 * to surround it with a border image and adjustable padding.
 */
UCLASS(meta=( Category="Common" ), ClassGroup=UserInterface)
class UMG_API UBorder : public UContentWidget
{
	GENERATED_UCLASS_BODY()

	/** Horizontal positioning of the content within the border */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Content Layout")
	TEnumAsByte<EHorizontalAlignment> HorizontalAlignment;

	/** Vertical positioning of the content within the border */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Content Layout")
	TEnumAsByte<EVerticalAlignment> VerticalAlignment;

	/** The padding to add around the inner content. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Content Layout")
	FMargin ContentPadding;

	/** The scaling factor for the border content */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Content Layout")
	FVector2D ContentScale;

	/** Color and opacity multiplier of content in the border */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Content Layout")
	FLinearColor ContentColorAndOpacity;

	//@TODO: Should these be exposed?
	// SLATE_EVENT( FPointerEventHandler, OnMouseButtonDown )
	// SLATE_EVENT( FPointerEventHandler, OnMouseButtonUp )
	// SLATE_EVENT( FPointerEventHandler, OnMouseMove )
	// SLATE_EVENT( FPointerEventHandler, OnMouseDoubleClick )

	/** Image to use for the border */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance, meta=( DisplayThumbnail = "true" ))
	USlateBrushAsset* BorderBrush;

	/** The scaling factor for the border */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	FVector2D DesiredSizeScale;

	/** Color and opacity of the actual border image */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
	FLinearColor BorderColorAndOpacity;

	/** The foreground color of text and some glyphs that appear as the border's content. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
	FLinearColor ForegroundColor;

	/** Whether or not to show the disabled effect when this border is disabled */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	bool bShowEffectWhenDisabled;

	// UContentWidget interface
	virtual void SetContent(UWidget* Content) override;
	// End UContentWidget interface

	// UWidget interface
	void SyncronizeProperties() override;
	// End of UWidget interface

protected:
	TSharedPtr<SBorder> MyBorder;

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface

	const FSlateBrush* GetBorderBrush() const;
};
