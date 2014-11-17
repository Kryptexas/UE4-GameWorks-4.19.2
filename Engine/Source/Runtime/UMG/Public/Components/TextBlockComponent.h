// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TextBlockComponent.generated.h"

UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class UMG_API UTextBlockComponent : public USlateLeafWidgetComponent
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditDefaultsOnly, Category=Content)
	FText Text;

	UPROPERTY()
	FGetText TextDelegate;
	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateFontInfo Font;

	UPROPERTY(EditAnywhere, Category=Appearance)
	FVector2D ShadowOffset;

	UPROPERTY(EditAnywhere, Category=Appearance)
	FLinearColor ColorAndOpacity;

	UPROPERTY()
	FGetSlateColor ColorAndOpacityDelegate;

	UPROPERTY(EditAnywhere, Category=Appearance)
	FLinearColor ShadowColorAndOpacity;

	UPROPERTY()
	FGetLinearColor ShadowColorAndOpacityDelegate;

	/** Text to display as the content of the button */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Style, meta=( DisplayThumbnail = "true" ))
	USlateWidgetStyleAsset* Style;

protected:
	// USlateWrapperComponent interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of USlateWrapperComponent interface

	FSlateColor GetColorAndOpacity() const;
	FLinearColor GetShadowColorAndOpacity() const;
};
