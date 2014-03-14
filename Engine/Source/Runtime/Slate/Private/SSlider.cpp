// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"


void SSlider::Construct( const SSlider::FArguments& InDeclaration )
{
	check(InDeclaration._Style);

	DisabledHandleImage = &InDeclaration._Style->DisabledThumbImage;
	NormalHandleImage = &InDeclaration._Style->NormalThumbImage;
	IndentHandle = InDeclaration._IndentHandle;
	LockedAttribute = InDeclaration._Locked;
	Orientation = InDeclaration._Orientation;
	ValueAttribute = InDeclaration._Value;
	SliderBarColor = InDeclaration._SliderBarColor;
	SliderHandleColor = InDeclaration._SliderHandleColor;
	OnMouseCaptureBegin = InDeclaration._OnMouseCaptureBegin;
	OnMouseCaptureEnd = InDeclaration._OnMouseCaptureEnd;

	OnValueChanged = InDeclaration._OnValueChanged;
}


int32 SSlider::OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
#if SLATE_HD_STATS
	SCOPE_CYCLE_COUNTER(STAT_SlateOnPaint_SSlider);
#endif

	float HandleRotation;
	FVector2D HandleTopLeftPoint;
	FVector2D SliderStartPoint;
	FVector2D SliderEndPoint;

	// calculate slider geometry
	const FVector2D HalfHandleSize = 0.5f * NormalHandleImage->ImageSize;
	const float Indentation = IndentHandle.Get() ? NormalHandleImage->ImageSize.X : 0.0f;

	if (Orientation == Orient_Horizontal)
	{
		const float SliderLength = AllottedGeometry.Size.X - Indentation;
		const float SliderHandleOffset = ValueAttribute.Get() * SliderLength;
		const float SliderY = 0.5f * AllottedGeometry.Size.Y;

		HandleRotation = 0.0f;
		HandleTopLeftPoint = FVector2D(SliderHandleOffset - HalfHandleSize.X + 0.5f * Indentation, SliderY - HalfHandleSize.Y);

		SliderStartPoint = FVector2D(HalfHandleSize.X, SliderY);
		SliderEndPoint = FVector2D(AllottedGeometry.Size.X - HalfHandleSize.X, SliderY);
	}
	else
	{
		const float SliderLength = AllottedGeometry.Size.Y - Indentation;
		const float SliderHandleOffset = ValueAttribute.Get() * SliderLength;
		const float SliderX = 0.5f * AllottedGeometry.Size.X;

		HandleRotation = -0.5f * PI;
		HandleTopLeftPoint = FVector2D(SliderX - HalfHandleSize.X, AllottedGeometry.Size.Y - SliderHandleOffset - HalfHandleSize.Y - 0.5f * Indentation);

		SliderStartPoint = FVector2D(SliderX, AllottedGeometry.Size.Y - HalfHandleSize.X);
		SliderEndPoint = FVector2D(SliderX, HalfHandleSize.X);
	}

	// draw slider bar
	TArray<FVector2D> LinePoints;
	LinePoints.Add(SliderStartPoint);
	LinePoints.Add(SliderEndPoint);

	const bool bEnabled = ShouldBeEnabled(bParentEnabled);
	const ESlateDrawEffect::Type DrawEffects = bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

	FSlateDrawElement::MakeLines( 
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(),
		LinePoints,
		MyClippingRect,
		DrawEffects,
		SliderBarColor.Get().GetColor(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint()
	);

	++LayerId;

	// draw slider handle
	FSlateDrawElement::MakeRotatedBox( 
		OutDrawElements,
		LayerId,
		AllottedGeometry.ToPaintGeometry(HandleTopLeftPoint, NormalHandleImage->ImageSize),
		LockedAttribute.Get() ? DisabledHandleImage : NormalHandleImage,
		MyClippingRect,
		DrawEffects,
		HandleRotation,
		TOptional<FVector2D>(),
		FSlateDrawElement::RelativeToElement,
		SliderHandleColor.Get().GetColor(InWidgetStyle) * InWidgetStyle.GetColorAndOpacityTint()
	);

	return LayerId;
}


FVector2D SSlider::ComputeDesiredSize() const
{
	static const FVector2D SSliderDesiredSize(16.0f, 16.0f);

	if (NormalHandleImage == nullptr)
	{
		return SSliderDesiredSize;
	}

	if (Orientation == Orient_Vertical)
	{
		return FVector2D(NormalHandleImage->ImageSize.Y, SSliderDesiredSize.Y);
	}

	return FVector2D(SSliderDesiredSize.X, NormalHandleImage->ImageSize.Y);
}


FReply SSlider::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if ((MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) && !LockedAttribute.Get())
	{
		OnMouseCaptureBegin.ExecuteIfBound();
		CommitValue(PositionToValue(MyGeometry, MouseEvent.GetLastScreenSpacePosition()));

		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	return FReply::Unhandled();
}


FReply SSlider::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if ((MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) && HasMouseCapture())
	{
		SetCursor(EMouseCursor::Default);
		OnMouseCaptureEnd.ExecuteIfBound();

		return FReply::Handled().ReleaseMouseCapture();	
	}

	return FReply::Unhandled();
}


FReply SSlider::OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if (this->HasMouseCapture() && !LockedAttribute.Get())
	{
		SetCursor((Orientation == Orient_Horizontal) ? EMouseCursor::ResizeLeftRight : EMouseCursor::ResizeUpDown);
		CommitValue(PositionToValue(MyGeometry, MouseEvent.GetLastScreenSpacePosition()));

		return FReply::Handled();
	}

	return FReply::Unhandled();
}


void SSlider::CommitValue(float NewValue)
{
	if (!ValueAttribute.IsBound())
	{
		ValueAttribute.Set(NewValue);
	}

	OnValueChanged.ExecuteIfBound(NewValue);
}


float SSlider::PositionToValue( const FGeometry& MyGeometry, const FVector2D& AbsolutePosition )
{
	const FVector2D LocalPosition = MyGeometry.AbsoluteToLocal(AbsolutePosition);
	const float Indentation = IndentHandle.Get() ? NormalHandleImage->ImageSize.X : 0.0f;

	float RelativeValue;

	if (Orientation == Orient_Horizontal)
	{
		RelativeValue = (LocalPosition.X - 0.5f * Indentation) / (MyGeometry.Size.X - Indentation);
	}
	else
	{
		RelativeValue = (MyGeometry.Size.Y - LocalPosition.Y - 0.5f * Indentation) / (MyGeometry.Size.Y - Indentation);
	}
	
	return FMath::Clamp(RelativeValue, 0.0f, 1.0f);
}
