// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SlateCorePrivatePCH.h"
#include "ReflectionMetadata.h"

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

/**  */
TAutoConsoleVariable<int32> DrawVolatileWidgets(
	TEXT("Slate.DrawVolatileWidgets"),
	true,
	TEXT("Whether to draw volatile widgets"));

#endif

DECLARE_CYCLE_STAT(TEXT("FSlateDrawElement::Make Time"), STAT_SlateDrawElementMakeTime, STATGROUP_SlateVerbose);

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

void FSlateDrawElement::MakeCachedBuffer(FSlateWindowElementList& ElementList, uint32 InLayer, TSharedPtr<FSlateRenderDataHandle, ESPMode::ThreadSafe>& CachedRenderDataHandle)
{
	SCOPE_CYCLE_COUNTER(STAT_SlateDrawElementMakeTime)
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.Init(InLayer, FPaintGeometry(), FSlateRect(1, 1, 1, 1), ESlateDrawEffect::None);
	DrawElt.RenderTransform = FSlateRenderTransform();
	DrawElt.ElementType = ET_CachedBuffer;
	DrawElt.DataPayload.SetCachedBufferPayloadProperties(CachedRenderDataHandle.Get());
}

void FSlateDrawElement::MakeLayer(FSlateWindowElementList& ElementList, uint32 InLayer, TSharedPtr<FSlateDrawLayerHandle, ESPMode::ThreadSafe>& DrawLayerHandle)
{
	SCOPE_CYCLE_COUNTER(STAT_SlateDrawElementMakeTime)
	FSlateDrawElement& DrawElt = ElementList.AddUninitialized();
	DrawElt.Init(InLayer, FPaintGeometry(), FSlateRect(1, 1, 1, 1), ESlateDrawEffect::None);
	DrawElt.RenderTransform = FSlateRenderTransform();
	DrawElt.ElementType = ET_Layer;
	DrawElt.DataPayload.SetLayerPayloadProperties(DrawLayerHandle.Get());
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

	RenderDataHandle.Reset();
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
		// Ignore foreign batches that are inserted into our render set.
		if ( RenderDataHandle != Batch.CachedRenderHandle )
		{
			continue;
		}

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

				Vertices.Reset();
				Indices.Reset();
			}
		}
	}
}

void FSlateBatchData::UpdateRenderBatches(FVector2D PositionOffset)
{
	checkSlow(IsInRenderingThread());

	for ( const FSlateRenderBatch& Batch : RenderBatches )
	{
		if ( Batch.VertexArrayIndex != INDEX_NONE && Batch.IndexArrayIndex != INDEX_NONE )
		{
			TArray<FSlateVertex>& Vertices = BatchVertexArrays[Batch.VertexArrayIndex];

			int32 Count = Vertices.Num();
			for ( int32 VertexIndex = 0; VertexIndex < Count; VertexIndex++ )
			{
				FSlateVertex& Vertex = Vertices[VertexIndex];
				Vertex.Position[0] += PositionOffset.X;
				Vertex.Position[1] += PositionOffset.Y;
				Vertex.ClipRect.TopLeft += PositionOffset;
			}
		}
	}
}

void FSlateBatchData::CreateRenderBatches(FElementBatchMap& LayerToElementBatches)
{
	checkSlow( IsInRenderingThread() );

	uint32 VertexOffset = 0;
	uint32 IndexOffset = 0;

	Merge(LayerToElementBatches, VertexOffset, IndexOffset);

	// 
	if ( RenderDataHandle.IsValid() )
	{
		RenderDataHandle->SetRenderBatches(&RenderBatches);
	}
}

