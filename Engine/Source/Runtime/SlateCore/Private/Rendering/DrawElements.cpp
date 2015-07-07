// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SlateCorePrivatePCH.h"

DECLARE_CYCLE_STAT(TEXT("FSlateDrawElement::Make Time"), STAT_SlateDrawElementMakeTime, STATGROUP_Slate);

void FSlateDrawElement::Init(uint32 InLayer, const FPaintGeometry& PaintGeometry, const FSlateRect& InClippingRect, ESlateDrawEffect::Type InDrawEffects)
{
	RenderTransform = PaintGeometry.GetAccumulatedRenderTransform();
	Position = PaintGeometry.DrawPosition;
	LocalSize = PaintGeometry.GetLocalSize();
	ClippingRect = InClippingRect;
	Layer = InLayer;
	Scale = PaintGeometry.DrawScale;
	DrawEffects = InDrawEffects;
	extern SLATECORE_API TOptional<FShortRect> GSlateScissorRect;
	ScissorRect = GSlateScissorRect;
}

void FSlateDrawElement::MakeDebugQuad( FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& PaintGeometry, const FSlateRect& InClippingRect)
{
	SCOPE_CYCLE_COUNTER( STAT_SlateDrawElementMakeTime )
	PaintGeometry.CommitTransformsIfUsingLegacyConstructor();
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.Init(InLayer, PaintGeometry, InClippingRect, ESlateDrawEffect::None);
	DrawElt.ElementType = ET_DebugQuad;
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
	SCOPE_CYCLE_COUNTER( STAT_SlateDrawElementMakeTime )
	PaintGeometry.CommitTransformsIfUsingLegacyConstructor();
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.Init(InLayer, PaintGeometry, InClippingRect, InDrawEffects);
	DrawElt.ElementType = (InBrush->DrawAs == ESlateBrushDrawType::Border) ? ET_Border : ET_Box;
	DrawElt.DataPayload.SetBoxPayloadProperties( InBrush, InTint );
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
	SCOPE_CYCLE_COUNTER( STAT_SlateDrawElementMakeTime )
	PaintGeometry.CommitTransformsIfUsingLegacyConstructor();
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.Init(InLayer, PaintGeometry, InClippingRect, InDrawEffects);
	DrawElt.ElementType = (InBrush->DrawAs == ESlateBrushDrawType::Border) ? ET_Border : ET_Box;

	FVector2D RotationPoint = GetRotationPoint( PaintGeometry, InRotationPoint, RotationSpace );
	DrawElt.DataPayload.SetRotatedBoxPayloadProperties( InBrush, Angle, RotationPoint, InTint );
}


void FSlateDrawElement::MakeText( FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& PaintGeometry, const FString& InText, const int32 StartIndex, const int32 EndIndex, const FSlateFontInfo& InFontInfo, const FSlateRect& InClippingRect,ESlateDrawEffect::Type InDrawEffects, const FLinearColor& InTint )
{
	SCOPE_CYCLE_COUNTER( STAT_SlateDrawElementMakeTime )
	PaintGeometry.CommitTransformsIfUsingLegacyConstructor();
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.Init(InLayer, PaintGeometry, InClippingRect, InDrawEffects);
	DrawElt.ElementType = ET_Text;
	DrawElt.DataPayload.SetTextPayloadProperties( FString( EndIndex - StartIndex, *InText + StartIndex ), InFontInfo, InTint );
}


void FSlateDrawElement::MakeText( FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& PaintGeometry, const FString& InText, const FSlateFontInfo& InFontInfo, const FSlateRect& InClippingRect,ESlateDrawEffect::Type InDrawEffects, const FLinearColor& InTint )
{
	SCOPE_CYCLE_COUNTER( STAT_SlateDrawElementMakeTime )
	PaintGeometry.CommitTransformsIfUsingLegacyConstructor();
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.Init(InLayer, PaintGeometry, InClippingRect, InDrawEffects);
	DrawElt.ElementType = ET_Text;
	DrawElt.DataPayload.SetTextPayloadProperties( InText, InFontInfo, InTint );
}


