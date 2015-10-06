#include "SimplygonUtilitiesPrivatePCH.h"
#include "EngineModule.h"
#include "LocalVertexFactory.h"
#include "MeshBatch.h"
#include "RendererInterface.h"
#include "RawMesh.h"
#include "MaterialExportUtils.h"
#include "MaterialCompiler.h"
#include "ThumbnailHelpers.h" // for FClassThumbnailScene
#include "MaterialUtilities.h"
#include "ShaderCompiler.h"   // for GShaderCompilingManager

#include "SimplygonUtilitiesModule.h"

// NOTE: if this code would fail in cook commandlet, check FApp::CanEverRender() function. It disables rendering
// when commandlet is running. We should disallow IsRunningCommandlet() to return 'true' when rendering a material.
// By the way, UE4/Github has pull request https://github.com/EpicGames/UnrealEngine/pull/797 which allows to do
// the same. This should be integrated into UE4.9 with CL 2523559.


//#define SHOW_WIREFRAME_MESH 1 // should use "reuse UVs" for better visualization effect
//#define VISUALIZE_DILATION 1
//#define SAVE_INTERMEDIATE_TEXTURES		TEXT("C:/TEMP")

// Could use ERHIFeatureLevel::Num, but this will cause assertion when baking material on startup.
#define MATERIAL_FEATURE_LEVEL		ERHIFeatureLevel::SM5

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

TGlobalResource<FMeshVertexFactory> GMeshVertexFactory;

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
		const FMaterial* Material = Data.MaterialRenderProxy->GetMaterial(MATERIAL_FEATURE_LEVEL);
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

struct FMaterialData
{
	/*
	* Input data
	*/
	UMaterialInterface* Material;
	FExportMaterialProxyCache ProxyCache;
	const FRawMesh* Mesh;
	const FStaticLODModel* LODModel;
	int32 MaterialIndex;
	const FBox2D& TexcoordBounds;
	const TArray<FVector2D>& TexCoords;

	/*
	* Output data
	*/
	float EmissiveScale;

	FMaterialData(
		UMaterialInterface* InMaterial,
		const FRawMesh* InMesh,
		const FStaticLODModel* InLODModel,
		int32 InMaterialIndex,
		const FBox2D& InTexcoordBounds,
		const TArray<FVector2D>& InTexCoords)
		: Material(InMaterial)
		, Mesh(InMesh)
		, LODModel(InLODModel)
		, MaterialIndex(InMaterialIndex)
		, TexcoordBounds(InTexcoordBounds)
		, TexCoords(InTexCoords)
		, EmissiveScale(0.0f)
	{}
};

// Reference: FCanvasTileRendererItem::Render_GameThread
bool RenderMaterial(
	FMaterialData& InMaterialData,
	FMaterialRenderProxy* InMaterialProxy,
	EMaterialProperty InMaterialProperty,
	UTextureRenderTarget2D* InRenderTarget,
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

		FVector2D UV0(InMaterialData.TexcoordBounds.Min.X, InMaterialData.TexcoordBounds.Min.Y);
		FVector2D UV1(InMaterialData.TexcoordBounds.Max.X, InMaterialData.TexcoordBounds.Max.Y);
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
			// Force global shaders to be compiled and saved
			if (GShaderCompilingManager)
			{
				// Process any asynchronous shader compile results that are ready, limit execution time
				GShaderCompilingManager->ProcessAsyncResults(false, true);
			}

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
			InMaterialData.Mesh,
			InMaterialData.LODModel,
			InMaterialData.MaterialIndex,
			InMaterialData.TexcoordBounds,
			InMaterialData.TexCoords,
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
		InMaterialData.EmissiveScale = MaxValue;
	}

	PerformImageDilation(OutBMP, InRenderTarget->GetSurfaceWidth(), InRenderTarget->GetSurfaceHeight(), bNormalmap);
#ifdef SAVE_INTERMEDIATE_TEXTURES
	FString FilenameString = FString::Printf(
		SAVE_INTERMEDIATE_TEXTURES TEXT("/%s-mat%d-prop%d.bmp"),
		*InMaterialProxy->GetFriendlyName(), InMaterialData.MaterialIndex, (int32)InMaterialProperty);
	FFileHelper::CreateBitmap(*FilenameString, InRenderTarget->GetSurfaceWidth(), InRenderTarget->GetSurfaceHeight(), OutBMP.GetData());
