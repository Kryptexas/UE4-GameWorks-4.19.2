// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "SlateMeshData.h"


static void StaticMeshToSlateRenderData(const UStaticMesh& DataSource, TArray<FSlateMeshVertex>& OutSlateVerts, TArray<uint32>& OutIndexes)
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
				// Copy Position
				const FVector& Position = LOD.PositionVertexBuffer.VertexPosition(i);
				
				// Copy Color
				FColor Color = (LOD.ColorVertexBuffer.GetNumVertices() > 0) ? LOD.ColorVertexBuffer.VertexColor(i) : FColor::White;
				
				// Copy all the UVs that we have, and as many as we can fit.
				const FVector2D& UV0 = (TexCoordsPerVertex > 0) ? LOD.VertexBuffer.GetVertexUV(i, 0) : FVector2D(1, 1);

				const FVector2D& UV1 = (TexCoordsPerVertex > 1) ? LOD.VertexBuffer.GetVertexUV(i, 1) : FVector2D(1, 1);

				const FVector2D& UV2 = (TexCoordsPerVertex > 2) ? LOD.VertexBuffer.GetVertexUV(i, 2) : FVector2D(1, 1);

				const FVector2D& UV3 = (TexCoordsPerVertex > 3) ? LOD.VertexBuffer.GetVertexUV(i, 3) : FVector2D(1, 1);

				const FVector2D& UV4 = (TexCoordsPerVertex > 4) ? LOD.VertexBuffer.GetVertexUV(i, 4) : FVector2D(1, 1);

				const FVector2D& UV5 = (TexCoordsPerVertex > 5) ? LOD.VertexBuffer.GetVertexUV(i, 5) : FVector2D(1, 1);

				OutSlateVerts.Add(FSlateMeshVertex(
					FVector2D(Position.X, Position.Y),
					Color,
					UV0,
					UV1,
					UV2,
					UV3,
					UV4,
					UV5
				));
			}
		}
	}
}


USlateMeshData::USlateMeshData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void USlateMeshData::InitFromStaticMesh(const UStaticMesh& InSourceMesh)
{
	Material = InSourceMesh.GetMaterial(0);
	StaticMeshToSlateRenderData(InSourceMesh, VertexData, IndexData );
	
}