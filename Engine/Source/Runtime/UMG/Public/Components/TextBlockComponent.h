// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TextBlockComponent.generated.h"

UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class UMG_API UTextBlockComponent : public USlateLeafWidgetComponent
{
	GENERATED_UCLASS_BODY()

	DECLARE_DYNAMIC_DELEGATE_RetVal(FText, FTextDelegate);

public:
	UPROPERTY(EditDefaultsOnly, Category=Content)
	FText Text;

	UPROPERTY()
	FTextDelegate TextDelegate;
	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateFontInfo Font;

	UPROPERTY(EditAnywhere, Category=Appearance)
	FVector2D ShadowOffset;

	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateColor ColorAndOpacity;

	UPROPERTY(EditAnywhere, Category=Appearance)
	FLinearColor ShadowColorAndOpacity;

	/** Text to display as the content of the button */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Style, meta=( DisplayThumbnail = "true" ))
	USlateWidgetStyleAsset* Style;

protected:
	// USlateWrapperComponent interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of USlateWrapperComponent interface

	FText GetText() const;
	FSlateColor GetColorAndOpacity() const;
	FLinearColor GetShadowColorAndOpacity() const;
};
