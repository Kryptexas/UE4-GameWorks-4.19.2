// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#include "SMeshWidget.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "SlateMaterialBrush.h"
#include "Runtime/SlateRHIRenderer/Public/Interfaces/ISlateRHIRendererModule.h"


void SMeshWidget::Construct(const FArguments& Args)
{
	if (Args._StaticMeshSource != nullptr)
	{
		SetMesh(*Args._StaticMeshSource);
	}
}

static const FVector2D DontCare(FVector2D(64, 64));
void SMeshWidget::SetMesh(const UStaticMesh& InStaticMesh)
{	
	Brush = MakeShareable( new FSlateMaterialBrush(*InStaticMesh.GetMaterial(0), DontCare) );
	SMeshWidget::StaticMeshToSlateRenderData(InStaticMesh, VertexData, IndexData);
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


void SMeshWidget::StaticMeshToSlateRenderData(const UStaticMesh& DataSource, TArray<FSlateVertex>& OutSlateVerts, TArray<SlateIndex>& OutIndexes)
{
	const FStaticMeshLODResources& LOD = DataSource.RenderData->LODResources[0];
	const int32 NumSections = LOD.Sections.Num();
	if (NumSections > 1)
	{
		UE_LOG(LogUMG, Warning, TEXT("StaticMesh %s has %d sections. SMeshWidget expects a static mesh with 1 section."), *DataSource.GetName(), NumSections);
	}
	else
	{
		// Populate Index data
		{
			FIndexArrayView SourceIndexes = LOD.IndexBuffer.GetArrayView();
			const int32 NumIndexes = SourceIndexes.Num();
			OutIndexes.Empty();
			OutIndexes.Reserve(NumIndexes);
			for (int32 i = 0; i < NumIndexes; ++i)
			{
				OutIndexes.Add(SourceIndexes[i]);
			}
		}

		// Populate Vertex Data
		{
			const uint32 NumVerts = LOD.PositionVertexBuffer.GetNumVertices();
			OutSlateVerts.Empty();
			OutSlateVerts.Reserve(NumVerts);

			static const int32 MAX_SUPPORTED_UV_SETS = 6;
			const int32 TexCoordsPerVertex = LOD.GetNumTexCoords();
			if (TexCoordsPerVertex > MAX_SUPPORTED_UV_SETS)
			{
				UE_LOG(LogStaticMesh, Warning, TEXT("[%s] has %d UV sets; slate vertex data supports at most %d"), *DataSource.GetName(), TexCoordsPerVertex, MAX_SUPPORTED_UV_SETS);
			}

			for (uint32 i = 0; i < NumVerts; ++i)
			{
				FSlateVertex& NewVert = OutSlateVerts[OutSlateVerts.AddUninitialized()];

				// Copy Position
				{
					const FVector& Position = LOD.PositionVertexBuffer.VertexPosition(i);
					NewVert.Position[0] = Position.X;
					NewVert.Position[1] = Position.Y;
				}

				// Copy Color
				if (LOD.ColorVertexBuffer.GetNumVertices() > 0)
				{
					NewVert.Color = LOD.ColorVertexBuffer.VertexColor(i);
				}


				// Copy all the UVs that we have, and as many as we can fit.
				{
					const FVector2D& UV0 = (TexCoordsPerVertex > 0) ? LOD.VertexBuffer.GetVertexUV(i, 0) : FVector2D(1, 1);
					NewVert.TexCoords[0] = UV0.X;
					NewVert.TexCoords[1] = UV0.Y;

					const FVector2D& UV1 = (TexCoordsPerVertex > 1) ? LOD.VertexBuffer.GetVertexUV(i, 1) : FVector2D(1, 1);
					NewVert.TexCoords[2] = UV1.X;
					NewVert.TexCoords[3] = UV1.Y;

					const FVector2D& UV2 = (TexCoordsPerVertex > 2) ? LOD.VertexBuffer.GetVertexUV(i, 2) : FVector2D(1, 1);
					NewVert.MaterialTexCoords[0] = UV2.X;
					NewVert.MaterialTexCoords[1] = UV2.Y;

					const FVector2D& UV3 = (TexCoordsPerVertex > 3) ? LOD.VertexBuffer.GetVertexUV(i, 3) : FVector2D(1, 1);
					NewVert.ClipRect.TopLeft = UV3;

					const FVector2D& UV4 = (TexCoordsPerVertex > 4) ? LOD.VertexBuffer.GetVertexUV(i, 4) : FVector2D(1, 1);
					NewVert.ClipRect.ExtentX = UV4;

					const FVector2D& UV5 = (TexCoordsPerVertex > 5) ? LOD.VertexBuffer.GetVertexUV(i, 5) : FVector2D(1, 1);
					NewVert.ClipRect.ExtentY = UV5;
				}
			}
		}
	}
}