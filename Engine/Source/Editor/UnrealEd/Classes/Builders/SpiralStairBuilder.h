// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// SpiralStairBuilder: Builds a spiral staircase.
//=============================================================================

#pragma once
#include "SpiralStairBuilder.generated.h"

UCLASS(MinimalAPI, autoexpandcategories=BrushSettings, EditInlineNew, meta=(DisplayName="Spiral Stair"))
class USpiralStairBuilder : public UEditorBrushBuilder
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=BrushSettings, meta=(ClampMin = "1"))
	int32 InnerRadius;

	UPROPERTY(EditAnywhere, Category=BrushSettings, meta=(ClampMin = "1"))
	int32 StepWidth;

	UPROPERTY(EditAnywhere, Category=BrushSettings, meta=(ClampMin = "1"))
	int32 StepHeight;

	UPROPERTY(EditAnywhere, Category=BrushSettings, meta=(ClampMin = "1"))
	int32 StepThickness;

	UPROPERTY(EditAnywhere, Category=BrushSettings, meta=(ClampMin = "3"))
	int32 NumStepsPer360;

	UPROPERTY(EditAnywhere, Category=BrushSettings, meta=(ClampMin = "1", ClampMax = "100"))
	int32 NumSteps;

	UPROPERTY()
	FName GroupName;

	UPROPERTY(EditAnywhere, Category=BrushSettings)
	uint32 SlopedCeiling:1;

	UPROPERTY(EditAnywhere, Category=BrushSettings)
	uint32 SlopedFloor:1;

	UPROPERTY(EditAnywhere, Category=BrushSettings)
	uint32 CounterClockwise:1;


	// Begin UBrushBuilder Interface
	virtual bool Build( UWorld* InWorld, ABrush* InBrush = NULL ) OVERRIDE;
	// End UBrushBuilder Interface

	// @todo document
	virtual void BuildCurvedStair( int32 Direction );
};