void FSlateBatchData::Merge(FElementBatchMap& InLayerToElementBatches, uint32& VertexOffset, uint32& IndexOffset)
{
	InLayerToElementBatches.KeySort(TLess<uint32>());

	const bool bExpandLayersAndCachedHandles = RenderDataHandle.IsValid() == false;

	// For each element batch add its vertices and indices to the bulk lists.
	for ( FElementBatchMap::TIterator It(InLayerToElementBatches); It; ++It )
	{
		uint32 Layer = It.Key();
		FElementBatchArray& ElementBatches = It.Value();

		if ( ElementBatches.Num() > 0 )
		{
			++NumLayers;
			for ( FElementBatchArray::TIterator BatchIt(ElementBatches); BatchIt; ++BatchIt )
			{
				FSlateElementBatch& ElementBatch = *BatchIt;

				if ( ElementBatch.GetCustomDrawer().IsValid() )
				{
					AddRenderBatch(Layer, ElementBatch, 0, 0, 0, 0);
				}
				else
				{
					if ( bExpandLayersAndCachedHandles )
					{
						if ( FSlateRenderDataHandle* RenderHandle = ElementBatch.GetCachedRenderHandle().Get() )
						{
							if ( TArray<FSlateRenderBatch>* ForeignBatches = RenderHandle->GetRenderBatches() )
							{
								TArray<FSlateRenderBatch>& ForeignBatchesRef = *ForeignBatches;
								for ( int32 i = 0; i < ForeignBatches->Num(); i++ )
								{
									TSharedPtr<FSlateDrawLayerHandle, ESPMode::ThreadSafe> LayerHandle = ForeignBatchesRef[i].LayerHandle.Pin();
									if ( LayerHandle.IsValid() )
									{
										// If a record was added for a layer, but nothing was ever drawn for it, the batch map will be null.
										if ( LayerHandle->BatchMap )
										{
											Merge(*LayerHandle->BatchMap, VertexOffset, IndexOffset);
											LayerHandle->BatchMap = nullptr;
										}
									}
									else
									{
										RenderBatches.Add(ForeignBatchesRef[i]);
									}
									//RenderBatches.Append(*RenderHandle->GetRenderBatches());
								}
							}

							continue;
						}
					}
					else
					{
						// Insert if we're not expanding
						if ( FSlateDrawLayerHandle* LayerHandle = ElementBatch.GetLayerHandle().Get() )
						{
							AddRenderBatch(Layer, ElementBatch, 0, 0, 0, 0);
							continue;
						}
					}
					
					if ( ElementBatch.VertexArrayIndex != INDEX_NONE && ElementBatch.IndexArrayIndex != INDEX_NONE )
					{
						TArray<FSlateVertex>& BatchVertices = GetBatchVertexList(ElementBatch);
						TArray<SlateIndex>& BatchIndices = GetBatchIndexList(ElementBatch);

						// We should have at least some vertices and indices in the batch or none at all
						check(BatchVertices.Num() > 0 && BatchIndices.Num() > 0 || BatchVertices.Num() == 0 && BatchIndices.Num() == 0);

						if ( BatchVertices.Num() > 0 && BatchIndices.Num() > 0 )
						{
							const int32 NumVertices = BatchVertices.Num();
							const int32 NumIndices = BatchIndices.Num();

							AddRenderBatch(Layer, ElementBatch, NumVertices, NumIndices, VertexOffset, IndexOffset);

							VertexOffset += BatchVertices.Num();
							IndexOffset += BatchIndices.Num();
						}
					}
				}
			}
		}

		ElementBatches.Reset();
	}
}

void FSlateBatchData::SortRenderBatches()
{
	//RenderBatches.StableSort();
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
	DeferredPaintList.Add(MakeShareable(new FDeferredPaint(InDeferredPaint)));
}

int32 FSlateWindowElementList::PaintDeferred(int32 LayerId)
{
	for ( int32 i = 0; i < DeferredPaintList.Num(); ++i )
	{
		LayerId = DeferredPaintList[i]->ExecutePaint(LayerId, *this);
	}

	return LayerId;
}


