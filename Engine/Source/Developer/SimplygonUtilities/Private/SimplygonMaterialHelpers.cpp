#include "SimplygonUtilitiesPrivatePCH.h"
#include "EngineModule.h"
#include "LocalVertexFactory.h"
#include "MeshBatch.h"
#include "RendererInterface.h"
#include "RawMesh.h"
#include "MaterialExportUtils.h"
#include "MaterialCompiler.h"
#include "ThumbnailHelpers.h" // for FClassThumbnailScene

#include "SimplygonUtilitiesModule.h"

#include "MaterialUtilities.h"


// NOTE: if this code would fail in cook commandlet, check FApp::CanEverRender() function. It disables rendering
// when commandlet is running. We should disallow IsRunningCommandlet() to return 'true' when rendering a material.
// By the way, UE4/Github has pull request https://github.com/EpicGames/UnrealEngine/pull/797 which allows to do
// the same. This should be integrated into UE4.9 with CL 2523559.


//#define SHOW_WIREFRAME_MESH 1 // should use "reuse UVs" for better visualization effect
//#define VISUALIZE_DILATION 1
//#define SAVE_INTERMEDIATE_TEXTURES		TEXT("C:/TEMP")

//PRAGMA_DISABLE_OPTIMIZATION

// Vertex data. Reference: TileRendering.cpp

/** 
* Vertex data for a screen quad.
*/
struct FMaterialMeshVertex
{
	FVector			Position;
	FPackedNormal	TangentX,
					TangentZ;
	uint32			Color;
	FVector2D		TextureCoordinate[MAX_STATIC_TEXCOORDS];

	void SetTangents(const FVector& InTangentX, const FVector& InTangentY, const FVector& InTangentZ)
	{
		TangentX = InTangentX;
		TangentZ = InTangentZ;
		// store determinant of basis in w component of normal vector
		TangentZ.Vector.W = GetBasisDeterminantSign(InTangentX,InTangentY,InTangentZ) < 0.0f ? 0 : 255;
	}
};

/**
 * A dummy vertex buffer used to give the FMeshVertexFactory something to reference as a stream source.
 */
class FMaterialMeshVertexBuffer : public FVertexBuffer
{
public:
	virtual void InitRHI() override
	{
		FRHIResourceCreateInfo CreateInfo;
		VertexBufferRHI = RHICreateVertexBuffer(sizeof(FMaterialMeshVertex),BUF_Static,CreateInfo);
	}
};
TGlobalResource<FMaterialMeshVertexBuffer> GDummyMeshRendererVertexBuffer;

/**
 * Vertex factory for rendering meshes with materials.
 */
class FMeshVertexFactory : public FLocalVertexFactory
{
public:

	/** Default constructor. */
	FMeshVertexFactory()
	{
		FLocalVertexFactory::DataType Data;

		// position
		Data.PositionComponent = FVertexStreamComponent(
			&GDummyMeshRendererVertexBuffer,
			STRUCT_OFFSET(FMaterialMeshVertex,Position),
			sizeof(FMaterialMeshVertex),
			VET_Float3
			);
		// tangents
		Data.TangentBasisComponents[0] = FVertexStreamComponent(
			&GDummyMeshRendererVertexBuffer,
			STRUCT_OFFSET(FMaterialMeshVertex,TangentX),
			sizeof(FMaterialMeshVertex),
			VET_PackedNormal
			);
		Data.TangentBasisComponents[1] = FVertexStreamComponent(
			&GDummyMeshRendererVertexBuffer,
			STRUCT_OFFSET(FMaterialMeshVertex,TangentZ),
			sizeof(FMaterialMeshVertex),
			VET_PackedNormal
			);
		// color
		Data.ColorComponent = FVertexStreamComponent(
			&GDummyMeshRendererVertexBuffer,
			STRUCT_OFFSET(FMaterialMeshVertex,Color),
			sizeof(FMaterialMeshVertex),
			VET_Color
			);
		// UVs
		int32 UVIndex;
		for (UVIndex = 0; UVIndex < MAX_STATIC_TEXCOORDS - 1; UVIndex += 2)
		{
			Data.TextureCoordinates.Add(FVertexStreamComponent(
				&GDummyMeshRendererVertexBuffer,
				STRUCT_OFFSET(FMaterialMeshVertex,TextureCoordinate) + sizeof(FVector2D)* UVIndex,
				sizeof(FMaterialMeshVertex),
				VET_Float4
				));
		}
		// possible last UV channel if we have an odd number (by the way, MAX_STATIC_TEXCOORDS is even value, so most
		// likely the following code will never be executed)
		if (UVIndex < MAX_STATIC_TEXCOORDS)
		{
			Data.TextureCoordinates.Add(FVertexStreamComponent(
				&GDummyMeshRendererVertexBuffer,
				STRUCT_OFFSET(FMaterialMeshVertex,TextureCoordinate) + sizeof(FVector2D)* UVIndex,
				sizeof(FMaterialMeshVertex),
				VET_Float2
				));
		}
		

		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FMeshVertexFactoryConstructor,
			FMeshVertexFactory*, FactoryParam, this,
			FLocalVertexFactory::DataType, DataParam, Data,
			{
				FactoryParam->SetData(DataParam);
			}
		);

		FlushRenderingCommands();
		//SetData(Data);
	}
};
static TGlobalResource<FMeshVertexFactory> GMeshVertexFactory;

/**
 * Canvas render item enqueued into renderer command list.
 */
