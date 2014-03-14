// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"

void FSlateDrawElement::MakeDebugQuad( FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& PaintGeometry, const FSlateRect& InClippingRect)
{
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.ElementType = ET_DebugQuad;
	DrawElt.Position = PaintGeometry.DrawPosition;
	DrawElt.Size = PaintGeometry.DrawSize;
	DrawElt.ClippingRect = InClippingRect;
	DrawElt.Layer = InLayer;
	DrawElt.DrawEffects = ESlateDrawEffect::None;
	DrawElt.Scale = PaintGeometry.DrawScale;
}

void FSlateDrawElement::MakeBox( 
	FSlateWindowElementList& ElementList,
	uint32 InLayer, 
	const FPaintGeometry& PaintGeometry, 
	const FSlateBrush* InBrush, 
	const FSlateRect& InClippingRect, 
	ESlateDrawEffect::Type InDrawEffects, 
	const FLinearColor& InTint )
{
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.ElementType = (InBrush->DrawAs == ESlateBrushDrawType::Border) ? ET_Border : ET_Box;
	DrawElt.Position = PaintGeometry.DrawPosition;
	DrawElt.Size = PaintGeometry.DrawSize;
	DrawElt.ClippingRect = InClippingRect;
	DrawElt.DataPayload.SetBoxPayloadProperties( InBrush, InTint );
	DrawElt.Layer = InLayer;
	DrawElt.DrawEffects = InDrawEffects;
	DrawElt.Scale = PaintGeometry.DrawScale;
}

void FSlateDrawElement::MakeRotatedBox( 
	FSlateWindowElementList& ElementList,
	uint32 InLayer, 
	const FPaintGeometry& PaintGeometry, 
	const FSlateBrush* InBrush, 
	const FSlateRect& InClippingRect, 
	ESlateDrawEffect::Type InDrawEffects, 
	float Angle,
	TOptional<FVector2D> InRotationPoint,
	ERotationSpace RotationSpace,
	const FLinearColor& InTint )
{
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.ElementType = (InBrush->DrawAs == ESlateBrushDrawType::Border) ? ET_Border : ET_Box;
	DrawElt.Position = PaintGeometry.DrawPosition;
	DrawElt.Size = PaintGeometry.DrawSize;
	DrawElt.ClippingRect = InClippingRect;
	DrawElt.Layer = InLayer;
	DrawElt.DrawEffects = InDrawEffects;
	DrawElt.Scale = PaintGeometry.DrawScale;

	FVector2D RotationPoint = GetRotationPoint( PaintGeometry, InRotationPoint, RotationSpace );
	DrawElt.DataPayload.SetRotatedBoxPayloadProperties( InBrush, Angle, RotationPoint, InTint );
}

void FSlateDrawElement::MakeText( FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& PaintGeometry, const FString& InText, const int32 StartIndex, const int32 EndIndex, const FSlateFontInfo& InFontInfo, const FSlateRect& InClippingRect,ESlateDrawEffect::Type InDrawEffects, const FLinearColor& InTint )
{
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.ElementType = ET_Text;
	DrawElt.Position = PaintGeometry.DrawPosition;
	DrawElt.Size = PaintGeometry.DrawSize;
	DrawElt.ClippingRect = InClippingRect;
	DrawElt.DataPayload.SetTextPayloadProperties( FString( EndIndex - StartIndex, *InText + StartIndex ), InFontInfo, InTint );
	DrawElt.Layer = InLayer;
	DrawElt.DrawEffects = InDrawEffects;
	DrawElt.Scale = PaintGeometry.DrawScale;
}

void FSlateDrawElement::MakeText( FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& PaintGeometry, const FString& InText, const FSlateFontInfo& InFontInfo, const FSlateRect& InClippingRect,ESlateDrawEffect::Type InDrawEffects, const FLinearColor& InTint )
{
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.ElementType = ET_Text;
	DrawElt.Position = PaintGeometry.DrawPosition;
	DrawElt.Size = PaintGeometry.DrawSize;
	DrawElt.ClippingRect = InClippingRect;
	DrawElt.DataPayload.SetTextPayloadProperties( InText, InFontInfo, InTint );
	DrawElt.Layer = InLayer;
	DrawElt.DrawEffects = InDrawEffects;
	DrawElt.Scale = PaintGeometry.DrawScale;
}