void FSlateDrawElement::MakeText( FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& PaintGeometry, const FText& InText, const FSlateFontInfo& InFontInfo, const FSlateRect& InClippingRect,ESlateDrawEffect::Type InDrawEffects, const FLinearColor& InTint )
{
	SCOPE_CYCLE_COUNTER( STAT_SlateDrawElementMakeTime )
	PaintGeometry.CommitTransformsIfUsingLegacyConstructor();
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.Init(InLayer, PaintGeometry, InClippingRect, InDrawEffects);
	DrawElt.ElementType = ET_Text;
	DrawElt.DataPayload.SetTextPayloadProperties( InText.ToString(), InFontInfo, InTint );
}

void FSlateDrawElement::MakeGradient( FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& PaintGeometry, TArray<FSlateGradientStop> InGradientStops, EOrientation InGradientType, const FSlateRect& InClippingRect, ESlateDrawEffect::Type InDrawEffects, bool bGammaCorrect )
{
	SCOPE_CYCLE_COUNTER( STAT_SlateDrawElementMakeTime )
	PaintGeometry.CommitTransformsIfUsingLegacyConstructor();
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.Init(InLayer, PaintGeometry, InClippingRect, InDrawEffects);
	DrawElt.ElementType = ET_Gradient;
	DrawElt.DataPayload.SetGradientPayloadProperties( InGradientStops, InGradientType, bGammaCorrect );
}


void FSlateDrawElement::MakeSpline( FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& PaintGeometry, const FVector2D& InStart, const FVector2D& InStartDir, const FVector2D& InEnd, const FVector2D& InEndDir, const FSlateRect InClippingRect, float InThickness, ESlateDrawEffect::Type InDrawEffects, const FLinearColor& InTint )
{
	SCOPE_CYCLE_COUNTER( STAT_SlateDrawElementMakeTime )
	PaintGeometry.CommitTransformsIfUsingLegacyConstructor();
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.Init(InLayer, PaintGeometry, InClippingRect, InDrawEffects);
	DrawElt.ElementType = ET_Spline;
	DrawElt.DataPayload.SetSplinePayloadProperties( InStart, InStartDir, InEnd, InEndDir, InThickness, InTint );
}


void FSlateDrawElement::MakeDrawSpaceSpline( FSlateWindowElementList& ElementList, uint32 InLayer, const FVector2D& InStart, const FVector2D& InStartDir, const FVector2D& InEnd, const FVector2D& InEndDir, const FSlateRect InClippingRect, float InThickness, ESlateDrawEffect::Type InDrawEffects, const FLinearColor& InTint )
{
	MakeSpline( ElementList, InLayer, FPaintGeometry(), InStart, InStartDir, InEnd, InEndDir, InClippingRect, InThickness, InDrawEffects, InTint );
}


void FSlateDrawElement::MakeLines( FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& PaintGeometry, const TArray<FVector2D>& Points, const FSlateRect InClippingRect, ESlateDrawEffect::Type InDrawEffects, const FLinearColor& InTint, bool bAntialias )
{
	SCOPE_CYCLE_COUNTER( STAT_SlateDrawElementMakeTime )
	PaintGeometry.CommitTransformsIfUsingLegacyConstructor();
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.Init(InLayer, PaintGeometry, InClippingRect, InDrawEffects);
	DrawElt.ElementType = ET_Line;
	DrawElt.DataPayload.SetLinesPayloadProperties( Points, InTint, bAntialias, ESlateLineJoinType::Sharp );
}


void FSlateDrawElement::MakeViewport( FSlateWindowElementList& ElementList, uint32 InLayer, const FPaintGeometry& PaintGeometry, TSharedPtr<const ISlateViewport> Viewport, const FSlateRect& InClippingRect, bool bGammaCorrect, bool bAllowBlending, ESlateDrawEffect::Type InDrawEffects, const FLinearColor& InTint )
{
	SCOPE_CYCLE_COUNTER( STAT_SlateDrawElementMakeTime )
	PaintGeometry.CommitTransformsIfUsingLegacyConstructor();
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.Init(InLayer, PaintGeometry, InClippingRect, InDrawEffects);
	DrawElt.ElementType = ET_Viewport;
	DrawElt.DataPayload.SetViewportPayloadProperties( Viewport, InTint, bGammaCorrect, bAllowBlending );
}


