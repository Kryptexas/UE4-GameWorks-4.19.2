// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TextBlockComponent.generated.h"

UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class UMG_API UTextBlockComponent : public USlateLeafWidgetComponent
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, Category=TextBlock)
	FText Text;

	UPROPERTY(EditAnywhere, Category=TextBlock)
	FSlateFontInfo Font;

	UPROPERTY(EditAnywhere, Category=TextBlock)
	FVector2D ShadowOffset;

	//@TODO: Expose FSlateColor
	UPROPERTY(EditAnywhere, Category=TextBlock)
	FLinearColor ColorAndOpacity;

	UPROPERTY(EditAnywhere, Category=TextBlock)
	FLinearColor ShadowColorAndOpacity;

protected:
	// USlateWrapperComponent interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of USlateWrapperComponent interface

	FString GetText() const;
	FSlateColor GetColorAndOpacity() const;
	FLinearColor GetShadowColorAndOpacity() const;
};
