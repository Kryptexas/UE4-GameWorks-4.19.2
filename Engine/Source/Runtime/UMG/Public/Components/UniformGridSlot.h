// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateWrapperTypes.h"

#include "UniformGridSlot.generated.h"

UCLASS()
class UMG_API UUniformGridSlot : public UPanelSlot
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Appearance)
	TEnumAsByte<EHorizontalAlignment> HorizontalAlignment;

	UPROPERTY(EditAnywhere, Category=Appearance)
	TEnumAsByte<EVerticalAlignment> VerticalAlignment;
	
	UPROPERTY(EditAnywhere, Category=Appearance)
	int32 Row;
	
	UPROPERTY(EditAnywhere, Category=Appearance)
	int32 Column;
};