// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ImageComponent.generated.h"

/** Image widget */
UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class SCWRUNTIME_API UImageComponent : public USlateLeafWidgetComponent
{
	GENERATED_UCLASS_BODY()

	/** Image to draw */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
	FSlateBrush Image;

	/** Color and opacity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
 	FSlateColor ColorAndOpacity;

protected:
	// USlateWrapperComponent interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of USlateWrapperComponent interface

 	FSlateColor GetColorAndOpacity() const;
};
