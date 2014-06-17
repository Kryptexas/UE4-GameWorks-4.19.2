// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TextBlock.generated.h"

/** A simple static text widget */
UCLASS(meta=( Category="Common" ), ClassGroup=UserInterface )
class UMG_API UTextBlock : public UWidget
{
	GENERATED_UCLASS_BODY()

public:
	/** The text to display */
	UPROPERTY(EditDefaultsOnly, Category=Content)
	FText Text;
	
	/** A bindable delegate to allow logic to drive the text of the widget */
	UPROPERTY()
	FGetText TextDelegate;

	/** The color of the text */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FLinearColor ColorAndOpacity;

	/** A bindable delegate for the ColorAndOpacity. */
	UPROPERTY()
	FGetSlateColor ColorAndOpacityDelegate;
	
	/** The font to render the text with */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FSlateFontInfo Font;

	/** The direction the shadow is cast */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FVector2D ShadowOffset;

	/** The color of the shadow */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FLinearColor ShadowColorAndOpacity;

	/** A bindable delegate for the ShadowColorAndOpacity. */
	UPROPERTY()
	FGetLinearColor ShadowColorAndOpacityDelegate;

	/** Whether text wraps onto a new line when it's length exceeds this width; if this value is zero or negative, no wrapping occurs. */
	UPROPERTY(EditDefaultsOnly, Category=Appearance, AdvancedDisplay)
	float WrapTextAt;

	/** True if we're wrapping text automatically based on the computed horizontal space for this widget */
	UPROPERTY(EditDefaultsOnly, Category=Appearance, AdvancedDisplay)
	bool AutoWrapText;

	/** The style to use to render the text */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Style, meta=( DisplayThumbnail = "true" ))
	USlateWidgetStyleAsset* Style;

	///** Called when this text is double clicked */
	//SLATE_EVENT(FOnClicked, OnDoubleClicked)

	// UWidget interface
	void SyncronizeProperties() override;
	// End of UWidget interface

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface

protected:
	TSharedPtr<STextBlock> MyTextBlock;
};
