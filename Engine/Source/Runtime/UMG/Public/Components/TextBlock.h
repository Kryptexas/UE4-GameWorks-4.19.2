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

	/** The color of the text */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FLinearColor ColorAndOpacity;

	/** A bindable delegate for the ColorAndOpacity. */
	UPROPERTY()
	FGetSlateColor ColorAndOpacityDelegate;

	/** The style to use to render the text */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Style, meta=( DisplayThumbnail = "true" ))
	USlateWidgetStyleAsset* Style;

	//TODO UMG Set text via UFunction.
	//TODO UMG set all these properties by UFunction as well.

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of UWidget interface

	/** Converts the bound FLinearColor to a slate color for the ColorAndOpacityDelegate */
	FSlateColor GetColorAndOpacity() const;

	/** Converts the bound FLinearColor to a slate color for the ShadowColorAndOpacityDelegate */
	FLinearColor GetShadowColorAndOpacity() const;
};
