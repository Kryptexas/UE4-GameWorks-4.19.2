// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MeshRendering.cpp: Mesh rendering implementation.
=============================================================================*/

#include "MeshRendering.h"
#include "EngineDefines.h"
#include "ShowFlags.h"
#include "RHI.h"
#include "RenderResource.h"
#include "HitProxies.h"
#include "RenderingThread.h"
#include "VertexFactory.h"
#include "TextureResource.h"
#include "PackedNormal.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Misc/App.h"
#include "MaterialUtilities.h"
#include "Misc/FileHelper.h"
#include "RawMesh.h"
#include "SceneView.h"
#include "MeshBatch.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "LocalVertexFactory.h"
#include "DrawingPolicy.h"
#include "SkeletalMeshLODRenderData.h"

#include "RendererInterface.h"
#include "EngineModule.h"
#include "LightMapHelpers.h"
#include "ParallelFor.h"
#include "DynamicMeshBuilder.h"

#define SHOW_WIREFRAME_MESH 0
#define SAVE_INTERMEDIATE_TEXTURES 0

FColor BoxBlurSample(TArray<FColor>& InBMP, int32 X, int32 Y, int32 InImageWidth, int32 InImageHeight, bool bIsNormalMap)
{
	const int32 SampleCount = 8;
	const int32 PixelIndices[SampleCount] = { -(InImageWidth + 1), -InImageWidth, -(InImageWidth - 1),
		-1, 1,
		(InImageWidth + -1), InImageWidth, (InImageWidth + 1) };

	static const int32 PixelOffsetX[SampleCount] = { -1, 0, 1,
		-1, 1,
		-1, 0, 1 };

	int32 PixelsSampled = 0;
	FLinearColor CombinedColor = FColor::Black;

	// Take samples for blur with square indices
	for (int32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
	{
		const int32 PixelIndex = ((Y * InImageWidth) + X) + PixelIndices[SampleIndex];
		const int32 XIndex = X + PixelOffsetX[SampleIndex];

		// Check if we are not out of texture bounds
		if (InBMP.IsValidIndex(PixelIndex) && XIndex >= 0 && XIndex < InImageWidth)
		{
			const FLinearColor SampledColor = InBMP[PixelIndex].ReinterpretAsLinear();
			// Check if the pixel is a rendered one (not clear colour)
			if ((!(SampledColor.R == 1.0f && SampledColor.B == 1.0f && SampledColor.G == 0.0f)) && (!bIsNormalMap || SampledColor.B != 0.0f))
			{
				CombinedColor += SampledColor;
				++PixelsSampled;
			}
		}
	}
	CombinedColor /= PixelsSampled;

	if (PixelsSampled == 0)
	{
		return InBMP[((Y * InImageWidth) + X)];
	}

	return CombinedColor.ToFColor(false);
}

void PerformUVBorderSmear(TArray<FColor>& InOutPixels, int32 ImageWidth, int32 ImageHeight, bool bIsNormalMap)
{
	// This is ONLY possible because this is never called from multiple threads
	static TArray<FColor> Swap;
	if (Swap.Num())
	{
		Swap.SetNum(0, false);
	}
	Swap.Append(InOutPixels);

	TArray<FColor>* Current = &InOutPixels;
	TArray<FColor>* Scratch = &Swap;

	const int32 MaxIterations = 32;
	const int32 NumThreads = [&]()
	{
		return FPlatformProcess::SupportsMultithreading() ? FPlatformMisc::NumberOfCores() : 1;
	}();

	const int32 LinesPerThread = FMath::CeilToInt((float)ImageHeight / (float)NumThreads);
	int32* MagentaPixels = new int32[NumThreads];
	FMemory::Memzero(MagentaPixels, sizeof(int32) * NumThreads);

	int32 SummedMagentaPixels = 1;

	// Sampling
	int32 LoopCount = 0;
	while (SummedMagentaPixels && (LoopCount <= MaxIterations))
	{
		SummedMagentaPixels = 0;
		// Left / right, Top / down
		ParallelFor(NumThreads, [ImageWidth, ImageHeight, bIsNormalMap, MaxIterations, LoopCount, Current, Scratch, &MagentaPixels, LinesPerThread]
		(int32 Index)
		{
			const int32 StartY = FMath::CeilToInt((Index)* LinesPerThread);
			const int32 EndY = FMath::Min(FMath::CeilToInt((Index + 1) * LinesPerThread), ImageHeight);

			for (int32 Y = StartY; Y < EndY; Y++)
			{
				for (int32 X = 0; X < ImageWidth; X++)
				{
					const int32 PixelIndex = (Y * ImageWidth) + X;
					FColor& Color = (*Current)[PixelIndex];
					if ((Color.R == 255 && Color.B == 255 && Color.G == 0) || (bIsNormalMap && Color.B == 0))
					{
						MagentaPixels[Index]++;
						const FColor SampledColor = BoxBlurSample(*Scratch, X, Y, ImageWidth, ImageHeight, bIsNormalMap);
						// If it's a valid pixel
						if ((!(SampledColor.R == 255 && SampledColor.B == 255 && SampledColor.G == 0)) && (!bIsNormalMap || SampledColor.B != 0))
						{
							Color = SampledColor;
						}
						else
						{
							// If we are at the end of our iterations, replace the pixels with black
							if (LoopCount == (MaxIterations - 1))
							{
								Color = FColor(0, 0, 0, 0);
							}
						}
					}
				}
			}
		});

		for (int32 ThreadIndex = 0; ThreadIndex < NumThreads; ++ThreadIndex)
		{
			SummedMagentaPixels += MagentaPixels[ThreadIndex];
			MagentaPixels[ThreadIndex] = 0;
		}

		TArray<FColor>& Temp = *Scratch;
		Scratch = Current;
		Current = &Temp;

		LoopCount++;
	}

	if (Current != &InOutPixels)
	{
		InOutPixels.Empty();
		InOutPixels.Append(*Current);
	}

	delete[] MagentaPixels;
}

class FMeshRenderInfo : public FLightCacheInterface
{
public:
	FMeshRenderInfo(const FLightMap* InLightMap, const FShadowMap* InShadowMap, FUniformBufferRHIRef Buffer)
		: FLightCacheInterface(InLightMap, InShadowMap)
	{
		SetPrecomputedLightingBuffer(Buffer);
	}

	virtual FLightInteraction GetInteraction(const class FLightSceneProxy* LightSceneProxy) const override
	{
		return LIT_CachedLightMap;
	}
};

/**
* Canvas render item enqueued into renderer command list.
*/
class FMeshMaterialRenderItem2 : public FCanvasBaseRenderItem
{
public:
	FMeshMaterialRenderItem2(FSceneViewFamily* InViewFamily, const FRawMesh* InMesh, const FSkeletalMeshLODRenderData* InLODData, int32 LightMapIndex, int32 InMaterialIndex, const FBox2D& InTexcoordBounds, const TArray<FVector2D>& InTexCoords, const FVector2D& InSize, const FMaterialRenderProxy* InMaterialRenderProxy, const FCanvas::FTransformEntry& InTransform /*= FCanvas::FTransformEntry(FMatrix::Identity)*/, FLightMapRef LightMap, FShadowMapRef ShadowMap, FUniformBufferRHIRef Buffer) : Data(new FRenderData(
		InViewFamily,
		InMesh,
		InLODData,
		LightMapIndex,
		InMaterialIndex,
		InTexcoordBounds,
		InTexCoords,
		InSize,
		InMaterialRenderProxy,
		InTransform,
		new FMeshRenderInfo(LightMap, ShadowMap, Buffer)))
	{
	}

	~FMeshMaterialRenderItem2()
	{
	}

private:
	class FRenderData
	{
	public:
		FRenderData(
			FSceneViewFamily* InViewFamily,
			const FRawMesh* InMesh,
			const FSkeletalMeshLODRenderData* InLODData,
			int32 InLightMapIndex,
			int32 InMaterialIndex,
			const FBox2D& InTexcoordBounds,
			const TArray<FVector2D>& InTexCoords,
			const FVector2D& InSize,
			const FMaterialRenderProxy* InMaterialRenderProxy = nullptr,
			const FCanvas::FTransformEntry& InTransform = FCanvas::FTransformEntry(FMatrix::Identity),
			FLightCacheInterface* InLCI = nullptr)
			: ViewFamily(InViewFamily)
			, StaticMesh(InMesh)
			, SkeletalMesh(InLODData)
			, LightMapIndex(InLightMapIndex)
			, MaterialIndex(InMaterialIndex)
			, TexcoordBounds(InTexcoordBounds)
			, TexCoords(InTexCoords)
			, Size(InSize)
			, MaterialRenderProxy(InMaterialRenderProxy)
			, Transform(InTransform)
			, LCI(InLCI)

		{}
		FSceneViewFamily* ViewFamily;
		const FRawMesh* StaticMesh;
		const FSkeletalMeshLODRenderData* SkeletalMesh;
		int32 LightMapIndex;
		int32 MaterialIndex;
		FBox2D TexcoordBounds;
		const TArray<FVector2D>& TexCoords;
		FVector2D Size;
		const FMaterialRenderProxy* MaterialRenderProxy;
		
		FCanvas::FTransformEntry Transform;
		FLightCacheInterface* LCI;
	};
	FRenderData* Data;
public:

	static void EnqueueMaterialRender(class FCanvas* InCanvas, FSceneViewFamily* InViewFamily, const FRawMesh* InMesh, const FSkeletalMeshLODRenderData* InLODRenderData, int32 LightMapIndex, int32 InMaterialIndex, const FBox2D& InTexcoordBounds, const TArray<FVector2D>& InTexCoords, const FVector2D& InSize, const FMaterialRenderProxy* InMaterialRenderProxy, FLightMapRef LightMap, FShadowMapRef ShadowMap, FUniformBufferRHIRef Buffer)
	{
		// get sort element based on the current sort key from top of sort key stack
		FCanvas::FCanvasSortElement& SortElement = InCanvas->GetSortElement(InCanvas->TopDepthSortKey());
		// get the current transform entry from top of transform stack
		const FCanvas::FTransformEntry& TopTransformEntry = InCanvas->GetTransformStack().Top();
		// create a render batch
		FMeshMaterialRenderItem2* RenderBatch = new FMeshMaterialRenderItem2(
			InViewFamily,
			InMesh,
			InLODRenderData,
			LightMapIndex,
			InMaterialIndex,
			InTexcoordBounds,
			InTexCoords,
			InSize,
			InMaterialRenderProxy,
			TopTransformEntry,
			LightMap,
			ShadowMap, 
			Buffer);
		SortElement.RenderBatchArray.Add(RenderBatch);
	}

	static int32 FillStaticMeshData(bool bDuplicateTris, const FRawMesh& RawMesh, FRenderData& Data, TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices)
	{
		// count triangles for selected material
		int32 NumTris = 0;
		int32 TotalNumFaces = RawMesh.FaceMaterialIndices.Num();
		for (int32 FaceIndex = 0; FaceIndex < TotalNumFaces; FaceIndex++)
		{
			if (RawMesh.FaceMaterialIndices[FaceIndex] == Data.MaterialIndex)
			{
				NumTris++;
			}
		}
		if (NumTris == 0)
		{
			// there's nothing to do here
			return 0;
		}

		// vertices are not shared between triangles in FRawMesh, so NumVerts is NumTris * 3
		int32 NumVerts = NumTris * 3;

		// reserve renderer data
		OutVerts.Empty(NumVerts);
		OutIndices.Empty(bDuplicateTris ? NumVerts * 2 : NumVerts);

		float U = Data.TexcoordBounds.Min.X;
		float V = Data.TexcoordBounds.Min.Y;
		float SizeU = Data.TexcoordBounds.Max.X - Data.TexcoordBounds.Min.X;
		float SizeV = Data.TexcoordBounds.Max.Y - Data.TexcoordBounds.Min.Y;
		float ScaleX = (SizeU != 0) ? Data.Size.X / SizeU : 1.0;
		float ScaleY = (SizeV != 0) ? Data.Size.Y / SizeV : 1.0;

		// count number of texture coordinates for this mesh
		int32 NumTexcoords = 1;
		for (NumTexcoords = 1; NumTexcoords < MAX_STATIC_TEXCOORDS; NumTexcoords++)
		{
			if (RawMesh.WedgeTexCoords[NumTexcoords].Num() == 0)
				break;
		}

		// check if we should use NewUVs or original UV set
		bool bUseNewUVs = Data.TexCoords.Num() > 0;
		if (bUseNewUVs)
		{
			check(Data.TexCoords.Num() == RawMesh.WedgeTexCoords[0].Num());
			ScaleX = Data.Size.X;
			ScaleY = Data.Size.Y;
		}

		// add vertices
		int32 VertIndex = 0;
		bool bHasVertexColor = (RawMesh.WedgeColors.Num() > 0);
		for (int32 FaceIndex = 0; FaceIndex < TotalNumFaces; FaceIndex++)
		{
			if (RawMesh.FaceMaterialIndices[FaceIndex] == Data.MaterialIndex)
			{
				for (int32 Corner = 0; Corner < 3; Corner++)
				{
					int32 SrcVertIndex = FaceIndex * 3 + Corner;
					// add vertex
					FDynamicMeshVertex* Vert = new(OutVerts)FDynamicMeshVertex();
					if (!bUseNewUVs)
					{
						// compute vertex position from original UV
						const FVector2D& UV = RawMesh.WedgeTexCoords[0][SrcVertIndex];
						Vert->Position.Set((UV.X - U) * ScaleX, (UV.Y - V) * ScaleY, 0);
					}
					else
					{
						const FVector2D& UV = Data.TexCoords[SrcVertIndex];
						Vert->Position.Set(UV.X * ScaleX, UV.Y * ScaleY, 0);
					}
					Vert->SetTangents(RawMesh.WedgeTangentX[SrcVertIndex], RawMesh.WedgeTangentY[SrcVertIndex], RawMesh.WedgeTangentZ[SrcVertIndex]);
					for (int32 TexcoordIndex = 0; TexcoordIndex < NumTexcoords; TexcoordIndex++)
						Vert->TextureCoordinate[TexcoordIndex] = RawMesh.WedgeTexCoords[TexcoordIndex][SrcVertIndex];

					Vert->TextureCoordinate[6].X = RawMesh.VertexPositions[RawMesh.WedgeIndices[SrcVertIndex]].X;
					Vert->TextureCoordinate[6].Y = RawMesh.VertexPositions[RawMesh.WedgeIndices[SrcVertIndex]].Y;
					Vert->TextureCoordinate[7].X = RawMesh.VertexPositions[RawMesh.WedgeIndices[SrcVertIndex]].Z;

					Vert->Color = bHasVertexColor ? RawMesh.WedgeColors[SrcVertIndex] : FColor::White;
					// add index
					OutIndices.Add(VertIndex);
					VertIndex++;
				}
				if (bDuplicateTris)
				{
					// add the same triangle with opposite vertex order
					OutIndices.Add(VertIndex - 3);
					OutIndices.Add(VertIndex - 1);
					OutIndices.Add(VertIndex - 2);
				}
			}
		}

		return NumTris;
	}

	static int32 FillSkeletalMeshData(bool bDuplicateTris, const FSkeletalMeshLODRenderData& LODData, FRenderData& Data, TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices)
	{
		TArray<uint32> IndexData;
		LODData.MultiSizeIndexContainer.GetIndexBuffer(IndexData);

		int32 NumTris = 0;
		int32 NumVerts = 0;

		const int32 SectionCount = LODData.NumNonClothingSections();

		// count triangles and vertices for selected material
		for (int32 SectionIndex = 0; SectionIndex < SectionCount; SectionIndex++)
		{
			const FSkelMeshRenderSection& Section = LODData.RenderSections[SectionIndex];
			if (Section.MaterialIndex == Data.MaterialIndex)
			{
				NumTris += Section.NumTriangles;
				NumVerts += Section.NumVertices;
			}
		}

		if (NumTris == 0)
		{
			// there's nothing to do here
			return 0;
		}

		bool bUseNewUVs = Data.TexCoords.Num() > 0;

		if (bUseNewUVs)
		{
			// we should split all merged vertices because UVs are prepared per-corner, i.e. has
			// (NumTris * 3) vertices
			NumVerts = NumTris * 3;
		}
		
		// reserve renderer data
		OutVerts.Empty(NumVerts);
		OutIndices.Empty(bDuplicateTris ? NumVerts * 2 : NumVerts);

		float U = Data.TexcoordBounds.Min.X;
		float V = Data.TexcoordBounds.Min.Y;
		float SizeU = Data.TexcoordBounds.Max.X - Data.TexcoordBounds.Min.X;
		float SizeV = Data.TexcoordBounds.Max.Y - Data.TexcoordBounds.Min.Y;
		float ScaleX = (SizeU != 0) ? Data.Size.X / SizeU : 1.0;
		float ScaleY = (SizeV != 0) ? Data.Size.Y / SizeV : 1.0;
		uint32 DefaultColor = FColor::White.DWColor();

		int32 NumTexcoords = LODData.GetNumTexCoords();

		// check if we should use NewUVs or original UV set
		if (bUseNewUVs)
		{
			ScaleX = Data.Size.X;
			ScaleY = Data.Size.Y;
		}

		// add vertices
		if (!bUseNewUVs)
		{
			// Use original UV from mesh, render indexed mesh as indexed mesh.

			uint32 FirstVertex = 0;
			uint32 OutVertexIndex = 0;

			for (int32 SectionIndex = 0; SectionIndex < SectionCount; SectionIndex++)
			{
				const FSkelMeshRenderSection& Section = LODData.RenderSections[SectionIndex];

				const int32 NumVertsInSection = Section.NumVertices;

				if (Section.MaterialIndex == Data.MaterialIndex)
				{
					// offset to remap source mesh vertex index to destination vertex index
					int32 IndexOffset = FirstVertex - OutVertexIndex;

					// copy vertices
					int32 SrcVertIndex = FirstVertex;
					for (int32 VertIndex = 0; VertIndex < NumVertsInSection; VertIndex++)
					{
						FDynamicMeshVertex* DstVert = new(OutVerts)FDynamicMeshVertex();

						// compute vertex position from original UV
						const FVector2D UV = LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(SrcVertIndex, 0);
						DstVert->Position.Set((UV.X - U) * ScaleX, (UV.Y - V) * ScaleY, 0);

						DstVert->TangentX = LODData.StaticVertexBuffers.StaticMeshVertexBuffer.VertexTangentX(SrcVertIndex);
						DstVert->TangentZ = LODData.StaticVertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(SrcVertIndex);

						for (int32 TexcoordIndex = 0; TexcoordIndex < NumTexcoords; TexcoordIndex++)
						{
							DstVert->TextureCoordinate[TexcoordIndex] = LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(SrcVertIndex, TexcoordIndex);
						}

						DstVert->Color = LODData.StaticVertexBuffers.ColorVertexBuffer.VertexColor(SrcVertIndex);

						SrcVertIndex++;
						OutVertexIndex++;
					}

					// copy indices
					int32 Index = Section.BaseIndex;
					for (uint32 TriIndex = 0; TriIndex < Section.NumTriangles; TriIndex++)
					{
						uint32 Index0 = IndexData[Index++] - IndexOffset;
						uint32 Index1 = IndexData[Index++] - IndexOffset;
						uint32 Index2 = IndexData[Index++] - IndexOffset;
						OutIndices.Add(Index0);
						OutIndices.Add(Index1);
						OutIndices.Add(Index2);
						if (bDuplicateTris)
						{
							// add the same triangle with opposite vertex order
							OutIndices.Add(Index0);
							OutIndices.Add(Index2);
							OutIndices.Add(Index1);
						}
					}
				}
				FirstVertex += NumVertsInSection;
			}
		}
		else // bUseNewUVs
		{
			// Use external UVs. These UVs are prepared per-corner, so we should convert indexed mesh to non-indexed, without
			// sharing of vertices between triangles.

			uint32 OutVertexIndex = 0;

			for (int32 SectionIndex = 0; SectionIndex < SectionCount; SectionIndex++)
			{
				const FSkelMeshRenderSection& Section = LODData.RenderSections[SectionIndex];

				if (Section.MaterialIndex == Data.MaterialIndex)
				{
					// copy vertices
					int32 LastIndex = Section.BaseIndex + Section.NumTriangles * 3;
					for (int32 Index = Section.BaseIndex; Index < LastIndex; Index += 3)
					{
						for (int32 Corner = 0; Corner < 3; Corner++)
						{
							int32 CornerIndex = Index + Corner;
							int32 SrcVertIndex = IndexData[CornerIndex];
							FDynamicMeshVertex* DstVert = new(OutVerts)FDynamicMeshVertex();

							const FVector2D UV = LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(SrcVertIndex, 0);
							DstVert->Position.Set(UV.X * ScaleX, UV.Y * ScaleY, 0);

							DstVert->TangentX = LODData.StaticVertexBuffers.StaticMeshVertexBuffer.VertexTangentX(SrcVertIndex);
							DstVert->TangentZ = LODData.StaticVertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(SrcVertIndex);

							for (int32 TexcoordIndex = 0; TexcoordIndex < NumTexcoords; TexcoordIndex++)
							{
								DstVert->TextureCoordinate[TexcoordIndex] = LODData.StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(SrcVertIndex, TexcoordIndex);
							}

							DstVert->Color = LODData.StaticVertexBuffers.ColorVertexBuffer.VertexColor(SrcVertIndex);

							OutIndices.Add(OutVertexIndex);
							OutVertexIndex++;
						}
						if (bDuplicateTris)
						{
							// add the same triangle with opposite vertex order
							OutIndices.Add(OutVertexIndex - 3);
							OutIndices.Add(OutVertexIndex - 1);
							OutIndices.Add(OutVertexIndex - 2);
						}
					}
				}
			}
		}

		return NumTris;
	}

	static int32 FillQuadData(FRenderData& Data, TArray<FDynamicMeshVertex>& OutVerts, TArray<uint32>& OutIndices)
	{
		OutVerts.Empty(4);
		OutIndices.Empty(6);

		float U = Data.TexcoordBounds.Min.X;
		float V = Data.TexcoordBounds.Min.Y;
		float SizeU = Data.TexcoordBounds.Max.X - Data.TexcoordBounds.Min.X;
		float SizeV = Data.TexcoordBounds.Max.Y - Data.TexcoordBounds.Min.Y;
		float ScaleX = (SizeU != 0) ? Data.Size.X / SizeU : 1.0;
		float ScaleY = (SizeV != 0) ? Data.Size.Y / SizeV : 1.0;

		// add vertices
		for (int32 VertIndex = 0; VertIndex < 4; VertIndex++)
		{
			FDynamicMeshVertex* Vert = new(OutVerts)FDynamicMeshVertex();

			int X = VertIndex & 1;
			int Y = (VertIndex >> 1) & 1;

			Vert->Position.Set(ScaleX * X, ScaleY * Y, 0);
			Vert->SetTangents(FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1));
			FMemory::Memzero(&Vert->TextureCoordinate, sizeof(Vert->TextureCoordinate));
			Vert->TextureCoordinate[0].Set(U + SizeU * X, V + SizeV * Y);
			Vert->Color = FColor::White;
		}

		// add indices
		static const uint32 Indices[6] = { 0, 2, 1, 2, 3, 1 };
		OutIndices.Append(Indices, 6);

		return 2;
	}

	static void RenderMaterial(FRHICommandListImmediate& RHICmdList, FDrawingPolicyRenderState& DrawRenderState, const class FSceneView& View, FRenderData& Data)
	{
		// Check if material is TwoSided - single-sided materials should be rendered with normal and reverse
		// triangle corner orders, to avoid problems with inside-out meshes or mesh parts. Note:
		// FExportMaterialProxy::GetMaterial() (which is really called here) ignores 'InFeatureLevel' parameter.
		const FMaterial* Material = Data.MaterialRenderProxy->GetMaterial(GMaxRHIFeatureLevel);
		bool bIsMaterialTwoSided = Material->IsTwoSided();

		TArray<FDynamicMeshVertex> Verts;
		TArray<uint32> Indices;

		int32 NumTris = 0;
		if (Data.StaticMesh != nullptr)
		{
			check(Data.SkeletalMesh == nullptr)
				NumTris = FillStaticMeshData(!bIsMaterialTwoSided, *Data.StaticMesh, Data, Verts, Indices);
		}
		else if (Data.SkeletalMesh != nullptr)
		{
			NumTris = FillSkeletalMeshData(!bIsMaterialTwoSided, *Data.SkeletalMesh, Data, Verts, Indices);
		}
		else
		{
			// both are null, use simple rectangle
			NumTris = FillQuadData(Data, Verts, Indices);
		}
		if (NumTris == 0)
		{
			// there's nothing to do here
			return;
		}

		uint32 LightMapCoordinateIndex = (uint32)Data.LightMapIndex;
		LightMapCoordinateIndex = LightMapCoordinateIndex < MAX_STATIC_TEXCOORDS ? LightMapCoordinateIndex : MAX_STATIC_TEXCOORDS - 1;

		FDynamicMeshBuilder DynamicMeshBuilder(View.GetFeatureLevel(), MAX_STATIC_TEXCOORDS, LightMapCoordinateIndex);
		DynamicMeshBuilder.AddVertices(Verts);
		DynamicMeshBuilder.AddTriangles(Indices);

		FMeshBatch MeshElement;
		FMeshBuilderOneFrameResources OneFrameResource;
		DynamicMeshBuilder.GetMeshElement(FMatrix::Identity, Data.MaterialRenderProxy, SDPG_Foreground, true, false, 0, OneFrameResource, MeshElement);

		check(OneFrameResource.IsValidForRendering());

		Data.LCI->SetPrecomputedLightingBuffer(LightMapHelpers::CreateDummyPrecomputedLightingUniformBuffer(UniformBuffer_SingleFrame, GMaxRHIFeatureLevel, Data.LCI));
		MeshElement.LCI = Data.LCI;
		MeshElement.ReverseCulling = false;

#if SHOW_WIREFRAME_MESH
		MeshElement.bWireframe = true;
#endif

		GetRendererModule().DrawTileMesh(RHICmdList, DrawRenderState, View, MeshElement, false /*bIsHitTesting*/, FHitProxyId());
	}

	virtual bool Render_RenderThread(FRHICommandListImmediate& RHICmdList, FDrawingPolicyRenderState& DrawRenderState, const FCanvas* Canvas)
	{
		checkSlow(Data);
		// current render target set for the canvas
		const FRenderTarget* CanvasRenderTarget = Canvas->GetRenderTarget();
		FIntRect ViewRect(FIntPoint(0, 0), CanvasRenderTarget->GetSizeXY());

		// make a temporary view
		FSceneViewInitOptions ViewInitOptions;
		ViewInitOptions.ViewFamily = Data->ViewFamily;
		ViewInitOptions.SetViewRectangle(ViewRect);
		ViewInitOptions.ViewOrigin = FVector::ZeroVector;
		ViewInitOptions.ViewRotationMatrix = FMatrix::Identity;
		ViewInitOptions.ProjectionMatrix = Data->Transform.GetMatrix();
		ViewInitOptions.BackgroundColor = FLinearColor::Black;
		ViewInitOptions.OverlayColor = FLinearColor::White;

		bool bNeedsToSwitchVerticalAxis = RHINeedsToSwitchVerticalAxis(Canvas->GetShaderPlatform()) && !Canvas->GetAllowSwitchVerticalAxis();
		check(bNeedsToSwitchVerticalAxis == false);

		FSceneView* View = new FSceneView(ViewInitOptions);

		RenderMaterial(RHICmdList, DrawRenderState, *View, *Data);

		delete View;
		if (Canvas->GetAllowedModes() & FCanvas::Allow_DeleteOnRender)
		{
			delete Data;
		}
		if (Canvas->GetAllowedModes() & FCanvas::Allow_DeleteOnRender)
		{
			Data = NULL;
		}
		return true;
	}

	virtual bool Render_GameThread(const FCanvas* Canvas)
	{
		checkSlow(Data);
		// current render target set for the canvas
		const FRenderTarget* CanvasRenderTarget = Canvas->GetRenderTarget();
		FIntRect ViewRect(FIntPoint(0, 0), CanvasRenderTarget->GetSizeXY());

		// make a temporary view
		FSceneViewInitOptions ViewInitOptions;
		ViewInitOptions.ViewFamily = Data->ViewFamily;
		ViewInitOptions.SetViewRectangle(ViewRect);
		ViewInitOptions.ViewOrigin = FVector::ZeroVector;
		ViewInitOptions.ViewRotationMatrix = FMatrix::Identity;
		ViewInitOptions.ProjectionMatrix = Data->Transform.GetMatrix();
		ViewInitOptions.BackgroundColor = FLinearColor::Black;
		ViewInitOptions.OverlayColor = FLinearColor::White;

		FSceneView* View = new FSceneView(ViewInitOptions);

		bool bNeedsToSwitchVerticalAxis = RHINeedsToSwitchVerticalAxis(Canvas->GetShaderPlatform()) && !Canvas->GetAllowSwitchVerticalAxis();
		check(bNeedsToSwitchVerticalAxis == false);
		struct FDrawMaterialParameters
		{
			FSceneView* View;
			FRenderData* RenderData;
			uint32 AllowedCanvasModes;
		};
		FDrawMaterialParameters DrawMaterialParameters =
		{
			View,
			Data,
			Canvas->GetAllowedModes()
		};

		FDrawMaterialParameters Parameters = DrawMaterialParameters;
		ENQUEUE_RENDER_COMMAND(DrawMaterialCommand)(
			[Parameters](FRHICommandListImmediate& RHICmdList)
			{
				FDrawingPolicyRenderState DrawRenderState(*Parameters.View);

				// disable depth test & writes
				DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());

				RenderMaterial(RHICmdList, DrawRenderState, *Parameters.View, *Parameters.RenderData);

				delete Parameters.View;
				if (Parameters.AllowedCanvasModes & FCanvas::Allow_DeleteOnRender)
				{
					delete Parameters.RenderData;
				}
			});
		if (Canvas->GetAllowedModes() & FCanvas::Allow_DeleteOnRender)
		{
			Data = NULL;
		}
		return true;
	}
};

