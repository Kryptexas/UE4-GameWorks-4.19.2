// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "SColorGradingPicker.h"
#include "SColorGradingWheel.h"
#include "SNumericEntryBox.h"
#include "SBox.h"


#define LOCTEXT_NAMESPACE "ColorGradingWheel"

SColorGradingPicker::~SColorGradingPicker()
{
}

void SColorGradingPicker::Construct( const FArguments& InArgs )
{
	ValuesMin = InArgs._ValuesMin;
	ValuesMax = InArgs._ValuesMax;
	MainDelta = InArgs._MainDelta;
	MainShiftMouseMovePixelPerDelta = InArgs._MainShiftMouseMovePixelPerDelta;
	ColorGradingModes = InArgs._ColorGradingModes;
	OnColorCommitted = InArgs._OnColorCommitted;
	OnQueryCurrentColor = InArgs._OnQueryCurrentColor;
	
	float ColorGradingWheelExponent = 2.4f;
	if (ColorGradingModes == EColorGradingModes::Offset)
	{
		ColorGradingWheelExponent = 3.0f;
	}

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.WidthOverride(125.0f)
				.HeightOverride(125.0f)
				.MinDesiredWidth(125.0f)
				.MaxDesiredWidth(125.0f)
				[
					SNew(SColorGradingWheel)
					.SelectedColor(TAttribute<FLinearColor>::Create(TAttribute<FLinearColor>::FGetter::CreateSP(this, &SColorGradingPicker::GetCurrentLinearColor)))
					.DesiredWheelSize(125)
					.ExponentDisplacement(ColorGradingWheelExponent)
					.OnValueChanged(this, &SColorGradingPicker::HandleCurrentColorValueChanged)
				]
			]
		+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SNumericEntryBox<float>)
				.EditableTextBoxStyle(&FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("DarkEditableTextBox"))
				.Value(this, &SColorGradingPicker::OnGetMainValue)
				.OnValueChanged(this, &SColorGradingPicker::OnMainValueChanged)
				.AllowSpin(true)
				.MinValue(ValuesMin)
				.MaxValue(ValuesMax)
				.MinSliderValue(ValuesMin)
				.MaxSliderValue(ValuesMax)
				.Delta(MainDelta)
				.ShiftMouseMovePixelPerDelta(MainShiftMouseMovePixelPerDelta)
			]
	];
}

void SColorGradingPicker::OnMainValueChanged(float InValue)
{
	TransformColorGradingRangeToLinearColorRange(InValue);

	FVector4 CurrentValue(0.0f, 0.0f, 0.0f, 0.0f);
	OnQueryCurrentColor.ExecuteIfBound(CurrentValue);
	TransformColorGradingRangeToLinearColorRange(CurrentValue);
	
	
	//The MainValue is the maximum of any channel value
	float MaxCurrentValue = FMath::Max3(CurrentValue.X, CurrentValue.Y, CurrentValue.Z);
	if (MaxCurrentValue <= 0.0f)
	{
		//We need the neutral value for the type of color grading, currently only offset is an addition(0.0) all other are multiplier(1.0)
		CurrentValue = FVector4(InValue, InValue, InValue, CurrentValue.W);
	}
	else
	{
		float Ratio = InValue / MaxCurrentValue;
		CurrentValue *= FVector4(Ratio, Ratio, Ratio, 1.0f);
	}
	TransformLinearColorRangeToColorGradingRange(CurrentValue);
	OnColorCommitted.ExecuteIfBound(CurrentValue);
}

TOptional<float> SColorGradingPicker::OnGetMainValue() const
{
	FVector4 CurrentValue(0.0f, 0.0f, 0.0f, 0.0f);
	OnQueryCurrentColor.ExecuteIfBound(CurrentValue);
	//The MainValue is the maximum of any channel value
	TOptional<float> CurrentValueOption = FMath::Max3(CurrentValue.X, CurrentValue.Y, CurrentValue.Z);
	return CurrentValueOption;
}

void SColorGradingPicker::TransformLinearColorRangeToColorGradingRange(FVector4 &VectorValue) const
{
	float Ratio = (ValuesMax - ValuesMin);
	VectorValue *= Ratio;
	VectorValue += FVector4(ValuesMin, ValuesMin, ValuesMin, ValuesMin);
}

void SColorGradingPicker::TransformColorGradingRangeToLinearColorRange(FVector4 &VectorValue) const
{
	VectorValue += -FVector4(ValuesMin, ValuesMin, ValuesMin, ValuesMin);
	float Ratio = 1.0f/(ValuesMax - ValuesMin);
	VectorValue *= Ratio;
}

void SColorGradingPicker::TransformColorGradingRangeToLinearColorRange(float &FloatValue)
{
	FloatValue -= ValuesMin;
	float Ratio = 1.0f/(ValuesMax - ValuesMin);
	FloatValue *= Ratio;
}

FLinearColor SColorGradingPicker::GetCurrentLinearColor()
{
	FLinearColor CurrentColor;
	FVector4 CurrentValue;
	if (OnQueryCurrentColor.ExecuteIfBound(CurrentValue))
	{
		TransformColorGradingRangeToLinearColorRange(CurrentValue);
		CurrentColor = FLinearColor(CurrentValue.X, CurrentValue.Y, CurrentValue.Z);
	}
	return CurrentColor.LinearRGBToHSV();
}

void SColorGradingPicker::HandleCurrentColorValueChanged( FLinearColor NewValue )
{
	//Query the current vector4 so we can pass back the w value
	FVector4 CurrentValue(0.0f, 0.0f, 0.0f, 0.0f);
	OnQueryCurrentColor.ExecuteIfBound(CurrentValue);

	FLinearColor NewValueRGB = NewValue.HSVToLinearRGB();
	FVector4 NewValueVector(NewValueRGB.R, NewValueRGB.G, NewValueRGB.B);
	TransformLinearColorRangeToColorGradingRange(NewValueVector);
	//Set the W with the original value
	NewValueVector.W = CurrentValue.W;
	OnColorCommitted.ExecuteIfBound(NewValueVector);
}

#undef LOCTEXT_NAMESPACE