void FSlateDrawElement::MakeText( FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& PaintGeometry, const FText& InText, const FSlateFontInfo& InFontInfo, const FSlateRect& InClippingRect,ESlateDrawEffect::Type InDrawEffects, const FLinearColor& InTint )
{
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.ElementType = ET_Text;
	DrawElt.Position = PaintGeometry.DrawPosition;
	DrawElt.Size = PaintGeometry.DrawSize;
	DrawElt.ClippingRect = InClippingRect;
	DrawElt.DataPayload.SetTextPayloadProperties( InText.ToString(), InFontInfo, InTint );
	DrawElt.Layer = InLayer;
	DrawElt.DrawEffects = InDrawEffects;
	DrawElt.Scale = PaintGeometry.DrawScale;
}

void FSlateDrawElement::MakeGradient( FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& InPaintGeometry, TArray<FSlateGradientStop> InGradientStops, EOrientation InGradientType, const FSlateRect& InClippingRect, ESlateDrawEffect::Type InDrawEffects, bool bGammaCorrect )
{
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.ElementType = ET_Gradient;
	DrawElt.Position = InPaintGeometry.DrawPosition;
	DrawElt.Size = InPaintGeometry.DrawSize;
	DrawElt.ClippingRect = InClippingRect;
	DrawElt.DataPayload.SetGradientPayloadProperties( InGradientStops, InGradientType, bGammaCorrect );
	DrawElt.Layer = InLayer;
	DrawElt.DrawEffects = InDrawEffects;
	DrawElt.Scale = InPaintGeometry.DrawScale;
}

void FSlateDrawElement::MakeSpline( FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& PaintGeometry, const FVector2D& InStart, const FVector2D& InStartDir, const FVector2D& InEnd, const FVector2D& InEndDir, const FSlateRect InClippingRect, float InThickness, ESlateDrawEffect::Type InDrawEffects, const FLinearColor& InTint )
{
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.ElementType = ET_Spline;
	DrawElt.Position = PaintGeometry.DrawPosition;
	DrawElt.ClippingRect = InClippingRect;
	DrawElt.DataPayload.SetSplinePayloadProperties( InStart, InStartDir, InEnd, InEndDir, InThickness, InTint );
	DrawElt.Layer = InLayer;
	DrawElt.DrawEffects = InDrawEffects;
	DrawElt.Scale = PaintGeometry.DrawScale;
}

void FSlateDrawElement::MakeDrawSpaceSpline( FSlateWindowElementList& ElementList, uint32 InLayer, const FVector2D& InStart, const FVector2D& InStartDir, const FVector2D& InEnd, const FVector2D& InEndDir, const FSlateRect InClippingRect, float InThickness, ESlateDrawEffect::Type InDrawEffects, const FColor& InTint )
{
	MakeSpline( ElementList, InLayer, FPaintGeometry::Identity(), InStart, InStartDir, InEnd, InEndDir, InClippingRect, InThickness, InDrawEffects, InTint );
}

void FSlateDrawElement::MakeLines( FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& PaintGeometry, const TArray<FVector2D>& Points, const FSlateRect InClippingRect, ESlateDrawEffect::Type InDrawEffects, const FLinearColor& InTint, bool bAntialias )
{
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.ElementType = ET_Line;
	DrawElt.Position = PaintGeometry.DrawPosition;
	DrawElt.ClippingRect = InClippingRect;
	DrawElt.DataPayload.SetLinesPayloadProperties( Points, InTint, bAntialias, ESlateLineJoinType::Sharp );
	DrawElt.Layer = InLayer;
	DrawElt.DrawEffects = InDrawEffects;
	DrawElt.Scale = PaintGeometry.DrawScale;
}

void FSlateDrawElement::MakeViewport( FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& PaintGeometry, TSharedPtr<const ISlateViewport> Viewport, const FSlateRect& InClippingRect, bool bGammaCorrect, bool bAllowBlending, ESlateDrawEffect::Type InDrawEffects, const FLinearColor& InTint )
{
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.ElementType = ET_Viewport;
	DrawElt.Position = PaintGeometry.DrawPosition;
	DrawElt.Size = PaintGeometry.DrawSize;
	DrawElt.ClippingRect = InClippingRect;
	DrawElt.DataPayload.SetViewportPayloadProperties( Viewport, InTint, bGammaCorrect, bAllowBlending );
	DrawElt.Layer = InLayer;
	DrawElt.DrawEffects = InDrawEffects;
	DrawElt.Scale = PaintGeometry.DrawScale;
}
void FSlateDrawElement::MakeCustom( FSlateWindowElementList& ElementList, uint32 InLayer, TSharedPtr<ICustomSlateElement, ESPMode::ThreadSafe> CustomDrawer )
{
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.ElementType = ET_Custom;
	DrawElt.Layer = InLayer;
	DrawElt.DataPayload.SetCustomDrawerPayloadProperties( CustomDrawer );
	DrawElt.ClippingRect = FSlateRect(1,1,1,1);
}