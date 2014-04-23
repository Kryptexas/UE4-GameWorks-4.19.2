// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "HorizontalBoxSlot.generated.h"

UCLASS()
class UMG_API UHorizontalBoxSlot : public UPanelSlot
{
	GENERATED_UCLASS_BODY()
	
	UPROPERTY()
	class USlateWrapperComponent* Content;

	UPROPERTY(EditAnywhere, Category=Appearance)
	TEnumAsByte<EHorizontalAlignment> HorizontalAlignment;

	UPROPERTY(EditAnywhere, Category=Appearance)
	TEnumAsByte<EVerticalAlignment> VerticalAlignment;
};