// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TextBlockComponent.generated.h"

UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class UMG_API UTextBlockComponent : public USlateLeafWidgetComponent
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, Category=Content)
	FText Text;

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

	FString GetText() const;
	FSlateColor GetColorAndOpacity() const;
	FLinearColor GetShadowColorAndOpacity() const;
};
