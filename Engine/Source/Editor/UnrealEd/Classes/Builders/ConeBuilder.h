// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// ConeBuilder: Builds a 3D cone brush, compatible with cylinder of same size.
//=============================================================================

#pragma once
#include "ConeBuilder.generated.h"

UCLASS(MinimalAPI, autoexpandcategories=BrushSettings, EditInlineNew, meta=(DisplayName="Cone"))
class UConeBuilder : public UEditorBrushBuilder
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=BrushSettings, meta=(ClampMin = "0.000001"))
	float Z;

	UPROPERTY(EditAnywhere, Category=BrushSettings)
	float CapZ;

	UPROPERTY(EditAnywhere, Category=BrushSettings, meta=(ClampMin = "0.000001"))
	float OuterRadius;

	UPROPERTY(EditAnywhere, Category=BrushSettings)
	float InnerRadius;

	UPROPERTY(EditAnywhere, Category=BrushSettings, meta=(ClampMin = "3", ClampMax = "500"))
	int32 Sides;

	UPROPERTY()
	FName GroupName;

	UPROPERTY(EditAnywhere, Category=BrushSettings)
	uint32 AlignToSide:1;

	UPROPERTY(EditAnywhere, Category=BrushSettings)
	uint32 Hollow:1;


	// Begin UBrushBuilder Interface
	virtual bool Build( UWorld* InWorld, ABrush* InBrush = NULL ) OVERRIDE;
	// End UBrushBuilder Interface

	// @todo document
	virtual void BuildCone( int32 Direction, bool InAlignToSide, int32 InSides, float InZ, float Radius, FName Item );
};