class FSimplygonMaterialRenderItem : public FCanvasBaseRenderItem
{
public:
	FSimplygonMaterialRenderItem(
		FSceneViewFamily* InViewFamily,
		const FRawMesh* InMesh,
		const FStaticLODModel* InLODModel,
		int32 InMaterialIndex,
		const FBox2D& InTexcoordBounds,
		const TArray<FVector2D>& InTexCoords,
		const FVector2D& InSize,
		const FMaterialRenderProxy* InMaterialRenderProxy,
		const FCanvas::FTransformEntry& InTransform=FCanvas::FTransformEntry(FMatrix::Identity) )
		:	Data(new FRenderData(
			InViewFamily,
			InMesh,
			InLODModel,
			InMaterialIndex,
			InTexcoordBounds,
			InTexCoords,
			InSize,
			InMaterialRenderProxy,
			InTransform))
	{}
	virtual ~FSimplygonMaterialRenderItem() override
	{}

private:
	class FRenderData
	{
	public:
		FRenderData(
			FSceneViewFamily* InViewFamily,
			const FRawMesh* InMesh,
			const FStaticLODModel* InLODModel,
			int32 InMaterialIndex,
			const FBox2D& InTexcoordBounds,
			const TArray<FVector2D>& InTexCoords,
			const FVector2D& InSize,
			const FMaterialRenderProxy* InMaterialRenderProxy=NULL,
			const FCanvas::FTransformEntry& InTransform=FCanvas::FTransformEntry(FMatrix::Identity) )
			:	ViewFamily(InViewFamily)
			,	StaticMesh(InMesh)
			,	SkeletalMesh(InLODModel)
			,	MaterialIndex(InMaterialIndex)
			,	TexcoordBounds(InTexcoordBounds)
			,	TexCoords(InTexCoords)
			,	Size(InSize)
			,	MaterialRenderProxy(InMaterialRenderProxy)
			,	Transform(InTransform)
		{}
		FSceneViewFamily* ViewFamily;
		const FRawMesh* StaticMesh;
		const FStaticLODModel* SkeletalMesh;
		int32 MaterialIndex;
		FBox2D TexcoordBounds;
		const TArray<FVector2D>& TexCoords;
		FVector2D Size;
		const FMaterialRenderProxy* MaterialRenderProxy;
		FCanvas::FTransformEntry Transform;
	};
	FRenderData* Data;	

public:
	// Reference: FCanvasTileItem::RenderMaterialTile
	static void EnqueueMaterialRender(
		class FCanvas* InCanvas,
		FSceneViewFamily* InViewFamily,
		const FRawMesh* InMesh,
		const FStaticLODModel* InLODModel,
		int32 InMaterialIndex,
		const FBox2D& InTexcoordBounds,
		const TArray<FVector2D>& InTexCoords,
		const FVector2D& InSize,
		const FMaterialRenderProxy* InMaterialRenderProxy)
	{
		// get sort element based on the current sort key from top of sort key stack
		FCanvas::FCanvasSortElement& SortElement = InCanvas->GetSortElement(InCanvas->TopDepthSortKey());
		// get the current transform entry from top of transform stack
		const FCanvas::FTransformEntry& TopTransformEntry = InCanvas->GetTransformStack().Top();	
		// create a render batch
		FSimplygonMaterialRenderItem* RenderBatch = new FSimplygonMaterialRenderItem(
			InViewFamily,
			InMesh,
			InLODModel,
			InMaterialIndex,
			InTexcoordBounds,
			InTexCoords,
			InSize,
			InMaterialRenderProxy,
			TopTransformEntry);
		SortElement.RenderBatchArray.Add(RenderBatch);
	}

	static int32 FillStaticMeshData(
		bool bDuplicateTris,
		const FRawMesh& RawMesh,
		FRenderData& Data,
		TArray<FMaterialMeshVertex>& OutVerts,
		TArray<int32>& OutIndices)
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
		OutIndices.Empty(bDuplicateTris ? NumVerts * 2: NumVerts);

		float U = Data.TexcoordBounds.Min.X;
		float V = Data.TexcoordBounds.Min.Y;
		float SizeU = Data.TexcoordBounds.Max.X - Data.TexcoordBounds.Min.X;
		float SizeV = Data.TexcoordBounds.Max.Y - Data.TexcoordBounds.Min.Y;
		float ScaleX = (SizeU != 0) ? Data.Size.X / SizeU : 1.0;
		float ScaleY = (SizeV != 0) ? Data.Size.Y / SizeV : 1.0;
		uint32 DefaultColor = FColor::White.DWColor();

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
					FMaterialMeshVertex* Vert = new(OutVerts) FMaterialMeshVertex();
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
					Vert->Color = bHasVertexColor ? RawMesh.WedgeColors[SrcVertIndex].DWColor() : DefaultColor;
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