FSlateWindowElementList::FVolatilePaint::FVolatilePaint(const TSharedRef<const SWidget>& InWidgetToPaint, const FPaintArgs& InArgs, const FGeometry InAllottedGeometry, const FSlateRect InMyClippingRect, int32 InLayerId, const FWidgetStyle& InWidgetStyle, bool InParentEnabled)
	: WidgetToPaintPtr(InWidgetToPaint)
	, Args(InArgs.EnableCaching(InArgs.GetLayoutCache(), InArgs.GetParentCacheNode(), false, true))
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
		//FPlatformMisc::BeginNamedEvent(FColor::Orange, "Overhead");
		//FPlatformMisc::BeginNamedEvent(FColor::Orange, *FReflectionMetaData::GetWidgetDebugInfo(WidgetToPaint));

		// Have to run a slate pre-pass for all volatile elements, some widgets cache information like 
		// the STextBlock.  This may be all kinds of terrible an idea to do during paint.
		SWidget* MutableWidget = const_cast<SWidget*>( WidgetToPaint.Get() );
		MutableWidget->SlatePrepass(AllottedGeometry.Scale);

		const int32 NewLayer = WidgetToPaint->Paint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, WidgetStyle, bParentEnabled);
		
		//FPlatformMisc::EndNamedEvent();
		//FPlatformMisc::EndNamedEvent();

		return NewLayer;
	}

	return LayerId;
}

void FSlateWindowElementList::QueueVolatilePainting(const FVolatilePaint& InVolatilePaint)
{
	TSharedPtr< FSlateDrawLayerHandle, ESPMode::ThreadSafe > LayerHandle = MakeShareable(new FSlateDrawLayerHandle());

	FSlateDrawElement::MakeLayer(*this, InVolatilePaint.GetLayerId(), LayerHandle);

	const int32 NewEntryIndex = VolatilePaintList.Add(MakeShareable(new FVolatilePaint(InVolatilePaint)));
	VolatilePaintList[NewEntryIndex]->LayerHandle = LayerHandle;
}

int32 FSlateWindowElementList::PaintVolatile(FSlateWindowElementList& OutElementList)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	const bool bDrawVolatile = DrawVolatileWidgets.GetValueOnGameThread() == 1;
#else
	const bool bDrawVolatile = true;
#endif

	const static FName InvalidationPanelName(TEXT("SInvalidationPanelName"));

	int32 MaxLayerId = 0;

	for ( int32 VolatileIndex = 0; VolatileIndex < VolatilePaintList.Num(); ++VolatileIndex )
	{
		const TSharedPtr<FVolatilePaint>& Args = VolatilePaintList[VolatileIndex];

		// If we're not drawing volatile elements, just draw invalidation panels.  They're
		// the exception, as we're likely trying to determine the volatile widget overhead.
		if ( bDrawVolatile == false )
		{
			if ( const SWidget* Widget = Args->GetWidget() )
			{
				if ( Widget->GetType() != InvalidationPanelName )
				{
					continue;
				}
			}
		}

		OutElementList.BeginLogicalLayer(Args->LayerHandle);
		MaxLayerId = FMath::Max(MaxLayerId, Args->ExecutePaint(OutElementList));
		OutElementList.EndLogicalLayer();
	}

	return MaxLayerId;
}

void FSlateWindowElementList::BeginLogicalLayer(const TSharedPtr<FSlateDrawLayerHandle, ESPMode::ThreadSafe>& LayerHandle)
{
	// Don't attempt to begin logical layers inside a cached view of the data.
	checkSlow(!IsCachedRenderDataInUse());

	//FPlatformMisc::BeginNamedEvent(FColor::Orange, "FindLayer");
	TSharedPtr<FSlateDrawLayer> Layer = DrawLayers.FindRef(LayerHandle);
	//FPlatformMisc::EndNamedEvent();

	if ( !Layer.IsValid() )
	{
		if ( DrawLayerPool.Num() > 0 )
		{
			Layer = DrawLayerPool.Pop(false);
		}
		else
		{
			Layer = MakeShareable(new FSlateDrawLayer());
		}

		//FPlatformMisc::BeginNamedEvent(FColor::Orange, "AddLayer");
		DrawLayers.Add(LayerHandle, Layer);
		//FPlatformMisc::EndNamedEvent();
	}

	//FPlatformMisc::BeginNamedEvent(FColor::Orange, "PushLayer");
	DrawStack.Push(Layer.Get());
	//FPlatformMisc::EndNamedEvent();
}