#endif // SAVE_INTERMEDIATE_TEXTURES
	return result;
}

FMaterialRenderProxy* CreateExportMaterialProxy(FMaterialData& InMaterialData, EMaterialProperty InMaterialProperty)
{
	check(InMaterialProperty >= 0 && InMaterialProperty < ARRAY_COUNT(InMaterialData.ProxyCache.Proxies));
	if (InMaterialData.ProxyCache.Proxies[InMaterialProperty])
	{
		return InMaterialData.ProxyCache.Proxies[InMaterialProperty];
	}
	FMaterialRenderProxy* Proxy = FMaterialUtilities::CreateExportMaterialProxy(InMaterialData.Material, InMaterialProperty);
	InMaterialData.ProxyCache.Proxies[InMaterialProperty] = Proxy;
	return Proxy;
}

// Reference: MaterialExportUtils::RenderMaterialPropertyToTexture, MaterialExportUtils::ExportMaterialProperty
bool FSimplygonUtilities::RenderMaterialPropertyToTexture(
	FMaterialData& InMaterialData,
	bool bInCompileOnly,
	EMaterialProperty InMaterialProperty,
	bool bInForceLinearGamma,
	EPixelFormat InPixelFormat,
	FIntPoint& InTargetSize,
	TArray<FColor>& OutSamples)
{
	if (InTargetSize.X == 0 || InTargetSize.Y == 0)
	{
		return false;
	}

	FMaterialRenderProxy* MaterialProxy = CreateExportMaterialProxy(InMaterialData, InMaterialProperty);
	if (MaterialProxy == nullptr)
	{
		return false;
	}

	if (bInCompileOnly)
	{
		// Rendering will be executed on 2nd pass. Currently we only need to launch a shader compiler for this property.
		return true;
	}

//	FMaterial* Material = const_cast<FMaterial*>(MaterialProxy->GetMaterial(MATERIAL_FEATURE_LEVEL)); - doesn't work, causes assertion
//	Material->FinishCompilation();
	GShaderCompilingManager->FinishAllCompilation();

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

	// Disallow garbage collection of RenderTarget.
	check(CurrentlyRendering == false);
	CurrentlyRendering = true;

	UTextureRenderTarget2D* RenderTarget = CreateRenderTarget(bInForceLinearGamma, InPixelFormat, InTargetSize);

	OutSamples.Empty(InTargetSize.X * InTargetSize.Y);
	bool bResult = RenderMaterial(
		InMaterialData,
		MaterialProxy,
		InMaterialProperty,
		RenderTarget,
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
		//todo: check if 2nd call to ExportMaterialProperty() will recompile shader
		RenderTarget->ClearColor = FLinearColor(MaxColor);
		TArray<FColor> OutSamples2;
		ExportMaterialProperty(
			InMaterialData,
			InMaterialProperty,
			RenderTarget,
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

	CurrentlyRendering = false;


	return bResult;
}

bool FSimplygonUtilities::ExportMaterial(
	FMaterialData& InMaterialData,
	FFlattenMaterial& OutFlattenMaterial,
	FExportMaterialProxyCache* CompiledMaterial)
{
	UMaterialInterface* Material = InMaterialData.Material;
	UE_LOG(LogSimplygonUtilities, Log, TEXT("Flattening material: %s"), *Material->GetName());

	if (CompiledMaterial)
	{
		// ExportMaterial was called with non-null CompiledMaterial. This means compiled shaders
		// should be stored outside, and could be re-used in next call to ExportMaterial.
		// FMaterialData already has "proxy cache" fiels, should swap it with CompiledMaterial,
		// and swap back before returning from this function.
		// Purpose of the following line: use compiled material cached from previous call.
		Exchange(*CompiledMaterial, InMaterialData.ProxyCache);
	}

	// Check if comandlet rendering is enabled
	if (!FApp::CanEverRender())
	{
		UE_LOG(LogSimplygonUtilities, Warning, TEXT("Commandlet rendering is disabled. This will mostlikely break the cooking process."));
	}

	FullyLoadMaterialStatic(Material);

	// Precache all used textures, otherwise could get everything rendered with low-res textures.
	TArray<UTexture*> MaterialTextures;
	Material->GetUsedTextures(MaterialTextures, EMaterialQualityLevel::Num, true, GMaxRHIFeatureLevel, true);

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

	// Render base color property
	bool bRenderNormal = Material->GetMaterial()->HasNormalConnected() || ( Material->GetMaterial()->bUseMaterialAttributes ); // QQ check for material attributes connection
	bool bRenderEmissive = Material->IsPropertyActive(MP_EmissiveColor);

	bool bRenderOpacityMask = Material->IsPropertyActive(MP_OpacityMask) && Material->GetBlendMode() == BLEND_Masked;
	bool bRenderOpacity = Material->IsPropertyActive(MP_Opacity) && IsTranslucentBlendMode(Material->GetBlendMode());
	check(!bRenderOpacity || !bRenderOpacityMask);

	for (int32 Pass = 0; Pass < 2; Pass++)
	{
		// Perform this operation in 2 passes: the first will compile shaders, second will render.
		bool bCompileOnly = Pass == 0;

		// Compile shaders and render flatten material.
		RenderMaterialPropertyToTexture(InMaterialData, bCompileOnly, MP_BaseColor, false, PF_B8G8R8A8, OutFlattenMaterial.DiffuseSize, OutFlattenMaterial.DiffuseSamples);
		RenderMaterialPropertyToTexture(InMaterialData, bCompileOnly, MP_Metallic, false, PF_B8G8R8A8, OutFlattenMaterial.MetallicSize, OutFlattenMaterial.MetallicSamples);
		RenderMaterialPropertyToTexture(InMaterialData, bCompileOnly, MP_Specular, false, PF_B8G8R8A8, OutFlattenMaterial.SpecularSize, OutFlattenMaterial.SpecularSamples);
		RenderMaterialPropertyToTexture(InMaterialData, bCompileOnly, MP_Roughness, false, PF_B8G8R8A8, OutFlattenMaterial.RoughnessSize, OutFlattenMaterial.RoughnessSamples);
		if (bRenderNormal)
		{
			RenderMaterialPropertyToTexture(InMaterialData, bCompileOnly, MP_Normal, true, PF_B8G8R8A8, OutFlattenMaterial.NormalSize, OutFlattenMaterial.NormalSamples);
		}
		if (bRenderOpacityMask)
		{
			RenderMaterialPropertyToTexture(InMaterialData, bCompileOnly, MP_OpacityMask, true, PF_B8G8R8A8, OutFlattenMaterial.OpacitySize, OutFlattenMaterial.OpacitySamples);
		}
		if (bRenderOpacity)
		{
			// Number of blend modes, let's UMaterial decide whether it wants this property
			RenderMaterialPropertyToTexture(InMaterialData, bCompileOnly, MP_Opacity, true, PF_B8G8R8A8, OutFlattenMaterial.OpacitySize, OutFlattenMaterial.OpacitySamples);
		}
		if (bRenderEmissive)
		{
			// PF_FloatRGBA is here to be able to render and read HDR image using ReadFloat16Pixels()
			RenderMaterialPropertyToTexture(InMaterialData, bCompileOnly, MP_EmissiveColor, false, PF_FloatRGBA, OutFlattenMaterial.EmissiveSize, OutFlattenMaterial.EmissiveSamples);
			OutFlattenMaterial.EmissiveScale = InMaterialData.EmissiveScale;
		}
	}

	OutFlattenMaterial.MaterialId = Material->GetLightingGuid();

	if (CompiledMaterial)
	{
		// Store compiled material to external cache.
		Exchange(*CompiledMaterial, InMaterialData.ProxyCache);
	}

	UE_LOG(LogSimplygonUtilities, Log, TEXT("Flattening done. (%s)"), *Material->GetName());
	return true;
}

bool FSimplygonUtilities::ExportMaterial(
	UMaterialInterface* InMaterial,
	FFlattenMaterial& OutFlattenMaterial,
	FExportMaterialProxyCache* CompiledMaterial)
{
	FBox2D DummyBounds(FVector2D(0, 0), FVector2D(1, 1));
	TArray<FVector2D> EmptyTexCoords;

	FMaterialData MaterialData(InMaterial, nullptr, nullptr, 0, DummyBounds, EmptyTexCoords);
	ExportMaterial(MaterialData, OutFlattenMaterial, CompiledMaterial);
	return true;
}

bool FSimplygonUtilities::ExportMaterial(
	UMaterialInterface* InMaterial,
	const FRawMesh* InMesh,
	int32 InMaterialIndex,
	const FBox2D& InTexcoordBounds,
	const TArray<FVector2D>& InTexCoords,
	FFlattenMaterial& OutFlattenMaterial,
	FExportMaterialProxyCache* CompiledMaterial)
{
	FMaterialData MaterialData(InMaterial, InMesh, nullptr, InMaterialIndex, InTexcoordBounds, InTexCoords);
	ExportMaterial(MaterialData, OutFlattenMaterial, CompiledMaterial);
	return true;
}

bool FSimplygonUtilities::ExportMaterial(
	UMaterialInterface* InMaterial,
	const FStaticLODModel* InMesh,
	int32 InMaterialIndex,
	const FBox2D& InTexcoordBounds,
	const TArray<FVector2D>& InTexCoords,
	FFlattenMaterial& OutFlattenMaterial,
	FExportMaterialProxyCache* CompiledMaterial)
{
	FMaterialData MaterialData(InMaterial, nullptr, InMesh, InMaterialIndex, InTexcoordBounds, InTexCoords);
	ExportMaterial(MaterialData, OutFlattenMaterial, CompiledMaterial);
	return true;
}

FFlattenMaterial* FSimplygonUtilities::CreateFlattenMaterial(
	const FSimplygonMaterialLODSettings& InMaterialLODSettings,
	int32 InTextureWidth,
	int32 InTextureHeight)
{
	// Create new material.
	FFlattenMaterial* Material = new FFlattenMaterial();
	// Override non-zero defaults with zeros.
	Material->DiffuseSize = FIntPoint::ZeroValue;
	Material->NormalSize = FIntPoint::ZeroValue;

	// Initialize size of enabled channels. Other channels
	FIntPoint ChannelSize(InTextureWidth, InTextureHeight);
	for (int32 ChannelIndex = 0; ChannelIndex < InMaterialLODSettings.ChannelsToCast.Num(); ChannelIndex++)
	{
		const FSimplygonChannelCastingSettings& Channel = InMaterialLODSettings.ChannelsToCast[ChannelIndex];
		if (Channel.bActive)
		{
			switch (Channel.MaterialChannel)
			{
			case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_BASECOLOR:
				Material->DiffuseSize = ChannelSize;
				break;
			case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_SPECULAR:
				Material->SpecularSize = ChannelSize;
				break;
			case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_ROUGHNESS:
				Material->RoughnessSize = ChannelSize;
				break;
			case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_METALLIC:
				Material->MetallicSize = ChannelSize;
				break;
			case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_NORMALS:
				Material->NormalSize = ChannelSize;
				break;
			case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_OPACITY:
				Material->OpacitySize = ChannelSize;
				break;
			case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_EMISSIVE:
				Material->EmissiveSize = ChannelSize;
				break;
			default:
				UE_LOG(LogSimplygonUtilities, Error, TEXT("Unsupported material channel: %d"), (int32)Channel.MaterialChannel);
			}
		}
	}

	return Material;
}

void FSimplygonUtilities::AnalyzeMaterial(
	UMaterialInterface* InMaterial,
	const FSimplygonMaterialLODSettings& InMaterialLODSettings,
	int32& OutNumTexCoords,
	bool& OutUseVertexColor)
{
	OutUseVertexColor = false;
	OutNumTexCoords = 0;
	for (int32 ChannelIndex = 0; ChannelIndex < InMaterialLODSettings.ChannelsToCast.Num(); ChannelIndex++)
	{
		const FSimplygonChannelCastingSettings& Channel = InMaterialLODSettings.ChannelsToCast[ChannelIndex];
		if (!Channel.bActive)
		{
			continue;
		}
		EMaterialProperty Property;
		switch (Channel.MaterialChannel)
		{
		case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_BASECOLOR:
			Property = MP_BaseColor;
			break;
		case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_SPECULAR:
			Property = MP_Specular;
			break;
		case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_ROUGHNESS:
			Property = MP_Roughness;
			break;
		case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_METALLIC:
			Property = MP_Metallic;
			break;
		case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_NORMALS:
			Property = MP_Normal;
			break;
		case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_OPACITY:
			{
				EBlendMode BlendMode = InMaterial->GetBlendMode();
				if (BlendMode == BLEND_Masked)
				{
					Property = MP_OpacityMask;
				}
				else if (IsTranslucentBlendMode(BlendMode))
				{
					Property = MP_Opacity;
				}
				else
				{
					continue;
				}
			}
			break;
		case ESimplygonMaterialChannel::SG_MATERIAL_CHANNEL_EMISSIVE:
			Property = MP_EmissiveColor;
			break;
		default:
			UE_LOG(LogSimplygonUtilities, Error, TEXT("Unsupported material channel: %d"), (int32)Channel.MaterialChannel);
			continue;
		}
		// Analyze this material channel.
		int32 NumTextureCoordinates = 0;
		bool bUseVertexColor = false;
		InMaterial->AnalyzeMaterialProperty(Property, NumTextureCoordinates, bUseVertexColor);
		// Accumulate data.
		OutNumTexCoords = FMath::Max(NumTextureCoordinates, OutNumTexCoords);
		OutUseVertexColor |= bUseVertexColor;
	}
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

// Load material and all linked expressions/textures. This function will do nothing useful if material is already loaded.
// It's intended to work when we're baking a new material during engine startup, inside a PostLoad call - in this case we
// could have ExportMaterial() call for material which has not all components loaded yet.
void FSimplygonUtilities::FullyLoadMaterialStatic(UMaterialInterface* Material)
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
	Material->GetUsedTextures(Textures, EMaterialQualityLevel::Num, true, MATERIAL_FEATURE_LEVEL, true);
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

void FSimplygonUtilities::FullyLoadMaterial(UMaterialInterface* Material)
{
	FullyLoadMaterialStatic(Material);
}

void FSimplygonUtilities::PurgeMaterialTextures(UMaterialInterface* Material)
{
	TArray<UTexture*> Textures;
	Material->GetUsedTextures(Textures, EMaterialQualityLevel::Num, true, MATERIAL_FEATURE_LEVEL, true);
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

	UE_LOG(LogSimplygonUtilities, Log, TEXT("Saving material: %s"), *Material->GetName());
	TArray<UPackage*> PackagesToSave;
	TArray<UTexture*> Textures;

	Material->GetUsedTextures(Textures, EMaterialQualityLevel::Num, false, MATERIAL_FEATURE_LEVEL, true);
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
	UE_LOG(LogSimplygonUtilities, Log, TEXT("Saved successfully (%s)"), *Material->GetName());
}

UTextureRenderTarget2D* FSimplygonUtilities::CreateRenderTarget(bool bInForceLinearGamma, EPixelFormat InPixelFormat, FIntPoint& InTargetSize)
{
	// Find any pooled render tagret with suitable properties.
	for (int32 RTIndex = 0; RTIndex < RTPool.Num(); RTIndex++)
	{
		UTextureRenderTarget2D* RT = RTPool[RTIndex];
		if (RT->SizeX == InTargetSize.X &&
			RT->SizeY == InTargetSize.Y &&
			RT->OverrideFormat == InPixelFormat &&
			RT->bForceLinearGamma == bInForceLinearGamma)
		{
			return RT;
		}
	}

	// Not found - create a new one.
	UTextureRenderTarget2D* NewRT = NewObject<UTextureRenderTarget2D>();
	check(NewRT);
	NewRT->AddToRoot();
	NewRT->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
	NewRT->TargetGamma = 0.0f;
	NewRT->InitCustomFormat(InTargetSize.X, InTargetSize.Y, InPixelFormat, bInForceLinearGamma);

	RTPool.Add(NewRT);
	return NewRT;
}

void FSimplygonUtilities::ClearRTPool()
{
	if (CurrentlyRendering)
	{
		// Just in case - if garbage collection will happen during rendering, don't allow to GC used render target.
		return;
	}

	// Allow GC'ing of all render targets.
	for (int32 RTIndex = 0; RTIndex < RTPool.Num(); RTIndex++)
	{
		RTPool[RTIndex]->RemoveFromRoot();
	}
	RTPool.Empty();
}

//PRAGMA_ENABLE_OPTIMIZATION

#undef SHOW_WIREFRAME_MESH
#undef VISUALIZE_DILATION
#undef SAVE_INTERMEDIATE_TEXTURES
