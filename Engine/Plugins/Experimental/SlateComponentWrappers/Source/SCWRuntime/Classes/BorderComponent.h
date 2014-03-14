// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BorderComponent.generated.h"

/** A border is a container widget that can contain one child widget, providing an opportunity to surround it with a border image and adjustable padding. */

UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class SCWRUNTIME_API UBorderComponent : public USlateNonLeafWidgetComponent
{
	GENERATED_UCLASS_BODY()

	/** Horizontal positioning of the content within the border */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	TEnumAsByte<EHorizontalAlignment> HorizontalAlignment;

	/** Vertical positioning of the content within the border */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	TEnumAsByte<EVerticalAlignment> VerticalAlignment;

	/** The padding to add around the inner content. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
	FMargin ContentPadding;

	//@TODO: Should these be exposed?
	// SLATE_EVENT( FPointerEventHandler, OnMouseButtonDown )
	// SLATE_EVENT( FPointerEventHandler, OnMouseButtonUp )
	// SLATE_EVENT( FPointerEventHandler, OnMouseMove )
	// SLATE_EVENT( FPointerEventHandler, OnMouseDoubleClick )

	/** Image to use for the border */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
	FSlateBrush BorderImage;

	/** The scaling factor for the border */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	FVector2D DesiredSizeScale;

	/** The scaling factor for the border content */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	FVector2D ContentScale;

	/** Color and opacity multiplier of content in the border */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
	FLinearColor ContentColorAndOpacity;

	/** Color and opacity of the actual border image */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
	FSlateColor BorderColorAndOpacity;

	/** The foreground color of text and some glyphs that appear as the border's content. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
	FSlateColor ForegroundColor;

	/** Whether or not to show the disabled effect when this border is disabled */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	bool bShowEffectWhenDisabled;

protected:
	TWeakPtr<class SBorder> MyBorder;

protected:
	// USlateWrapperComponent interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of USlateWrapperComponent interface

	// USlateNonLeafWidgetComponent interface
	virtual void OnKnownChildrenChanged() OVERRIDE;
	// End of USlateNonLeafWidgetComponent

	FMargin GetContentPadding() const;
	FLinearColor GetContentColor() const;
	FSlateColor GetBorderColor() const;
	FSlateColor GetForegroundColor() const;
};
