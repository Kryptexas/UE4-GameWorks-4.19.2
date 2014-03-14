// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TextBlockComponent.generated.h"

UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class SCWRUNTIME_API UTextBlockComponent : public USlateLeafWidgetComponent
{
	GENERATED_UCLASS_BODY()

protected:
	UPROPERTY(EditAnywhere, Category=TextBlock)
	FText DisplayText;

	//@TODO: Expose FSlateColor
	UPROPERTY(EditAnywhere, Category=TextBlock)
	FLinearColor ColorAndOpacity;

	UPROPERTY(EditAnywhere, Category=TextBlock)
	FLinearColor ShadowColorAndOpacity;

protected:
	// USlateWrapperComponent interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of USlateWrapperComponent interface

	FString GetDisplayText() const;
	FSlateColor GetColorAndOpacity() const;
	FLinearColor GetShadowColorAndOpacity() const;
};
