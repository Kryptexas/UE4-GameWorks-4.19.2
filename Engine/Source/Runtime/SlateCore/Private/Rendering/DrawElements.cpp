// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateCorePrivatePCH.h"


void FSlateDrawElement::MakeDebugQuad( FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& PaintGeometry, const FSlateRect& InClippingRect)
{
	PaintGeometry.CommitTransformsIfUsingLegacyConstructor();
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.ElementType = ET_DebugQuad;
	DrawElt.RenderTransform = PaintGeometry.GetAccumulatedRenderTransform();
	DrawElt.Position = PaintGeometry.DrawPosition;
	DrawElt.LocalSize = PaintGeometry.GetLocalSize();
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
	PaintGeometry.CommitTransformsIfUsingLegacyConstructor();
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.ElementType = (InBrush->DrawAs == ESlateBrushDrawType::Border) ? ET_Border : ET_Box;
	DrawElt.RenderTransform = PaintGeometry.GetAccumulatedRenderTransform();
	DrawElt.Position = PaintGeometry.DrawPosition;
	DrawElt.LocalSize = PaintGeometry.GetLocalSize();
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
	PaintGeometry.CommitTransformsIfUsingLegacyConstructor();
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.ElementType = (InBrush->DrawAs == ESlateBrushDrawType::Border) ? ET_Border : ET_Box;
	DrawElt.RenderTransform = PaintGeometry.GetAccumulatedRenderTransform();
	DrawElt.Position = PaintGeometry.DrawPosition;
	DrawElt.LocalSize = PaintGeometry.GetLocalSize();
	DrawElt.ClippingRect = InClippingRect;
	DrawElt.Layer = InLayer;
	DrawElt.DrawEffects = InDrawEffects;
	DrawElt.Scale = PaintGeometry.DrawScale;

	FVector2D RotationPoint = GetRotationPoint( PaintGeometry, InRotationPoint, RotationSpace );
	DrawElt.DataPayload.SetRotatedBoxPayloadProperties( InBrush, Angle, RotationPoint, InTint );
}


void FSlateDrawElement::MakeText( FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& PaintGeometry, const FString& InText, const int32 StartIndex, const int32 EndIndex, const FSlateFontInfo& InFontInfo, const FSlateRect& InClippingRect,ESlateDrawEffect::Type InDrawEffects, const FLinearColor& InTint )
{
	PaintGeometry.CommitTransformsIfUsingLegacyConstructor();
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.ElementType = ET_Text;
	DrawElt.RenderTransform = PaintGeometry.GetAccumulatedRenderTransform();
	DrawElt.Position = PaintGeometry.DrawPosition;
	DrawElt.LocalSize = PaintGeometry.GetLocalSize();
	DrawElt.ClippingRect = InClippingRect;
	DrawElt.DataPayload.SetTextPayloadProperties( FString( EndIndex - StartIndex, *InText + StartIndex ), InFontInfo, InTint );
	DrawElt.Layer = InLayer;
	DrawElt.DrawEffects = InDrawEffects;
	DrawElt.Scale = PaintGeometry.DrawScale;
}


void FSlateDrawElement::MakeText( FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& PaintGeometry, const FString& InText, const FSlateFontInfo& InFontInfo, const FSlateRect& InClippingRect,ESlateDrawEffect::Type InDrawEffects, const FLinearColor& InTint )
{
	PaintGeometry.CommitTransformsIfUsingLegacyConstructor();
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.ElementType = ET_Text;
	DrawElt.RenderTransform = PaintGeometry.GetAccumulatedRenderTransform();
	DrawElt.Position = PaintGeometry.DrawPosition;
	DrawElt.LocalSize = PaintGeometry.GetLocalSize();
	DrawElt.ClippingRect = InClippingRect;
	DrawElt.DataPayload.SetTextPayloadProperties( InText, InFontInfo, InTint );
	DrawElt.Layer = InLayer;
	DrawElt.DrawEffects = InDrawEffects;
	DrawElt.Scale = PaintGeometry.DrawScale;
}