bool FMeshRenderer::RenderMaterial(struct FMaterialMergeData& InMaterialData, FMaterialRenderProxy* InMaterialProxy, EMaterialProperty InMaterialProperty, UTextureRenderTarget2D* InRenderTarget, TArray<FColor>& OutBMP)
{
	check(IsInGameThread());
	check(InRenderTarget);
	FTextureRenderTargetResource* RTResource = InRenderTarget->GameThread_GetRenderTargetResource();

	{
		// Create a canvas for the render target and clear it to black
		FCanvas Canvas(RTResource, NULL, FApp::GetCurrentTime() - GStartTime, FApp::GetDeltaTime(), FApp::GetCurrentTime() - GStartTime, GMaxRHIFeatureLevel);

#if 0	// original FFlattenMaterial code - kept here for comparison

#if !SHOW_WIREFRAME_MESH
		Canvas.Clear(InRenderTarget->ClearColor);
#else
		Canvas.Clear(FLinearColor::Yellow);
#endif

		FVector2D UV0(InMaterialData.TexcoordBounds.Min.X, InMaterialData.TexcoordBounds.Min.Y);
		FVector2D UV1(InMaterialData.TexcoordBounds.Max.X, InMaterialData.TexcoordBounds.Max.Y);
		FCanvasTileItem TileItem(FVector2D(0.0f, 0.0f), InMaterialProxy, FVector2D(InRenderTarget->SizeX, InRenderTarget->SizeY), UV0, UV1);
		TileItem.bFreezeTime = true;
		Canvas.DrawItem(TileItem);

		Canvas.Flush_GameThread();
#else

		// create ViewFamily
		float CurrentRealTime = 0.f;
		float CurrentWorldTime = 0.f;
		float DeltaWorldTime = 0.f;

		const FRenderTarget* CanvasRenderTarget = Canvas.GetRenderTarget();
		FSceneViewFamily ViewFamily(FSceneViewFamily::ConstructionValues(
			CanvasRenderTarget,
			NULL,
			FEngineShowFlags(ESFIM_Game))
			.SetWorldTimes(CurrentWorldTime, DeltaWorldTime, CurrentRealTime)
			.SetGammaCorrection(CanvasRenderTarget->GetDisplayGamma()));
		
#if !SHOW_WIREFRAME_MESH
		Canvas.Clear(InRenderTarget->ClearColor);
#else
		Canvas.Clear(FLinearColor::Yellow);
#endif

		// add item for rendering
		FMeshMaterialRenderItem2::EnqueueMaterialRender(
			&Canvas,
			&ViewFamily,
			InMaterialData.Mesh,
			InMaterialData.LODData,
			InMaterialData.LightMapIndex,
			InMaterialData.MaterialIndex,
			InMaterialData.TexcoordBounds,
			InMaterialData.TexCoords,
			FVector2D(InRenderTarget->SizeX, InRenderTarget->SizeY),
			InMaterialProxy,
			InMaterialData.LightMap,
			InMaterialData.ShadowMap,
			InMaterialData.Buffer
			);

		// In case of running commandlet the RHI is not fully set up on first flush so do it twice TODO 
		static bool TempForce = true;
		if (IsRunningCommandlet() && TempForce)
		{
			Canvas.Flush_GameThread();
			TempForce = false;
		}

		// rendering is performed here
		Canvas.Flush_GameThread();
#endif

		FlushRenderingCommands();
		Canvas.SetRenderTarget_GameThread(NULL);
		FlushRenderingCommands();
	}

	bool bNormalmap = (InMaterialProperty == MP_Normal);
	FReadSurfaceDataFlags ReadPixelFlags(bNormalmap ? RCM_SNorm : RCM_UNorm);
	ReadPixelFlags.SetLinearToGamma(false);

	bool result = false;

	if (InMaterialProperty != MP_EmissiveColor)
	{
		// Read normal color image
		result = RTResource->ReadPixels(OutBMP, ReadPixelFlags);
	}
	else
	{
		// Read HDR emissive image
		TArray<FFloat16Color> Color16;
		result = RTResource->ReadFloat16Pixels(Color16);
		// Find color scale value
		float MaxValue = 0;
		for (int32 PixelIndex = 0; PixelIndex < Color16.Num(); PixelIndex++)
		{
			FFloat16Color& Pixel16 = Color16[PixelIndex];
			float R = Pixel16.R.GetFloat();
			float G = Pixel16.G.GetFloat();
			float B = Pixel16.B.GetFloat();
			float Max = FMath::Max3(R, G, B);
			if (Max > MaxValue)
			{
				MaxValue = Max;
			}
		}
		if (MaxValue <= 0.01f)
		{
			// Black emissive, drop it
			return false;
		}
		// Now convert Float16 to Color
		OutBMP.SetNumUninitialized(Color16.Num());
		float Scale = 255.0f / MaxValue;
		for (int32 PixelIndex = 0; PixelIndex < Color16.Num(); PixelIndex++)
		{
			FFloat16Color& Pixel16 = Color16[PixelIndex];
			FColor& Pixel8 = OutBMP[PixelIndex];
			Pixel8.R = (uint8)FMath::RoundToInt(Pixel16.R.GetFloat() * Scale);
			Pixel8.G = (uint8)FMath::RoundToInt(Pixel16.G.GetFloat() * Scale);
			Pixel8.B = (uint8)FMath::RoundToInt(Pixel16.B.GetFloat() * Scale);
		}
		
	}

	PerformUVBorderSmear(OutBMP, InRenderTarget->GetSurfaceWidth(), InRenderTarget->GetSurfaceHeight(), bNormalmap);
#ifdef SAVE_INTERMEDIATE_TEXTURES
	FString FilenameString = FString::Printf(
		TEXT( "D:/TextureTest/%s-mat%d-prop%d.bmp"),
		*InMaterialProxy->GetFriendlyName(), InMaterialData.MaterialIndex, (int32)InMaterialProperty);
	FFileHelper::CreateBitmap(*FilenameString, InRenderTarget->GetSurfaceWidth(), InRenderTarget->GetSurfaceHeight(), OutBMP.GetData());
#endif // SAVE_INTERMEDIATE_TEXTURES
	return result;
}

