// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// USlider

USlider::USlider(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	Orientation = EOrientation::Orient_Horizontal;
	SliderBarColor = FLinearColor::White;
	SliderHandleColor = FLinearColor::White;
}

TSharedRef<SWidget> USlider::RebuildWidget()
{
	// TODO UMG Bind these
	//FOnMouseCaptureBeginEvent OnMouseCaptureBegin;
	//FOnMouseCaptureEndEvent OnMouseCaptureEnd;

	MySlider = SNew(SSlider)
		.Orientation(Orientation)
		.SliderBarColor(SliderBarColor)
		.SliderHandleColor(SliderHandleColor)
		.Value(Value)
		.OnMouseCaptureBegin(BIND_UOBJECT_DELEGATE(FSimpleDelegate, HandleOnMouseCaptureBegin))
		.OnMouseCaptureEnd(BIND_UOBJECT_DELEGATE(FSimpleDelegate, HandleOnMouseCaptureEnd))
		.OnValueChanged(BIND_UOBJECT_DELEGATE(FOnFloatValueChanged, HandleOnValueChanged));

	return MySlider.ToSharedRef();
}

void USlider::HandleOnValueChanged(float InValue)
{
	OnValueChanged.Broadcast(InValue);
}

void USlider::HandleOnMouseCaptureBegin()
{
	OnMouseCaptureBegin.Broadcast();
}

void USlider::HandleOnMouseCaptureEnd()
{
	OnMouseCaptureEnd.Broadcast();
}

float USlider::GetValue()
{
	return MySlider->GetValue();
}

void USlider::SetValue(float InValue)
{
	return MySlider->SetValue(InValue);
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
