// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Attribute.h"
#include "Input/Reply.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SLeafWidget.h"
#include "Framework/SlateDelegates.h"
#include "SCompoundWidget.h"

/** Callback to get the current FVector4 value */
DECLARE_DELEGATE_OneParam(FOnGetCurrentVector4Value, FVector4&)

/**
* Enumerates color picker modes.
*/
enum class EColorGradingModes
{
	Saturation,
	Contrast,
	Gamma,
	Gain,
	Offset
};


/**
 * Class for placing a color picker. If all you need is a standalone color picker,
 * use the functions OpenColorGradingWheel and DestroyColorGradingWheel, since they hold a static
 * instance of the color picker.
 */
class APPFRAMEWORK_API SColorGradingPicker
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SColorGradingPicker)
		: _ValuesMin(0.0f)
		, _ValuesMax(2.0f)
		, _MainDelta(0.01f)
		, _MainShiftMouseMovePixelPerDelta(10)
		, _ColorGradingModes(EColorGradingModes::Saturation)
		, _OnColorCommitted()
		, _OnQueryCurrentColor()
	{ }
		
		SLATE_ARGUMENT( float, ValuesMin )

		SLATE_ARGUMENT( float, ValuesMax )

		SLATE_ARGUMENT( float, MainDelta )

		SLATE_ARGUMENT( int32, MainShiftMouseMovePixelPerDelta)

		SLATE_ARGUMENT( EColorGradingModes, ColorGradingModes)

		/** The event called when the color is committed */
		SLATE_EVENT( FOnVector4ValueChanged, OnColorCommitted )

			/** Callback to get the current FVector4 value */
		SLATE_EVENT(FOnGetCurrentVector4Value, OnQueryCurrentColor)

	SLATE_END_ARGS()

	/**	Destructor. */
	~SColorGradingPicker();

public:

	/**
	 * Construct the widget
	 *
	 * @param InArgs Declaration from which to construct the widget.
	 */
	void Construct(const FArguments& InArgs );

protected:

	void TransformLinearColorRangeToColorGradingRange(FVector4 &VectorValue) const;
	void TransformColorGradingRangeToLinearColorRange(FVector4 &VectorValue) const;
	void TransformColorGradingRangeToLinearColorRange(float &FloatValue);

	TOptional<float> OnGetMainValue() const;
	void OnMainValueChanged(float InValue);
	FLinearColor GetCurrentLinearColor();

	// Callback for value changes in the color spectrum picker.
	void HandleCurrentColorValueChanged(FLinearColor NewValue);

	float ValuesMin;
	float ValuesMax;
	float MainDelta;
	int32 MainShiftMouseMovePixelPerDelta;
	EColorGradingModes ColorGradingModes;

	/** Invoked when a new value is selected on the color wheel */
	FOnVector4ValueChanged OnColorCommitted;

	FOnGetCurrentVector4Value OnQueryCurrentColor;
};