bool FMeshRenderer::RenderMaterialTexCoordScales(struct FMaterialMergeData& InMaterialData, FMaterialRenderProxy* InMaterialProxy, UTextureRenderTarget2D* InRenderTarget, TArray<FFloat16Color>& OutScales)
{
	check(IsInGameThread());
	check(InRenderTarget);
	// create ViewFamily
	float CurrentRealTime = 0.f;
	float CurrentWorldTime = 0.f;
	float DeltaWorldTime = 0.f;

	// Create a canvas for the render target and clear it to black
	FTextureRenderTargetResource* RTResource = InRenderTarget->GameThread_GetRenderTargetResource();
	FCanvas Canvas(RTResource, NULL, FApp::GetCurrentTime() - GStartTime, FApp::GetDeltaTime(), FApp::GetCurrentTime() - GStartTime, GMaxRHIFeatureLevel);
	const FRenderTarget* CanvasRenderTarget = Canvas.GetRenderTarget();
	Canvas.Clear(FLinearColor::Black);

	// Set show flag view mode to output tex coord scale
	FEngineShowFlags ShowFlags(ESFIM_Game);
	ApplyViewMode(VMI_MaterialTextureScaleAccuracy, false, ShowFlags);
	ShowFlags.OutputMaterialTextureScales = true; // This will bind the DVSM_OutputMaterialTextureScales

	FSceneViewFamily ViewFamily(FSceneViewFamily::ConstructionValues(CanvasRenderTarget, nullptr, ShowFlags)
		.SetWorldTimes(CurrentWorldTime, DeltaWorldTime, CurrentRealTime)
		.SetGammaCorrection(CanvasRenderTarget->GetDisplayGamma()));

	// The next line ensures a constant view vector of (0,0,1) for all pixels. Required because here SVPositionToTranslatedWorld is identity, making excessive view angle increase per pixel.
	// That creates bad side effects for anything that depends on the view vector, like parallax or bump offset mappings. For those, we want the tangent
	// space view vector to be perpendicular to the surface in order to generate the same results as if the feature was turned off. Which gives the good results
	// since any sub height sampling would in pratice requires less and less texture resolution, where as we are only concerned about the highest resolution the material needs.
	// This can be seen in the debug view mode, by a checkboard of white and cyan (up to green) values. The white value meaning the highest resolution taken is the good one
	// (blue meaning the texture has more resolution than required). Checkboard are only possible when a texture is sampled several times, like in parallax.
	//
	// Additionnal to affecting the view vector, it also forces a constant world position value, zeroing any textcoord scales that depends on the world position (as the UV don't change).
	// This is alright thought since the uniform quad can obviously not compute a valid mapping for world space texture mapping (only rendering the mesh at its world position could fix that).
	// The zero scale will be caught as an error, and the computed scale will fallback to 1.f
	ViewFamily.bNullifyWorldSpacePosition = true;

	// add item for rendering
	FMeshMaterialRenderItem2::EnqueueMaterialRender(
		&Canvas,
		&ViewFamily,
		InMaterialData.Mesh,
		InMaterialData.LODData,
		InMaterialData.LightMapIndex,
		InMaterialData.MaterialIndex,
		InMaterialData.TexcoordBounds,
		InMaterialData.TexCoords,
		FVector2D(InRenderTarget->SizeX, InRenderTarget->SizeY),
		InMaterialProxy,
		InMaterialData.LightMap,
		InMaterialData.ShadowMap,
		InMaterialData.Buffer
		);

	// rendering is performed here
	Canvas.Flush_GameThread();

	FlushRenderingCommands();
	Canvas.SetRenderTarget_GameThread(NULL);
	FlushRenderingCommands();

	return RTResource->ReadFloat16Pixels(OutScales);
}