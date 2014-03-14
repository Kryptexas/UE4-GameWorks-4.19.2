// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * CubeBuilder: Builds a 3D cube brush.
 */

#pragma once
#include "CubeBuilder.generated.h"

UCLASS(MinimalAPI, autoexpandcategories=BrushSettings, EditInlineNew, meta=(DisplayName="Box"))
class UCubeBuilder : public UEditorBrushBuilder
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=BrushSettings, meta=(ClampMin = "0.000001"))
	float X;

	UPROPERTY(EditAnywhere, Category=BrushSettings, meta=(ClampMin = "0.000001"))
	float Y;

	UPROPERTY(EditAnywhere, Category=BrushSettings, meta=(ClampMin = "0.000001"))
	float Z;

	UPROPERTY(EditAnywhere, Category=BrushSettings)
	float WallThickness;

	UPROPERTY()
	FName GroupName;

	UPROPERTY(EditAnywhere, Category=BrushSettings)
	uint32 Hollow:1;

	UPROPERTY(EditAnywhere, Category=BrushSettings)
	uint32 Tessellated:1;


	// Begin UBrushBuilder Interface
	virtual bool Build( UWorld* InWorld, ABrush* InBrush = NULL ) OVERRIDE;
	// End UBrushBuilder Interface

	// @todo document
	virtual void BuildCube( int32 Direction, float dx, float dy, float dz, bool _tessellated );
};



