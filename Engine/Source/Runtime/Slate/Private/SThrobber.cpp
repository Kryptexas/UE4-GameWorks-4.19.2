// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"


void SThrobber::Construct(const FArguments& InArgs)
{
	Animate = InArgs._Animate;
	AnimCurves = FCurveSequence();
	{
		for (int32 PieceIndex=0; PieceIndex < InArgs._NumPieces; ++PieceIndex)
		{
			ThrobberCurve.Add( AnimCurves.AddCurve(PieceIndex*0.05f,1.5f) );
		}
		AnimCurves.Play();
	}

	TSharedRef<SHorizontalBox> HBox = SNew(SHorizontalBox);
	
	for (int32 PieceIndex=0; PieceIndex < InArgs._NumPieces; ++PieceIndex)
	{
		HBox->AddSlot()
		.AutoWidth()
		[
			SNew(SBorder)
			.BorderImage( FStyleDefaults::GetNoBrush() )
			.ContentScale( this, &SThrobber::GetPieceScale, PieceIndex )
			.ColorAndOpacity( this, &SThrobber::GetPieceColor, PieceIndex )
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				.Image( InArgs._PieceImage )
			]
		];
	}

	this->ChildSlot
	[
		HBox
	];
}

FVector2D SThrobber::GetPieceScale(int32 PieceIndex) const
{
	const float Value = FMath::Sin( 2 * PI * ThrobberCurve[PieceIndex].GetLerpLooping());
	
	const bool bAnimateHorizontally = ((0 != (Animate & Horizontal)));
	const bool bAnimateVertically = (0 != (Animate & Vertical));
	
	return FVector2D(
		bAnimateHorizontally ? Value : 1.0f,
		bAnimateVertically ? Value : 1.0f
	);
}

FLinearColor SThrobber::GetPieceColor(int32 PieceIndex) const
{
	const bool bAnimateOpacity = (0 != (Animate & Opacity));
	if (bAnimateOpacity)
	{
		const float Value = FMath::Sin( 2 * PI * ThrobberCurve[PieceIndex].GetLerpLooping());
		return FLinearColor(1.0f,1.0f,1.0f, Value);
	}
	else
	{
		return FLinearColor::White;
	}
}


// SCircularThrobber
void SCircularThrobber::Construct(const FArguments& InArgs)
{
	Sequence = FCurveSequence();
	Curve = Sequence.AddCurve(0.f, InArgs._Period);
	Sequence.Play();
	
	PieceImage = InArgs._PieceImage;
	NumPieces = InArgs._NumPieces;
	Radius = InArgs._Radius;
}

int32 SCircularThrobber::OnPaint( const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	const FColor FinalColorAndOpacity( InWidgetStyle.GetColorAndOpacityTint() * PieceImage->GetTint( InWidgetStyle ) );
	
	const float Scale = AllottedGeometry.Scale; 
	const float OffsetX = (AllottedGeometry.Size.X * Scale * 0.5f) - (PieceImage->ImageSize.X * Scale * 0.5f);
	const float OffsetY = (AllottedGeometry.Size.Y * Scale * 0.5f) - (PieceImage->ImageSize.Y * Scale * 0.5f);

	FPaintGeometry PaintGeom = AllottedGeometry.ToPaintGeometry();
	FVector2D Origin = PaintGeom.DrawPosition;
	Origin.X += OffsetX;
	Origin.Y += OffsetY;

	const float DeltaAngle = NumPieces > 0 ? 2 * PI / NumPieces : 0;
	const float Phase = Curve.GetLerpLooping() * 2 * PI;

	for (int32 PieceIdx = 0; PieceIdx < NumPieces; ++PieceIdx)
	{
		PaintGeom.DrawPosition.X = Origin.X + FMath::Sin(DeltaAngle * PieceIdx + Phase) * OffsetX;
		PaintGeom.DrawPosition.Y = Origin.Y + FMath::Cos(DeltaAngle * PieceIdx + Phase) * OffsetY;
		PaintGeom.DrawSize = PieceImage->ImageSize * Scale * (PieceIdx + 1) / NumPieces;

		FSlateDrawElement::MakeBox(OutDrawElements, LayerId, PaintGeom, PieceImage, MyClippingRect, ESlateDrawEffect::None, InWidgetStyle.GetColorAndOpacityTint() );
	}
	
	return LayerId;
}

FVector2D SCircularThrobber::ComputeDesiredSize() const
{
	return FVector2D(Radius, Radius) * 2;
}
