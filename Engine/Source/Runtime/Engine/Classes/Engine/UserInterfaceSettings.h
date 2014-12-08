// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Curves/CurveFloat.h"
#include "UserInterfaceSettings.generated.h"


/** The Side to use when scaling the UI. */
UENUM()
namespace EUIScalingRule
{
	enum Type
	{
		/** Evaluates the scale curve based on the shortest side of the viewport */
		ShortestSide,
		/** Evaluates the scale curve based on the longest side of the viewport */
		LongestSide,
		/** Evaluates the scale curve based on the X axis of the viewport */
		Horizontal,
		/** Evaluates the scale curve based on the Y axis of the viewport */
		Vertical,
		/** Custom - Allows custom rule interpretation */
		//Custom
	};
}


/**
 * Implements user interface related settings.
 */
UCLASS(config=Engine, defaultconfig)
class ENGINE_API UUserInterfaceSettings
	: public UObject
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(config, EditAnywhere, Category="DPI Scaling", meta=(
		DisplayName="DPI Scale Rule",
		ToolTip="The rule used when trying to decide what scale to apply." ))
	TEnumAsByte<EUIScalingRule::Type> UIScaleRule;

	UPROPERTY(config, EditAnywhere, Category="DPI Scaling", meta=(
		DisplayName="DPI Curve",
		ToolTip="Controls how the UI is scaled at different resolutions based on the DPI Scale Rule",
		XAxisName="Resolution",
		YAxisName="Scale"))
	FRuntimeFloatCurve UIScaleCurve;

public:

	/** Gets the current scale of the UI based on the size */
	float GetDPIScaleBasedOnSize(FIntPoint Size) const;

#if WITH_EDITOR
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent);
#endif
};
