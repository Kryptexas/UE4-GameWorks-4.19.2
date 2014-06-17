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
	UPROPERTY(EditDefaultsOnly, Category="Content Layout")
	TEnumAsByte<EHorizontalAlignment> HorizontalAlignment;

	/** Vertical positioning of the content within the border */
	UPROPERTY(EditDefaultsOnly, Category="Content Layout")
	TEnumAsByte<EVerticalAlignment> VerticalAlignment;

	/** The padding to add around the inner content. */
	UPROPERTY(EditDefaultsOnly, Category="Content Layout")
	FMargin ContentPadding;

	/** Color and opacity multiplier of content in the border */
	UPROPERTY(EditDefaultsOnly, Category="Content Layout")
	FLinearColor ContentColorAndOpacity;

	/** The scaling factor for the border content */
	UPROPERTY(EditDefaultsOnly, Category="Content Layout", AdvancedDisplay)
	FVector2D ContentScale;

	/** Image to use for the border */
	UPROPERTY(EditDefaultsOnly, Category=Appearance, meta=( DisplayThumbnail = "true" ))
	USlateBrushAsset* BorderBrush;

	/** The scaling factor for the border */
	UPROPERTY(EditDefaultsOnly, Category=Appearance, AdvancedDisplay)
	FVector2D DesiredSizeScale;

	/** Color and opacity of the actual border image */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FLinearColor BorderColorAndOpacity;

	/** The foreground color of text and some glyphs that appear as the border's content. */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FLinearColor ForegroundColor;

	/** Whether or not to show the disabled effect when this border is disabled */
	UPROPERTY(EditDefaultsOnly, Category=Appearance, AdvancedDisplay)
	bool bShowEffectWhenDisabled;

	UPROPERTY(EditDefaultsOnly, Category=Events)
	FOnPointerEvent OnMouseButtonDownEvent;

	UPROPERTY(EditDefaultsOnly, Category=Events)
	FOnPointerEvent OnMouseButtonUpEvent;

	UPROPERTY(EditDefaultsOnly, Category=Events)
	FOnPointerEvent OnMouseMoveEvent;

	UPROPERTY(EditDefaultsOnly, Category=Events)
	FOnPointerEvent OnMouseDoubleClickEvent;

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

	FReply HandleMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent);
	FReply HandleMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& MouseEvent);
	FReply HandleMouseMove(const FGeometry& Geometry, const FPointerEvent& MouseEvent);
	FReply HandleMouseDoubleClick(const FGeometry& Geometry, const FPointerEvent& MouseEvent);

	const FSlateBrush* GetBorderBrush() const;
};
