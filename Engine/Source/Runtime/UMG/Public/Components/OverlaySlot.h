// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateWrapperTypes.h"

#include "OverlaySlot.generated.h"

UCLASS()
class UMG_API UOverlaySlot : public UPanelSlot
{
	GENERATED_UCLASS_BODY()
	
	UPROPERTY(EditAnywhere, Category=Appearance)
	FMargin Padding;

	UPROPERTY(EditAnywhere, Category=Appearance)
	TEnumAsByte<EHorizontalAlignment> HorizontalAlignment;

	UPROPERTY(EditAnywhere, Category=Appearance)
	TEnumAsByte<EVerticalAlignment> VerticalAlignment;
};