void FSlateDrawElement::MakeText( FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& PaintGeometry, const FText& InText, const FSlateFontInfo& InFontInfo, const FSlateRect& InClippingRect,ESlateDrawEffect::Type InDrawEffects, const FLinearColor& InTint )
{
	PaintGeometry.CommitTransformsIfUsingLegacyConstructor();
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.ElementType = ET_Text;
	DrawElt.RenderTransform = PaintGeometry.GetAccumulatedRenderTransform();
	DrawElt.Position = PaintGeometry.DrawPosition;
	DrawElt.LocalSize = PaintGeometry.GetLocalSize();
	DrawElt.ClippingRect = InClippingRect;
	DrawElt.DataPayload.SetTextPayloadProperties( InText.ToString(), InFontInfo, InTint );
	DrawElt.Layer = InLayer;
	DrawElt.DrawEffects = InDrawEffects;
	DrawElt.Scale = PaintGeometry.DrawScale;
}

void FSlateDrawElement::MakeGradient( FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& PaintGeometry, TArray<FSlateGradientStop> InGradientStops, EOrientation InGradientType, const FSlateRect& InClippingRect, ESlateDrawEffect::Type InDrawEffects, bool bGammaCorrect )
{
	PaintGeometry.CommitTransformsIfUsingLegacyConstructor();
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.ElementType = ET_Gradient;
	DrawElt.RenderTransform = PaintGeometry.GetAccumulatedRenderTransform();
	DrawElt.Position = PaintGeometry.DrawPosition;
	DrawElt.LocalSize = PaintGeometry.GetLocalSize();
	DrawElt.ClippingRect = InClippingRect;
	DrawElt.DataPayload.SetGradientPayloadProperties( InGradientStops, InGradientType, bGammaCorrect );
	DrawElt.Layer = InLayer;
	DrawElt.DrawEffects = InDrawEffects;
	DrawElt.Scale = PaintGeometry.DrawScale;
}


void FSlateDrawElement::MakeSpline( FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& PaintGeometry, const FVector2D& InStart, const FVector2D& InStartDir, const FVector2D& InEnd, const FVector2D& InEndDir, const FSlateRect InClippingRect, float InThickness, ESlateDrawEffect::Type InDrawEffects, const FLinearColor& InTint )
{
	PaintGeometry.CommitTransformsIfUsingLegacyConstructor();
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.ElementType = ET_Spline;
	DrawElt.RenderTransform = PaintGeometry.GetAccumulatedRenderTransform();
	DrawElt.Position = PaintGeometry.DrawPosition;
	DrawElt.LocalSize = PaintGeometry.GetLocalSize();
	DrawElt.ClippingRect = InClippingRect;
	DrawElt.DataPayload.SetSplinePayloadProperties( InStart, InStartDir, InEnd, InEndDir, InThickness, InTint );
	DrawElt.Layer = InLayer;
	DrawElt.DrawEffects = InDrawEffects;
	DrawElt.Scale = PaintGeometry.DrawScale;
}


void FSlateDrawElement::MakeDrawSpaceSpline( FSlateWindowElementList& ElementList, uint32 InLayer, const FVector2D& InStart, const FVector2D& InStartDir, const FVector2D& InEnd, const FVector2D& InEndDir, const FSlateRect InClippingRect, float InThickness, ESlateDrawEffect::Type InDrawEffects, const FColor& InTint )
{
	MakeSpline( ElementList, InLayer, FPaintGeometry(), InStart, InStartDir, InEnd, InEndDir, InClippingRect, InThickness, InDrawEffects, InTint );
}


void FSlateDrawElement::MakeLines( FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& PaintGeometry, const TArray<FVector2D>& Points, const FSlateRect InClippingRect, ESlateDrawEffect::Type InDrawEffects, const FLinearColor& InTint, bool bAntialias )
{
	PaintGeometry.CommitTransformsIfUsingLegacyConstructor();
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.ElementType = ET_Line;
	DrawElt.RenderTransform = PaintGeometry.GetAccumulatedRenderTransform();
	DrawElt.Position = PaintGeometry.DrawPosition;
	DrawElt.LocalSize = PaintGeometry.GetLocalSize();
	DrawElt.ClippingRect = InClippingRect;
	DrawElt.DataPayload.SetLinesPayloadProperties( Points, InTint, bAntialias, ESlateLineJoinType::Sharp );
	DrawElt.Layer = InLayer;
	DrawElt.DrawEffects = InDrawEffects;
	DrawElt.Scale = PaintGeometry.DrawScale;
}


