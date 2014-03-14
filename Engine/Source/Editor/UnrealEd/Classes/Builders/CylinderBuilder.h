// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 *
 */


//=============================================================================
// CylinderBuilder: Builds a 3D cylinder brush.
//=============================================================================

#pragma once
#include "CylinderBuilder.generated.h"

UCLASS(MinimalAPI, autoexpandcategories=BrushSettings, EditInlineNew, meta=(DisplayName="Cylinder"))
class UCylinderBuilder : public UEditorBrushBuilder
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=BrushSettings, meta=(ClampMin = "0.000001"))
	float Z;

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
	virtual void BuildCylinder( int32 Direction, bool InAlignToSide, int32 InSides, float InZ, float Radius );
};