	static int32 FillSkeletalMeshData(
		bool bDuplicateTris,
		const FStaticLODModel& LODModel,
		FRenderData& Data,
		TArray<FMaterialMeshVertex>& OutVerts,
		TArray<int32>& OutIndices)
	{
		TArray<FSoftSkinVertex> Vertices;
		FMultiSizeIndexContainerData IndexData;
		LODModel.GetVertices(Vertices);
		LODModel.MultiSizeIndexContainer.GetIndexBufferData(IndexData);

		int32 NumTris = 0;
		int32 NumVerts = 0;

#if WITH_APEX_CLOTHING
		const int32 SectionCount = LODModel.NumNonClothingSections();
#else
		const int32 SectionCount = LODModel.Sections.Num();
#endif // #if WITH_APEX_CLOTHING

		// count triangles and vertices for selected material
		for (int32 SectionIndex = 0; SectionIndex < SectionCount; SectionIndex++)
		{
			const FSkelMeshSection& Section = LODModel.Sections[SectionIndex];
			if (Section.MaterialIndex == Data.MaterialIndex)
			{
				NumTris += Section.NumTriangles;
				NumVerts += LODModel.Chunks[Section.ChunkIndex].GetNumVertices();
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
		OutIndices.Empty(bDuplicateTris ? NumVerts * 2: NumVerts);

		float U = Data.TexcoordBounds.Min.X;
		float V = Data.TexcoordBounds.Min.Y;
		float SizeU = Data.TexcoordBounds.Max.X - Data.TexcoordBounds.Min.X;
		float SizeV = Data.TexcoordBounds.Max.Y - Data.TexcoordBounds.Min.Y;
		float ScaleX = (SizeU != 0) ? Data.Size.X / SizeU : 1.0;
		float ScaleY = (SizeV != 0) ? Data.Size.Y / SizeV : 1.0;
		uint32 DefaultColor = FColor::White.DWColor();

		int32 NumTexcoords = LODModel.NumTexCoords;

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
				const FSkelMeshSection& Section = LODModel.Sections[SectionIndex];
				const FSkelMeshChunk& Chunk = LODModel.Chunks[Section.ChunkIndex];

				const int32 NumVertsInChunk = Chunk.GetNumVertices();

				if (Section.MaterialIndex == Data.MaterialIndex)
				{
					// offset to remap source mesh vertex index to destination vertex index
					int32 IndexOffset = FirstVertex - OutVertexIndex;

					// copy vertices
					int32 SrcVertIndex = FirstVertex;
					for (int32 VertIndex = 0; VertIndex < NumVertsInChunk; VertIndex++)
					{
						const FSoftSkinVertex& SrcVert = Vertices[SrcVertIndex];
						FMaterialMeshVertex* DstVert = new(OutVerts) FMaterialMeshVertex();

						// compute vertex position from original UV
						const FVector2D& UV = SrcVert.UVs[0];
						DstVert->Position.Set((UV.X - U) * ScaleX, (UV.Y - V) * ScaleY, 0);

						DstVert->SetTangents(SrcVert.TangentX, SrcVert.TangentY, SrcVert.TangentZ);
						for (int32 TexcoordIndex = 0; TexcoordIndex < NumTexcoords; TexcoordIndex++)
							DstVert->TextureCoordinate[TexcoordIndex] = SrcVert.UVs[TexcoordIndex];
						DstVert->Color = SrcVert.Color.DWColor();

						SrcVertIndex++;
						OutVertexIndex++;
					}

					// copy indices
					int32 Index = Section.BaseIndex;
					for (uint32 TriIndex = 0; TriIndex < Section.NumTriangles; TriIndex++)
					{
						uint32 Index0 = IndexData.Indices[Index++] - IndexOffset;
						uint32 Index1 = IndexData.Indices[Index++] - IndexOffset;
						uint32 Index2 = IndexData.Indices[Index++] - IndexOffset;
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
				FirstVertex += NumVertsInChunk;
			}
		}
		else // bUseNewUVs
		{
			// Use external UVs. These UVs are prepared per-corner, so we should convert indexed mesh to non-indexed, without
			// sharing of vertices between triangles.

			uint32 OutVertexIndex = 0;

			for (int32 SectionIndex = 0; SectionIndex < SectionCount; SectionIndex++)
			{
				const FSkelMeshSection& Section = LODModel.Sections[SectionIndex];

				if (Section.MaterialIndex == Data.MaterialIndex)
				{
					// copy vertices
					int32 LastIndex = Section.BaseIndex + Section.NumTriangles * 3;
					for (int32 Index = Section.BaseIndex; Index < LastIndex; Index += 3)
					{
						for (int32 Corner = 0; Corner < 3; Corner++)
						{
							int32 CornerIndex = Index + Corner;
							int32 SrcVertIndex = IndexData.Indices[CornerIndex];
							const FSoftSkinVertex& SrcVert = Vertices[SrcVertIndex];
							FMaterialMeshVertex* DstVert = new(OutVerts) FMaterialMeshVertex();

							const FVector2D& UV = Data.TexCoords[CornerIndex];
							DstVert->Position.Set(UV.X * ScaleX, UV.Y * ScaleY, 0);

							DstVert->SetTangents(SrcVert.TangentX, SrcVert.TangentY, SrcVert.TangentZ);
							for (int32 TexcoordIndex = 0; TexcoordIndex < NumTexcoords; TexcoordIndex++)
								DstVert->TextureCoordinate[TexcoordIndex] = SrcVert.UVs[TexcoordIndex];
							DstVert->Color = SrcVert.Color.DWColor();

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

	static int32 FillQuadData(FRenderData& Data, TArray<FMaterialMeshVertex>& OutVerts, TArray<int32>& OutIndices)
	{
		OutVerts.Empty(4);
		OutIndices.Empty(6);

		float U = Data.TexcoordBounds.Min.X;
		float V = Data.TexcoordBounds.Min.Y;
		float SizeU = Data.TexcoordBounds.Max.X - Data.TexcoordBounds.Min.X;
		float SizeV = Data.TexcoordBounds.Max.Y - Data.TexcoordBounds.Min.Y;
		float ScaleX = (SizeU != 0) ? Data.Size.X / SizeU : 1.0;
		float ScaleY = (SizeV != 0) ? Data.Size.Y / SizeV : 1.0;
		uint32 DefaultColor = FColor::White.DWColor();

		// add vertices
		for (int32 VertIndex = 0; VertIndex < 4; VertIndex++)
		{
			FMaterialMeshVertex* Vert = new(OutVerts) FMaterialMeshVertex();

			int X = VertIndex & 1;
			int Y = (VertIndex >> 1) & 1;

			Vert->Position.Set(ScaleX * X, ScaleY * Y, 0);
			Vert->SetTangents(FVector(1, 0, 0), FVector(0, 1, 0), FVector(0, 0, 1));
			FMemory::Memzero(&Vert->TextureCoordinate, sizeof(Vert->TextureCoordinate));
			Vert->TextureCoordinate[0].Set(U + SizeU * X, V + SizeV * Y);
			Vert->Color = DefaultColor;
		}

		// add indices
		static const int32 Indices[6] = { 0, 2, 1, 2, 3, 1 };
		OutIndices.Append(Indices, 6);

		return 2;
	}

	// Reference: FTileRenderer::DrawTile
	static void RenderMaterial(
		FRHICommandListImmediate& RHICmdList,
		const class FSceneView& View,
		FRenderData& Data)
	{
		FMeshBatch MeshElement;
		MeshElement.VertexFactory = &GMeshVertexFactory;
		MeshElement.DynamicVertexStride = sizeof(FMaterialMeshVertex);
		MeshElement.ReverseCulling = false;
		MeshElement.UseDynamicData = true;
		MeshElement.Type = PT_TriangleList;
		MeshElement.DepthPriorityGroup = SDPG_Foreground;
		FMeshBatchElement& BatchElement = MeshElement.Elements[0];
		BatchElement.PrimitiveUniformBufferResource = &GIdentityPrimitiveUniformBuffer;
#if SHOW_WIREFRAME_MESH
		MeshElement.bWireframe = true;
#endif

		// Check if material is TwoSided - single-sided materials should be rendered with normal and reverse
		// triangle corner orders, to avoid problems with inside-out meshes or mesh parts. Note:
		// FExportMaterialProxy::GetMaterial() (which is really called here) ignores 'InFeatureLevel' parameter.
		const FMaterial* Material = Data.MaterialRenderProxy->GetMaterial(ERHIFeatureLevel::SM5);
		bool bIsMaterialTwoSided = Material->IsTwoSided();

		TArray<FMaterialMeshVertex> Verts;
		TArray<int32> Indices;

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

		MeshElement.UseDynamicData = true;
		MeshElement.DynamicVertexData = Verts.GetData();
		MeshElement.MaterialRenderProxy = Data.MaterialRenderProxy;

		// an attempt to use index data
		BatchElement.FirstIndex = 0;
		BatchElement.NumPrimitives = bIsMaterialTwoSided ? NumTris : NumTris * 2;
		BatchElement.DynamicIndexData = Indices.GetData();
		BatchElement.DynamicIndexStride = sizeof(int32);
		BatchElement.MinVertexIndex = 0;
		BatchElement.MaxVertexIndex = Verts.Num() - 1;

		GetRendererModule().DrawTileMesh(RHICmdList, View, MeshElement, false /*bIsHitTesting*/, FHitProxyId());
	}

	// Reference: FCanvasTileRendererItem::Render_RenderThread
	virtual bool Render_RenderThread(FRHICommandListImmediate& RHICmdList, const FCanvas* Canvas) override
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

		RenderMaterial(RHICmdList, *View, *Data);
	
		delete View;
		if( Canvas->GetAllowedModes() & FCanvas::Allow_DeleteOnRender )
		{
			delete Data;
		}
		if( Canvas->GetAllowedModes() & FCanvas::Allow_DeleteOnRender )
		{
			Data = NULL;
		}
		return true;
	}

	// Reference: FCanvasTileRendererItem::Render_GameThread
	virtual bool Render_GameThread(const FCanvas* Canvas) override
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
		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			DrawMaterialCommand,
			FDrawMaterialParameters, Parameters, DrawMaterialParameters,
		{
			RenderMaterial(RHICmdList, *Parameters.View, *Parameters.RenderData);

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

void PerformImageDilation(TArray<FColor>& InBMP, int32 InImageWidth, int32 InImageHeight, bool IsNormalMap)
{
	int32 PixelIndex = 0;
	int32 PixelIndices[16];

	for (int32 Y = 0; Y < InImageHeight; Y++)
	{
		for (int32 X = 0; X < InImageWidth; X++, PixelIndex++)
		{
			FColor& Color = InBMP[PixelIndex];
			if (Color.A == 0 || (IsNormalMap && Color.B == 0))
			{
				// process this pixel only if it is not filled with color; note: normal map has A==255, so detect
				// empty pixels by zero blue component, which is not possible for normal maps
				int32 NumPixelsToCheck = 0;
				// check line at Y-1
				if (Y > 0)
				{
					PixelIndices[NumPixelsToCheck++] = PixelIndex - InImageWidth; // X,Y-1
					if (X > 0)
					{
						PixelIndices[NumPixelsToCheck++] = PixelIndex - InImageWidth - 1; // X-1,Y-1
					}
					if (X < InImageWidth-1)
					{
						PixelIndices[NumPixelsToCheck++] = PixelIndex - InImageWidth + 1; // X+1,Y-1
					}
				}
				// check line at Y
				if (X > 0)
				{
					PixelIndices[NumPixelsToCheck++] = PixelIndex - 1; // X-1,Y
				}
				if (X < InImageWidth-1)
				{
					PixelIndices[NumPixelsToCheck++] = PixelIndex + 1; // X+1,Y
				}
				// check line at Y+1
				if (Y < InImageHeight-1)
				{
					PixelIndices[NumPixelsToCheck++] = PixelIndex + InImageWidth; // X,Y+1
					if (X > 0)
					{
						PixelIndices[NumPixelsToCheck++] = PixelIndex + InImageWidth - 1; // X-1,Y+1
					}
					if (X < InImageWidth-1)
					{
						PixelIndices[NumPixelsToCheck++] = PixelIndex + InImageWidth + 1; // X+1,Y+1
					}
				}
				// get color
				int32 BestColorValue = 0;
				FColor BestColor(0);
				for (int32 PixelToCheck = 0; PixelToCheck < NumPixelsToCheck; PixelToCheck++)
				{
					const FColor& ColorToCheck = InBMP[PixelIndices[PixelToCheck]];
					if (ColorToCheck.A != 0 && (!IsNormalMap || ColorToCheck.B != 0))
					{
						// consider only original pixels, not computed ones
						int32 ColorValue = ColorToCheck.R + ColorToCheck.G + ColorToCheck.B;
						if (ColorValue > BestColorValue)
						{
							BestColorValue = ColorValue;
							BestColor = ColorToCheck;
						}
					}
				}
				// put the computed pixel back
				if (BestColorValue != 0)
				{
					Color = BestColor;
					Color.A = 0;
#if VISUALIZE_DILATION
					Color.R = 255;
					Color.G = 0;
					Color.B = 0;
#endif
				}
			}
		}
	}
}


bool GRendererInitialized = false;

// Reference: FCanvasTileRendererItem::Render_GameThread
bool RenderMaterial(
	FMaterialRenderProxy* InMaterialProxy,
	EMaterialProperty InMaterialProperty,
	UTextureRenderTarget2D* InRenderTarget,
	const FRawMesh* InMesh,
	const FStaticLODModel* InLODModel,
	int32 InMaterialIndex,
	const FBox2D& InTexcoordBounds,
	const TArray<FVector2D>& InTexCoords,
	TArray<FColor>& OutBMP)
{
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

		FVector2D UV0(InTexcoordBounds.Min.X, InTexcoordBounds.Min.Y);
		FVector2D UV1(InTexcoordBounds.Max.X, InTexcoordBounds.Max.Y);
		FCanvasTileItem TileItem(FVector2D(0.0f, 0.0f), InMaterialProxy, FVector2D(InRenderTarget->SizeX, InRenderTarget->SizeY), UV0, UV1);
		TileItem.bFreezeTime = true;
		Canvas.DrawItem( TileItem );

		Canvas.Flush_GameThread();
#else

		// create ViewFamily
		float CurrentRealTime = 0.f;
		float CurrentWorldTime = 0.f;
		float DeltaWorldTime = 0.f;

/*		if (!bFreezeTime)
		{
			CurrentRealTime = Canvas.GetCurrentRealTime();
			CurrentWorldTime = Canvas.GetCurrentWorldTime();
			DeltaWorldTime = Canvas.GetCurrentDeltaWorldTime();
		} */

		const FRenderTarget* CanvasRenderTarget = Canvas.GetRenderTarget();
		FSceneViewFamily ViewFamily( FSceneViewFamily::ConstructionValues(
			CanvasRenderTarget,
			NULL,
			FEngineShowFlags(ESFIM_Game))
			.SetWorldTimes( CurrentWorldTime, DeltaWorldTime, CurrentRealTime )
			.SetGammaCorrection( CanvasRenderTarget->GetDisplayGamma() ) );

		if (!GRendererInitialized)
		{
			// Initialize the renderer in a case if material LOD computed in UStaticMesh::PostLoad()
			// when loading a scene on UnrealEd startup. Use GetRendererModule().BeginRenderingViewFamily()
			// for that. Prepare a dummy scene because it is required by that function.
			FClassThumbnailScene DummyScene;
			DummyScene.SetClass(UStaticMesh::StaticClass());
			ViewFamily.Scene = DummyScene.GetScene();
			int32 X = 0, Y = 0, Width = 256, Height = 256;
			DummyScene.GetView(&ViewFamily, X, Y, Width, Height);
			GetRendererModule().BeginRenderingViewFamily(&Canvas, &ViewFamily);
			GRendererInitialized = true;
			ViewFamily.Scene = NULL;
		}

#if !SHOW_WIREFRAME_MESH
		Canvas.Clear(InRenderTarget->ClearColor);
#else
		Canvas.Clear(FLinearColor::Yellow);
#endif

		// add item for rendering
		FSimplygonMaterialRenderItem::EnqueueMaterialRender(
			&Canvas,
			&ViewFamily,
			InMesh,
			InLODModel,
			InMaterialIndex,
			InTexcoordBounds,
			InTexCoords,
			FVector2D(InRenderTarget->SizeX, InRenderTarget->SizeY),
			InMaterialProxy
		);

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

	bool result = RTResource->ReadPixels(OutBMP, ReadPixelFlags);
	PerformImageDilation(OutBMP, InRenderTarget->GetSurfaceWidth(), InRenderTarget->GetSurfaceHeight(), bNormalmap);
#ifdef SAVE_INTERMEDIATE_TEXTURES
	FString FilenameString = FString::Printf(
		SAVE_INTERMEDIATE_TEXTURES TEXT("/%s-mat%d-prop%d.bmp"),
		*InMaterialProxy->GetFriendlyName(), InMaterialIndex, (int32)InMaterialProperty);
	FFileHelper::CreateBitmap(*FilenameString, InRenderTarget->GetSurfaceWidth(), InRenderTarget->GetSurfaceHeight(), OutBMP.GetData());
#endif // SAVE_INTERMEDIATE_TEXTURES
	return result;
}

// Reference: MaterialExportUtils::ExportMaterialProperty
bool ExportMaterialProperty(
	UMaterialInterface* InMaterial,
	EMaterialProperty InMaterialProperty,
	UTextureRenderTarget2D* InRenderTarget,
	const FRawMesh* InMesh,
	const FStaticLODModel* InLODModel,
	int32 InMaterialIndex,
	const FBox2D& InTexcoordBounds,
	const TArray<FVector2D>& InTexCoords,
	TArray<FColor>& OutBMP)
{
	TScopedPointer<FMaterialRenderProxy> MaterialProxy(FMaterialUtilities::CreateExportMaterialProxy(InMaterial, InMaterialProperty));
	if (MaterialProxy == NULL)
	{
		return false;
	}
#if 0
	// The code is disabled: WillGenerateUniformData() is a method of FExportMaterialProxy class, which is not available through
	// FMaterialRenderProxy interface. The following code was used to optimize execution time of material flattening, without
	// any effect of resulting material. But for Simplygon it is possible to check resulting bitmap and replace whole image
	// with shader constant block - this will save texture memory when running a game.
	FColor UniformValue;
	if (MaterialProxy->WillGenerateUniformData(UniformValue))
	{
		// Single value... fill it in.
		OutBMP.Empty();
		OutBMP.Add(UniformValue);
		return true;
	}
#endif

	return RenderMaterial(
		MaterialProxy,
		InMaterialProperty,
		InRenderTarget,
		InMesh,
		InLODModel,
		InMaterialIndex,
		InTexcoordBounds,
		InTexCoords,
		OutBMP);
}

// Reference: MaterialExportUtils::RenderMaterialPropertyToTexture
bool RenderMaterialPropertyToTexture(
	UMaterialInterface* InMaterial, 
	EMaterialProperty InMaterialProperty,
	float InTargetGamma,
	bool bInForceLinearGamma,
	EPixelFormat InPixelFormat,
	FIntPoint& InTargetSize,
	const FRawMesh* InMesh,
	const FStaticLODModel* InLODModel,
	int32 InMaterialIndex,
	const FBox2D& InTexcoordBounds,
	const TArray<FVector2D>& InTexCoords,
	TArray<FColor>& OutSamples)
{
	// Create temporary render target
	auto RenderTargetTexture = NewObject<UTextureRenderTarget2D>();
	check(RenderTargetTexture);
	RenderTargetTexture->AddToRoot();
	RenderTargetTexture->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
	RenderTargetTexture->TargetGamma = InTargetGamma;
	RenderTargetTexture->InitCustomFormat(InTargetSize.X, InTargetSize.Y, InPixelFormat, bInForceLinearGamma);

	OutSamples.Empty(InTargetSize.X * InTargetSize.Y);
	bool bResult = ExportMaterialProperty(
		InMaterial,
		InMaterialProperty,
		RenderTargetTexture,
		InMesh,
		InLODModel,
		InMaterialIndex,
		InTexcoordBounds,
		InTexCoords,
		OutSamples);

	// Check for uniform value
#if 0
	bool bIsUniform = true;
	FColor MaxColor(0, 0, 0, 0);
	if (bResult)
	{
		// Find maximal color value
		int32 MaxColorValue = 0;
		for (int32 Index = 0; Index < OutSamples.Num(); Index++)
		{
			FColor Color = OutSamples[Index];
			int32 ColorValue = Color.R + Color.G + Color.B + Color.A;
			if (ColorValue > MaxColorValue)
			{
				MaxColorValue = ColorValue;
				MaxColor = Color;
			}
		}

		// Fill background with maximal color value and render again
		RenderTargetTexture->ClearColor = FLinearColor(MaxColor);
		TArray<FColor> OutSamples2;
		ExportMaterialProperty(
			InMaterial,
			InMaterialProperty,
			RenderTargetTexture,
			InMesh,
			InLODModel,
			InMaterialIndex,
			InTexcoordBounds,
			InTexCoords,
			OutSamples2);
		for (int32 Index = 0; Index < OutSamples2.Num(); Index++)
		{
			FColor Color = OutSamples2[Index];
			if (Color != MaxColor)
			{
				bIsUniform = false;
				break;
			}
		}
	}

	// Uniform value
	if (bIsUniform)
	{
		InTargetSize = FIntPoint(1, 1);
		OutSamples.Empty();
		OutSamples.Add(MaxColor);
	}
#endif

	RenderTargetTexture->RemoveFromRoot();
	RenderTargetTexture = nullptr;

	if (!bResult)
	{
		return false;
	}

	return true;
}

bool ExportMaterial(
	UMaterialInterface* InMaterial,
	const FRawMesh* InMesh,
	const FStaticLODModel* InLODModel,
	int32 InMaterialIndex,
	const FBox2D& InTexcoordBounds,
	const TArray<FVector2D>& InTexCoords,
	FFlattenMaterial& OutFlattenMaterial)
{
	UE_LOG(LogSimplygonUtilities, Log, TEXT("(Simplygon) Flattening material: %s"), *InMaterial->GetName());

	//Check if comandlet rendering is enabled
	if (FApp::CanEverRender())
	{
		UE_LOG(LogSimplygonUtilities, Log, TEXT("(Simplygon) Using commandlet rendering"));
	}
	else
	{
		UE_LOG(LogSimplygonUtilities, Warning, TEXT("(Simplygon) Commandlet rendering is disabled. This will mostlikely break the cooking process."));
	}

	// Precache all used textures
	TArray<UTexture*> MaterialTextures;
	InMaterial->GetUsedTextures(MaterialTextures, EMaterialQualityLevel::Num, true, GMaxRHIFeatureLevel, true);

	for (int32 TexIndex = 0; TexIndex < MaterialTextures.Num(); TexIndex++)
	{
		UTexture* Texture = MaterialTextures[TexIndex];

		if (Texture == NULL)
		{
			continue;
		}

		UTexture2D* Texture2D = Cast<UTexture2D>(Texture);
		if (Texture2D)
		{
			Texture2D->SetForceMipLevelsToBeResident(30.0f, true);
			Texture2D->WaitForStreaming();
		}
	}

	// Render Diffuse property
	bool bRenderDiffuse = OutFlattenMaterial.DiffuseSize.X > 0 &&
		OutFlattenMaterial.DiffuseSize.Y > 0;
	if (bRenderDiffuse)
	{
		RenderMaterialPropertyToTexture(InMaterial, MP_BaseColor, 0.0f, false, PF_B8G8R8A8, 
			OutFlattenMaterial.DiffuseSize, InMesh, InLODModel, InMaterialIndex, InTexcoordBounds, InTexCoords, OutFlattenMaterial.DiffuseSamples);
	}

	// Render metallic property
	bool bRenderMetallic = OutFlattenMaterial.MetallicSize.X > 0 &&
		OutFlattenMaterial.MetallicSize.Y > 0;
	if (bRenderMetallic)
	{
		RenderMaterialPropertyToTexture(InMaterial, MP_Metallic, 0.0f, false, PF_B8G8R8A8,
			OutFlattenMaterial.MetallicSize, InMesh, InLODModel, InMaterialIndex, InTexcoordBounds, InTexCoords, OutFlattenMaterial.MetallicSamples);
	}

	// Render specular property
	bool bRenderSpecular = OutFlattenMaterial.SpecularSize.X > 0 &&
						   OutFlattenMaterial.SpecularSize.Y > 0 ;
	if (bRenderSpecular)
	{
		RenderMaterialPropertyToTexture(InMaterial, MP_Specular, 0.0f, false, PF_B8G8R8A8,
			OutFlattenMaterial.SpecularSize, InMesh, InLODModel, InMaterialIndex, InTexcoordBounds, InTexCoords, OutFlattenMaterial.SpecularSamples);
	}

	// Render roughness property
	bool bRenderRoughness = OutFlattenMaterial.RoughnessSize.X > 0 &&
						    OutFlattenMaterial.RoughnessSize.Y > 0 ;
	if (bRenderRoughness)
	{
		RenderMaterialPropertyToTexture(InMaterial, MP_Roughness, 0.0f, false, PF_B8G8R8A8,
			OutFlattenMaterial.RoughnessSize, InMesh, InLODModel, InMaterialIndex, InTexcoordBounds, InTexCoords, OutFlattenMaterial.RoughnessSamples);
	}

	// Render AO property
	bool bRenderAO = OutFlattenMaterial.AOSize.X > 0 &&
		OutFlattenMaterial.AOSize.Y > 0;
	if (bRenderAO)
	{
		RenderMaterialPropertyToTexture(InMaterial, MP_AmbientOcclusion, 0.0f, false, PF_B8G8R8A8,
			OutFlattenMaterial.AOSize, InMesh, InLODModel, InMaterialIndex, InTexcoordBounds, InTexCoords, OutFlattenMaterial.AOSamples);
	}

	if (OutFlattenMaterial.NormalSize.X > 0 &&
		OutFlattenMaterial.NormalSize.Y > 0 &&
		( InMaterial->GetMaterial()->HasNormalConnected() ||
		(InMaterial->GetMaterial()->bUseMaterialAttributes /*&& InMaterial->GetMaterial()->MaterialAttributes.IsConnected(MP_Normal)*/)))


	{
		RenderMaterialPropertyToTexture(InMaterial, MP_Normal, 0.0f, true, PF_B8G8R8A8,
			OutFlattenMaterial.NormalSize, InMesh, InLODModel, InMaterialIndex, InTexcoordBounds, InTexCoords, OutFlattenMaterial.NormalSamples);
	}
	OutFlattenMaterial.MaterialId = InMaterial->GetLightingGuid();

	UE_LOG(LogSimplygonUtilities, Log, TEXT("(Simplygon) Flattening done. (%s)"), *InMaterial->GetName());
	return true;
}

bool FSimplygonUtilities::ExportMaterial(
	UMaterialInterface* InMaterial,
	FFlattenMaterial& OutFlattenMaterial)
{
	FBox2D DummyBounds(FVector2D(0, 0), FVector2D(1, 1));
	TArray<FVector2D> EmptyTexCoords;

	FullyLoadMaterial(InMaterial);
	::ExportMaterial(
		InMaterial,
		nullptr,
		nullptr,
		0,
		DummyBounds,
		EmptyTexCoords,
		OutFlattenMaterial);
	return true;
}

bool FSimplygonUtilities::ExportMaterial(
	UMaterialInterface* InMaterial,
	const FRawMesh* InMesh,
	int32 InMaterialIndex,
	const FBox2D& InTexcoordBounds,
	const TArray<FVector2D>& InTexCoords,
	FFlattenMaterial& OutFlattenMaterial)
{
	FullyLoadMaterial(InMaterial);
	::ExportMaterial(
		InMaterial,
		InMesh,
		nullptr,
		InMaterialIndex,
		InTexcoordBounds,
		InTexCoords,
		OutFlattenMaterial);
	return true;
}

bool FSimplygonUtilities::ExportMaterial(
	UMaterialInterface* InMaterial,
	const FStaticLODModel* InMesh,
	int32 InMaterialIndex,
	const FBox2D& InTexcoordBounds,
	const TArray<FVector2D>& InTexCoords,
	FFlattenMaterial& OutFlattenMaterial)
{
	FullyLoadMaterial(InMaterial);
	::ExportMaterial(
		InMaterial,
		nullptr,
		InMesh,
		InMaterialIndex,
		InTexcoordBounds,
		InTexCoords,
		OutFlattenMaterial);
	return true;
}

void FSimplygonUtilities::GetTextureCoordinateBoundsForRawMesh(const FRawMesh& InRawMesh, TArray<FBox2D>& OutBounds)
{
	int32 NumWedges = InRawMesh.WedgeIndices.Num();
	int32 NumTris = NumWedges / 3;

	OutBounds.Empty();
	int32 WedgeIndex = 0;
	for (int32 TriIndex = 0; TriIndex < NumTris; TriIndex++)
	{
		int MaterialIndex = InRawMesh.FaceMaterialIndices[TriIndex];
		if (OutBounds.Num() <= MaterialIndex)
			OutBounds.SetNumZeroed(MaterialIndex + 1);
		for (int32 CornerIndex = 0; CornerIndex < 3; CornerIndex++, WedgeIndex++)
		{
			OutBounds[MaterialIndex] += InRawMesh.WedgeTexCoords[0][WedgeIndex];
		}
	}
	return;
}

void FSimplygonUtilities::GetTextureCoordinateBoundsForSkeletalMesh(const FStaticLODModel& LODModel, TArray<FBox2D>& OutBounds)
{
	TArray<FSoftSkinVertex> Vertices;
	FMultiSizeIndexContainerData IndexData;
	LODModel.GetVertices(Vertices);
	LODModel.MultiSizeIndexContainer.GetIndexBufferData(IndexData);

#if WITH_APEX_CLOTHING
	const uint32 SectionCount = (uint32)LODModel.NumNonClothingSections();
#else
	const uint32 SectionCount = LODModel.Sections.Num();
#endif // #if WITH_APEX_CLOTHING
//	const uint32 TexCoordCount = LODModel.NumTexCoords;
//	check( TexCoordCount <= MAX_TEXCOORDS );

	OutBounds.Empty();

	for (uint32 SectionIndex = 0; SectionIndex < SectionCount; ++SectionIndex)
	{
		const FSkelMeshSection& Section = LODModel.Sections[SectionIndex];
		const uint32 FirstIndex = Section.BaseIndex;
		const uint32 LastIndex = FirstIndex + Section.NumTriangles * 3;
		const int32 MaterialIndex = Section.MaterialIndex;

		if (OutBounds.Num() <= MaterialIndex)
			OutBounds.SetNumZeroed(MaterialIndex + 1);

		for (uint32 Index = FirstIndex; Index < LastIndex; ++Index)
		{
			uint32 VertexIndex = IndexData.Indices[Index];
			FSoftSkinVertex& Vertex = Vertices[VertexIndex];

//			for( uint32 TexCoordIndex = 0; TexCoordIndex < TexCoordCount; ++TexCoordIndex )
			{
//				FVector2D TexCoord = Vertex.UVs[TexCoordIndex];
				FVector2D TexCoord = Vertex.UVs[0];
				OutBounds[MaterialIndex] += TexCoord;
			}
		}
	}
}

// Load material and all linked expressions/textures. This function is intended to be used with materials created by SgCreateMaterial() function,
// so material instances, functions and other nested objects are not handled. This function will do nothing useful if material is already loaded.
// It's intended to work when we're baking a new material during engine startup, inside a PostLoad call.
void FSimplygonUtilities::FullyLoadMaterial(UMaterialInterface* Material)
{
	FLinkerLoad* Linker = Material->GetLinker();
	if (Linker == nullptr)
	{
		return;
	}
	// Load material and all expressions.
	Linker->LoadAllObjects(true);
	Material->ConditionalPostLoad();
	// Now load all used textures.
	TArray<UTexture*> Textures;
	Material->GetUsedTextures(Textures, EMaterialQualityLevel::Num, true, ERHIFeatureLevel::Num, true);
	for (int32 i = 0; i < Textures.Num(); ++i)
	{
		UTexture* Texture = Textures[i];
		FLinkerLoad* Linker = Material->GetLinker();
		if (Linker)
		{
			Linker->Preload(Texture);
		}
	}
}

void FSimplygonUtilities::PurgeMaterialTextures(UMaterialInterface* Material)
{
	TArray<UTexture*> Textures;
	Material->GetUsedTextures(Textures, EMaterialQualityLevel::Num, true, ERHIFeatureLevel::Num, true);
	for (int32 i = 0; i < Textures.Num(); ++i)
	{
		UTexture* Texture = Textures[i];
		Texture->MarkPlatformDataTransient();
	}
}

void FSimplygonUtilities::SaveMaterial(UMaterialInterface* Material)
{
	if (!GIsEditor || IsRunningCommandlet() || !Material)
	{
		return;
	}

	if (IsAsyncLoading() || IsLoading())
	{
		// We're most likely inside PostLoad() call, so don't do any saving right now.
		return;
	}

	UPackage* MaterialPackage = Material->GetOutermost();
	if (!MaterialPackage)
	{
		return;
	}

	UE_LOG(LogSimplygonUtilities, Log, TEXT("(Simplygon) Saving material: %s"), *Material->GetName());
	TArray<UPackage*> PackagesToSave;
	TArray<UTexture*> Textures;

	Material->GetUsedTextures(Textures, EMaterialQualityLevel::Num, false, ERHIFeatureLevel::Num, true);
	for (int32 i = 0; i < Textures.Num(); ++i)
	{
		UTexture* texture = Textures[i];
		if (texture)
		{
			PackagesToSave.Add(texture->GetOutermost());
		}
	}

	PackagesToSave.Add(MaterialPackage);
	FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, /*bCheckDirty=*/ false, /*bPromptToSave=*/ false);
	UE_LOG(LogSimplygonUtilities, Log, TEXT("(Simplygon) Saved successfully (%s)"), *Material->GetName());
}


//PRAGMA_ENABLE_OPTIMIZATION

#undef SHOW_WIREFRAME_MESH
#undef VISUALIZE_DILATION
#undef SAVE_INTERMEDIATE_TEXTURES
