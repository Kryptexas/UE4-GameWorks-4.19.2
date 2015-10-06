// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#include "SMeshWidget.h"
#include "SlateMaterialBrush.h"
#include "Runtime/SlateRHIRenderer/Public/Interfaces/ISlateRHIRendererModule.h"
#include "SlateMeshData.h"



/** Populate OutSlateVerts and OutIndexes with data from this static mesh such that Slate can render it. */
static void SlateMeshToSlateRenderData(const USlateMeshData& DataSource, TArray<FSlateVertex>& OutSlateVerts, TArray<SlateIndex>& OutIndexes)
{
	// Populate Index data
	{
		// Note that we do a slow copy because on some platforms the SlateIndex is
		// a 16-bit value, so we cannot do a memcopy.
		const TArray<uint32>& IndexDataSource = DataSource.GetIndexData();
		const int32 NumIndexes = IndexDataSource.Num();
		OutIndexes.Empty();
		OutIndexes.Reserve(NumIndexes);
		for (int32 i = 0; i < NumIndexes; ++i)
		{
			OutIndexes.Add(IndexDataSource[i]);
		}
	}

	// Populate Vertex Data
	{
		const TArray<FSlateMeshVertex> VertexDataSource = DataSource.GetVertexData();
		const uint32 NumVerts = VertexDataSource.Num();
		OutSlateVerts.Empty();
		OutSlateVerts.Reserve(NumVerts);

		for (uint32 i = 0; i < NumVerts; ++i)
		{
			const FSlateMeshVertex& SourceVertex = VertexDataSource[i];
			FSlateVertex& NewVert = OutSlateVerts[OutSlateVerts.AddUninitialized()];

			// Copy Position
			{
				NewVert.Position[0] = SourceVertex.Position.X;
				NewVert.Position[1] = SourceVertex.Position.Y;
			}

			// Copy Color
			{
				NewVert.Color = SourceVertex.Color;
			}


			// Copy all the UVs that we have, and as many as we can fit.
			{
				NewVert.TexCoords[0] = SourceVertex.UV0.X;
				NewVert.TexCoords[1] = SourceVertex.UV0.Y;

				NewVert.TexCoords[2] = SourceVertex.UV1.X;
				NewVert.TexCoords[3] = SourceVertex.UV1.Y;

				NewVert.MaterialTexCoords[0] = SourceVertex.UV2.X;
				NewVert.MaterialTexCoords[1] = SourceVertex.UV2.Y;

				NewVert.ClipRect.TopLeft = SourceVertex.UV3;

				NewVert.ClipRect.ExtentX = SourceVertex.UV4;

				NewVert.ClipRect.ExtentY = SourceVertex.UV5;
			}
		}
	}
}



void SMeshWidget::Construct(const FArguments& Args)
{
	if (Args._MeshData != nullptr)
	{
		SetMesh(*Args._MeshData);
	}
}

void SMeshWidget::AddReferencedObjects( FReferenceCollector& Collector )
{
	if( Brush.IsValid() && Brush->GetResourceObject() )
	{
		UObject* ResourceObject = Brush->GetResourceObject();
		Collector.AddReferencedObject( ResourceObject );
	}
}

static const FVector2D DontCare(FVector2D(64, 64));
void SMeshWidget::SetMesh(const USlateMeshData& InMeshData)
{	
	Brush = MakeShareable(new FSlateMaterialBrush(*InMeshData.GetMaterial(), DontCare));
	SlateMeshToSlateRenderData(InMeshData, VertexData, IndexData);
}

static const FName SlateRHIModuleName("SlateRHIRenderer");
TSharedPtr<FSlateInstanceBufferUpdate> SMeshWidget::BeginPerInstanceBufferUpdate(int32 InitialSize)
{
	if (!PerInstanceBuffer.IsValid())
	{
		PerInstanceBuffer = FModuleManager::Get().GetModuleChecked<ISlateRHIRendererModule>(SlateRHIModuleName).CreateInstanceBuffer(InitialSize);
	}

	return PerInstanceBuffer->BeginUpdate();
}


int32 SMeshWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (Brush.IsValid() && VertexData.Num() > 0 && IndexData.Num() > 0)
	{
		FSlateDrawElement::MakeCustomVerts(OutDrawElements, LayerId, Brush.Get(), VertexData, IndexData, PerInstanceBuffer.Get());
	}

	return LayerId;
}


FVector2D SMeshWidget::ComputeDesiredSize(float) const
{
	return FVector2D(256,256);
}


