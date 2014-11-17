// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateWrapperTypes.h"

#include "HorizontalBoxSlot.generated.h"

UCLASS()
class UMG_API UHorizontalBoxSlot : public UPanelSlot
{
	GENERATED_UCLASS_BODY()
	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FMargin Padding;

	/** How much space this slot should occupy in the direction of the panel. */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FSlateChildSize Size;

	UPROPERTY(EditAnywhere, Category=Appearance)
	TEnumAsByte<EHorizontalAlignment> HorizontalAlignment;

	UPROPERTY(EditAnywhere, Category=Appearance)
	TEnumAsByte<EVerticalAlignment> VerticalAlignment;
};