void FSlateWindowElementList::EndLogicalLayer()
{
	DrawStack.Pop();
}

FSlateRenderDataHandle::FSlateRenderDataHandle(FSlateRenderer* InRenderer)
	: Renderer(InRenderer)
	, RenderBatches(nullptr)
{
}

FSlateRenderDataHandle::~FSlateRenderDataHandle()
{
	if ( Renderer )
	{
		Renderer->ReleaseCachedRenderData(this);
	}
}

void FSlateRenderDataHandle::Disconnect()
{
	Renderer = nullptr;
	RenderBatches = nullptr;
}

TSharedRef<FSlateRenderDataHandle, ESPMode::ThreadSafe> FSlateWindowElementList::CacheRenderData()
{
	// Don't attempt to use this slate window element list if the cache is still being used.
	checkSlow(!IsCachedRenderDataInUse());

	TSharedPtr<FSlateRenderer> Renderer = FSlateApplicationBase::Get().GetRenderer();

	TSharedRef<FSlateRenderDataHandle, ESPMode::ThreadSafe> CachedRenderDataHandleRef = Renderer->CacheElementRenderData(*this);
	CachedRenderDataHandle = CachedRenderDataHandleRef;

	return CachedRenderDataHandleRef;
}

void FSlateWindowElementList::UpdateCacheRenderData(FVector2D PositionOffset)
{
	// This should only be done if we're using cached render data
	checkSlow(IsCachedRenderDataInUse());

	TSharedPtr<FSlateRenderer> Renderer = FSlateApplicationBase::Get().GetRenderer();
	Renderer->UpdateElementRenderData(*this, PositionOffset);
}

void FSlateWindowElementList::PreDraw_RenderThread()
{
	check(IsInRenderingThread());

	for ( auto& Entry : DrawLayers )
	{
		checkSlow(Entry.Key->BatchMap == nullptr);
		Entry.Key->BatchMap = &Entry.Value->GetElementBatchMap();
	}
}

void FSlateWindowElementList::PostDraw_RenderThread()
{
	check(IsInRenderingThread());

	for ( auto& Entry : DrawLayers )
	{
		Entry.Key->BatchMap = nullptr;
	}
}

void FSlateWindowElementList::ResetBuffers()
{
	// Don't attempt to use this slate window element list if the cache is still being used.
	checkSlow(!IsCachedRenderDataInUse());

	DeferredPaintList.Reset();
	VolatilePaintList.Reset();
	BatchData.Reset();

#if SLATE_POOL_DRAW_ELEMENTS
	DrawElementFreePool.Append(RootDrawLayer.DrawElements);
#endif

	// Reset the draw elements on the root draw layer
	RootDrawLayer.DrawElements.Reset();

	// Return child draw layers to the pool, and reset their draw elements.
	for ( auto& Entry : DrawLayers )
	{
#if SLATE_POOL_DRAW_ELEMENTS
		TArray<FSlateDrawElement*>& DrawElements = Entry.Value->DrawElements;
		DrawElementFreePool.Append(DrawElements);
#else
		TArray<FSlateDrawElement>& DrawElements = Entry.Value->DrawElements;
#endif

		DrawElements.Reset();
		DrawLayerPool.Add(Entry.Value);
	}

	DrawLayers.Reset();

	DrawStack.Reset();
	DrawStack.Push(&RootDrawLayer);
}