void FSlateDrawElement::MakeViewport( FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& PaintGeometry, TSharedPtr<const ISlateViewport> Viewport, const FSlateRect& InClippingRect, bool bGammaCorrect, bool bAllowBlending, ESlateDrawEffect::Type InDrawEffects, const FLinearColor& InTint )
{
	PaintGeometry.CommitTransformsIfUsingLegacyConstructor();
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.ElementType = ET_Viewport;
	DrawElt.RenderTransform = PaintGeometry.GetAccumulatedRenderTransform();
	DrawElt.Position = PaintGeometry.DrawPosition;
	DrawElt.LocalSize = PaintGeometry.GetLocalSize();
	DrawElt.ClippingRect = InClippingRect;
	DrawElt.DataPayload.SetViewportPayloadProperties( Viewport, InTint, bGammaCorrect, bAllowBlending );
	DrawElt.Layer = InLayer;
	DrawElt.DrawEffects = InDrawEffects;
	DrawElt.Scale = PaintGeometry.DrawScale;
}


void FSlateDrawElement::MakeCustom( FSlateWindowElementList& ElementList, uint32 InLayer, TSharedPtr<ICustomSlateElement, ESPMode::ThreadSafe> CustomDrawer )
{
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.RenderTransform = FSlateRenderTransform();
	DrawElt.ElementType = ET_Custom;
	DrawElt.Layer = InLayer;
	DrawElt.DataPayload.SetCustomDrawerPayloadProperties( CustomDrawer );
	DrawElt.ClippingRect = FSlateRect(1,1,1,1);
}

FVector2D FSlateDrawElement::GetRotationPoint(const FPaintGeometry& PaintGeometry, const TOptional<FVector2D>& UserRotationPoint, ERotationSpace RotationSpace)
{
	FVector2D RotationPoint(0, 0);

	const FVector2D& LocalSize = PaintGeometry.GetLocalSize();

	switch (RotationSpace)
	{
	case RelativeToElement:
	{
		// If the user did not specify a rotation point, we rotate about the center of the element
		RotationPoint = UserRotationPoint.Get(LocalSize * 0.5f);
	}
		break;
	case RelativeToWorld:
	{
		// its in world space, must convert the point to local space.
		RotationPoint = TransformPoint(Inverse(PaintGeometry.GetAccumulatedRenderTransform()), UserRotationPoint.Get(FVector2D::ZeroVector));
	}
		break;
	default:
		check(0);
		break;
	}

	return RotationPoint;
}

FSlateWindowElementList::FDeferredPaint::FDeferredPaint( const TSharedRef<SWidget>& InWidgetToPaint, const FPaintArgs& InArgs, const FGeometry InAllottedGeometry, const FSlateRect InMyClippingRect, const FWidgetStyle& InWidgetStyle, bool InParentEnabled )
: WidgetToPaintPtr( InWidgetToPaint )
, Args( InArgs )
, AllottedGeometry( InAllottedGeometry )
, MyClippingRect( InMyClippingRect )
, WidgetStyle( InWidgetStyle )
, bParentEnabled( InParentEnabled )
{
}


int32 FSlateWindowElementList::FDeferredPaint::ExecutePaint( int32 LayerId, FSlateWindowElementList& OutDrawElements ) const
{
	TSharedPtr<SWidget> WidgetToPaint = WidgetToPaintPtr.Pin();
	if ( WidgetToPaint.IsValid() )
	{
		return WidgetToPaint->Paint( Args, AllottedGeometry, OutDrawElements.GetWindow()->GetClippingRectangleInWindow(), OutDrawElements, LayerId, WidgetStyle, bParentEnabled );
	}

	return LayerId;
}


void FSlateWindowElementList::QueueDeferredPainting( const FDeferredPaint& InDeferredPaint )
{
	DeferredPaintList.Add( MakeShareable( new FDeferredPaint( InDeferredPaint ) ) );
}


int32 FSlateWindowElementList::PaintDeferred( int32 LayerId )
{
	for ( const TSharedRef<FDeferredPaint>& PaintArgs : DeferredPaintList )
	{
		LayerId = PaintArgs->ExecutePaint( LayerId, *this );
	}

	return LayerId;
}