void FSlateDrawElement::MakeCustom( FSlateWindowElementList& ElementList, uint32 InLayer, TSharedPtr<ICustomSlateElement, ESPMode::ThreadSafe> CustomDrawer )
{
	SCOPE_CYCLE_COUNTER( STAT_SlateDrawElementMakeTime )
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.Init(InLayer, FPaintGeometry(), FSlateRect(1,1,1,1), ESlateDrawEffect::None);
	DrawElt.RenderTransform = FSlateRenderTransform();
	DrawElt.ElementType = ET_Custom;
	DrawElt.DataPayload.SetCustomDrawerPayloadProperties( CustomDrawer );
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

void FSlateBatchData::Reset()
{
	RenderBatches.Reset();
	
	// note: LayerToElementBatches is not reset here as the same layers are 
	// more than likely reused and we can save memory allocations by not resetting the map every frame

	NumBatchedVertices = 0;
	NumBatchedIndices = 0;
	NumLayers = 0;
}

void FSlateBatchData::AssignVertexArrayToBatch( FSlateElementBatch& Batch )
{
	// Get a free vertex array
	if (VertexArrayFreeList.Num() > 0)
	{
		Batch.VertexArrayIndex = VertexArrayFreeList.Pop(/*bAllowShrinking=*/ false);
		BatchVertexArrays[Batch.VertexArrayIndex].Reserve(200);
	}
	else
	{
		// There are no free vertex arrays so we must add one		
		uint32 NewIndex = BatchVertexArrays.Add(TArray<FSlateVertex>());
		BatchVertexArrays[NewIndex].Reserve(200);

		Batch.VertexArrayIndex = NewIndex;
	}
}

void FSlateBatchData::AssignIndexArrayToBatch( FSlateElementBatch& Batch )
{
	// Get a free index array
	if (IndexArrayFreeList.Num() > 0)
	{
		Batch.IndexArrayIndex = IndexArrayFreeList.Pop(/*bAllowShrinking=*/ false);
		BatchIndexArrays[Batch.IndexArrayIndex].Reserve(200);
	}
	else
	{
		// There are no free index arrays so we must add one
		uint32 NewIndex = BatchIndexArrays.Add(TArray<SlateIndex>());
		BatchIndexArrays[NewIndex].Reserve(500);

		Batch.IndexArrayIndex = NewIndex;
	}

}

void FSlateBatchData::FillVertexAndIndexBuffer( uint8* VertexBuffer, uint8* IndexBuffer )
{
	int32 IndexOffset = 0;
	int32 VertexOffset = 0;
	for( const FSlateRenderBatch& Batch : RenderBatches )
	{
		if( Batch.VertexArrayIndex != INDEX_NONE && Batch.IndexArrayIndex != INDEX_NONE )
		{
			TArray<FSlateVertex>& Vertices = BatchVertexArrays[Batch.VertexArrayIndex];
			TArray<SlateIndex>& Indices = BatchIndexArrays[Batch.IndexArrayIndex];

			if(Vertices.Num() && Indices.Num())
			{
				uint32 RequiredVertexSize = Vertices.Num() * Vertices.GetTypeSize();
				uint32 RequiredIndexSize = Indices.Num() * Indices.GetTypeSize();

				FMemory::Memcpy(VertexBuffer+VertexOffset, Vertices.GetData(), RequiredVertexSize);
				FMemory::Memcpy(IndexBuffer+IndexOffset, Indices.GetData(), RequiredIndexSize);

				VertexArrayFreeList.Add(Batch.VertexArrayIndex);
				IndexArrayFreeList.Add(Batch.IndexArrayIndex);

				IndexOffset += (Indices.Num()*sizeof(SlateIndex));
				VertexOffset += (Vertices.Num()*sizeof(FSlateVertex));

				Vertices.Empty(Vertices.Num());
				Indices.Empty(Indices.Num());
			}
		}
	}
}

void FSlateBatchData::CreateRenderBatches()
{
	checkSlow( IsInRenderingThread() );

	LayerToElementBatches.KeySort( TLess<uint32>() );

	uint32 VertexOffset = 0;
	uint32 IndexOffset = 0;
	// For each element batch add its vertices and indices to the bulk lists.
	for (FElementBatchMap::TIterator It(LayerToElementBatches); It; ++It)
	{
		FElementBatchArray& ElementBatches = It.Value();

		if( ElementBatches.Num() > 0 )
		{
			++NumLayers;
			for (FElementBatchArray::TIterator BatchIt(ElementBatches); BatchIt; ++BatchIt)
			{
				FSlateElementBatch& ElementBatch = *BatchIt;

				bool bIsMaterial = ElementBatch.GetShaderResource() && ElementBatch.GetShaderResource()->GetType() == ESlateShaderResource::Material;

				if (!ElementBatch.GetCustomDrawer().IsValid())
				{
					TArray<FSlateVertex>& BatchVertices = GetBatchVertexList(ElementBatch);
					TArray<SlateIndex>& BatchIndices = GetBatchIndexList(ElementBatch);
	
					// We should have at least some vertices and indices in the batch or none at all
					check(BatchVertices.Num() > 0 && BatchIndices.Num() > 0 || BatchVertices.Num() == 0 && BatchIndices.Num() == 0);

					if (BatchVertices.Num() > 0 && BatchIndices.Num() > 0)
					{
						int32 NumVertices = BatchVertices.Num();
						int32 NumIndices = BatchIndices.Num();

						AddRenderBatch( ElementBatch, NumVertices, NumIndices, VertexOffset, IndexOffset );
				
						VertexOffset += BatchVertices.Num();
						IndexOffset += BatchIndices.Num();
					}
				}
				else
				{
					AddRenderBatch( ElementBatch, 0, 0, 0, 0 );
				}
			}
		}
		ElementBatches.Empty( ElementBatches.Num() );
	}

}

FSlateWindowElementList::FDeferredPaint::FDeferredPaint( const TSharedRef<const SWidget>& InWidgetToPaint, const FPaintArgs& InArgs, const FGeometry InAllottedGeometry, const FSlateRect InMyClippingRect, const FWidgetStyle& InWidgetStyle, bool InParentEnabled )
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
	TSharedPtr<const SWidget> WidgetToPaint = WidgetToPaintPtr.Pin();
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

int32 FSlateWindowElementList::PaintDeferred(int32 LayerId)
{
	for ( int32 i = 0; i < DeferredPaintList.Num(); ++i )
	{
		const TSharedRef< FDeferredPaint >& Args = DeferredPaintList[i];

		LayerId = Args->ExecutePaint(LayerId, *this);
	}

	return LayerId;
}


FSlateWindowElementList::FVolatilePaint::FVolatilePaint(const TSharedRef<const SWidget>& InWidgetToPaint, const FPaintArgs& InArgs, const FGeometry InAllottedGeometry, const FSlateRect InMyClippingRect, int32 InLayerId, const FWidgetStyle& InWidgetStyle, bool InParentEnabled)
	: WidgetToPaintPtr(InWidgetToPaint)
	, Args(InArgs.EnableCaching(InArgs.GetLayoutCache(), nullptr, false, true))
	, AllottedGeometry(InAllottedGeometry)
	, MyClippingRect(InMyClippingRect)
	, LayerId(InLayerId)
	, WidgetStyle(InWidgetStyle)
	, bParentEnabled(InParentEnabled)
{
}

int32 FSlateWindowElementList::FVolatilePaint::ExecutePaint(FSlateWindowElementList& OutDrawElements) const
{
	TSharedPtr<const SWidget> WidgetToPaint = WidgetToPaintPtr.Pin();
	if ( WidgetToPaint.IsValid() )
	{
		// Have to run a slate pre-pass for all volatile elements, some widgets cache information like 
		// the STextBlock.  This may be all kinds of terrible an idea to do during paint.
		SWidget* MutableWidget = const_cast<SWidget*>( WidgetToPaint.Get() );
		MutableWidget->SlatePrepass(AllottedGeometry.Scale);

		return WidgetToPaint->Paint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, WidgetStyle, bParentEnabled);
	}

	return LayerId;
}

void FSlateWindowElementList::QueueVolatilePainting(const FVolatilePaint& InDeferredPaint)
{
	VolatilePaintList.Add(MakeShareable(new FVolatilePaint(InDeferredPaint)));
}

int32 FSlateWindowElementList::PaintVolatile(FSlateWindowElementList& OutElementList)
{
	int32 MaxLayerId = 0;

	for ( int32 VolatileIndex = 0; VolatileIndex < VolatilePaintList.Num(); ++VolatileIndex )
	{
		const TSharedRef< FVolatilePaint >& Args = VolatilePaintList[VolatileIndex];

		MaxLayerId = FMath::Max(MaxLayerId, Args->ExecutePaint(OutElementList));
	}

	return MaxLayerId;
}

void FSlateWindowElementList::Reset()
{
	BatchData.Reset();
	DrawElements.Reset();
	DeferredPaintList.Reset();
	VolatilePaintList.Reset();
}
