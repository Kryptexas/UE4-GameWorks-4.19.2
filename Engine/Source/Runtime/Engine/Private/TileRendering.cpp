// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	TileRendering.cpp: Tile rendering implementation.
=============================================================================*/

#include "RHI.h"
#include "ShowFlags.h"
#include "RenderResource.h"
#include "RenderingThread.h"
#include "VertexFactory.h"
#include "PackedNormal.h"
#include "LocalVertexFactory.h"
#include "SceneView.h"
#include "CanvasTypes.h"
#include "MeshBatch.h"
#include "RendererInterface.h"
#include "SceneUtils.h"
#include "EngineModule.h"
#include "DrawingPolicy.h"

#define NUM_MATERIAL_TILE_VERTS	6

/** 
* Vertex buffer
*/
class FMaterialTileVertexBuffer : public FRenderResource
{
public:
	FVertexBuffer PositionBuffer;
	FVertexBuffer TangentBuffer;
	FVertexBuffer TexCoordBuffer;
	FVertexBuffer ColorBuffer;

	FShaderResourceViewRHIRef TangentBufferSRV;
	FShaderResourceViewRHIRef TexCoordBufferSRV;
	FShaderResourceViewRHIRef ColorBufferSRV;
	FShaderResourceViewRHIRef PositionBufferSRV;

	/** 
	* Initialize the RHI for this rendering resource 
	*/
	virtual void InitRHI() override
	{
		FVector* PositionBufferData = nullptr;
		// used with a trilist, so 6 vertices are needed
		uint32 PositionSize = NUM_MATERIAL_TILE_VERTS * sizeof(FVector);
		// create vertex buffer
		{
			FRHIResourceCreateInfo CreateInfo;
			void* Data = nullptr;
			PositionBuffer.VertexBufferRHI = RHICreateAndLockVertexBuffer(PositionSize, BUF_Static | BUF_ShaderResource, CreateInfo, Data);
			PositionBufferData = static_cast<FVector*>(Data);
			if (RHISupportsManualVertexFetch(GMaxRHIShaderPlatform))
			{
				PositionBufferSRV = RHICreateShaderResourceView(PositionBuffer.VertexBufferRHI, sizeof(float), PF_R32_FLOAT);
			}

		}

		FPackedNormal* TangentBufferData = nullptr;
		// used with a trilist, so 6 vertices are needed
		uint32 TangentSize = NUM_MATERIAL_TILE_VERTS * 2 * sizeof(FPackedNormal);
		// create vertex buffer
		{
			FRHIResourceCreateInfo CreateInfo;
			void* Data = nullptr;
			TangentBuffer.VertexBufferRHI = RHICreateAndLockVertexBuffer(TangentSize, BUF_Static | BUF_ShaderResource, CreateInfo, Data);
			TangentBufferData = static_cast<FPackedNormal*>(Data);
			if (RHISupportsManualVertexFetch(GMaxRHIShaderPlatform))
			{
				TangentBufferSRV = RHICreateShaderResourceView(TangentBuffer.VertexBufferRHI, sizeof(FPackedNormal), PF_R8G8B8A8);
			}
		}

		FVector2D* TexCoordBufferData = nullptr;
		// used with a trilist, so 6 vertices are needed
		uint32 TexCoordSize = NUM_MATERIAL_TILE_VERTS * sizeof(FVector2D);
		// create vertex buffer
		{
			FRHIResourceCreateInfo CreateInfo;
			void* Data = nullptr;
			TexCoordBuffer.VertexBufferRHI = RHICreateAndLockVertexBuffer(TexCoordSize, BUF_Static | BUF_ShaderResource, CreateInfo, Data);
			TexCoordBufferData = static_cast<FVector2D*>(Data);

			if (RHISupportsManualVertexFetch(GMaxRHIShaderPlatform))
			{
				TexCoordBufferSRV = RHICreateShaderResourceView(TexCoordBuffer.VertexBufferRHI, sizeof(FVector2D), PF_G32R32F);
			}
		}

		uint32* ColorBufferData = nullptr;
		// used with a trilist, so 6 vertices are needed
		uint32 ColorSize = NUM_MATERIAL_TILE_VERTS * sizeof(uint32);
		// create vertex buffer
		{
			FRHIResourceCreateInfo CreateInfo;
			void* Data = nullptr;
			ColorBuffer.VertexBufferRHI = RHICreateAndLockVertexBuffer(ColorSize, BUF_Static | BUF_ShaderResource, CreateInfo, Data);
			ColorBufferData = static_cast<uint32*>(Data);
			if (RHISupportsManualVertexFetch(GMaxRHIShaderPlatform))
			{
				ColorBufferSRV = RHICreateShaderResourceView(ColorBuffer.VertexBufferRHI, sizeof(uint32), PF_R8G8B8A8);
			}
		}

		// fill out the verts
		PositionBufferData[0] = FVector( 1, -1, 0);
		PositionBufferData[1] = FVector( 1,  1, 0);
		PositionBufferData[2] = FVector(-1, -1, 0);
		PositionBufferData[3] = FVector(-1, -1, 0);
		PositionBufferData[4] = FVector( 1,  1, 0);	
		PositionBufferData[5] = FVector(-1,  1, 0);

		TexCoordBufferData[0] = FVector2D(1, 1);
		TexCoordBufferData[1] = FVector2D(1, 0);
		TexCoordBufferData[2] = FVector2D(0, 1);
		TexCoordBufferData[3] = FVector2D(0, 1);
		TexCoordBufferData[4] = FVector2D(1, 0);
		TexCoordBufferData[5] = FVector2D(0, 0);

		for (int i = 0; i < NUM_MATERIAL_TILE_VERTS; i++)
		{
			TangentBufferData[2 * i + 0] = FVector(1, 0, 0);
			TangentBufferData[2 * i + 1] = FVector(0, 0, 1);
			ColorBufferData[i] = FColor(255, 255, 255, 255).DWColor();
		}

		// Unlock the buffer.
		RHIUnlockVertexBuffer(PositionBuffer.VertexBufferRHI);
		RHIUnlockVertexBuffer(TangentBuffer.VertexBufferRHI);
		RHIUnlockVertexBuffer(TexCoordBuffer.VertexBufferRHI);
		RHIUnlockVertexBuffer(ColorBuffer.VertexBufferRHI);
	}

	void InitResource() override
	{
		FRenderResource::InitResource();
		PositionBuffer.InitResource();
		TangentBuffer.InitResource();
		TexCoordBuffer.InitResource();
		ColorBuffer.InitResource();
	}

	void ReleaseResource() override
	{
		FRenderResource::ReleaseResource();
		PositionBuffer.ReleaseResource();
		TangentBuffer.ReleaseResource();
		TexCoordBuffer.ReleaseResource();
		ColorBuffer.ReleaseResource();
	}

	virtual void ReleaseRHI() override
	{
		PositionBuffer.ReleaseRHI();
		TangentBuffer.ReleaseRHI();
		TexCoordBuffer.ReleaseRHI();
		ColorBuffer.ReleaseRHI();

		TangentBufferSRV.SafeRelease();
		TexCoordBufferSRV.SafeRelease();
		ColorBufferSRV.SafeRelease();
		PositionBufferSRV.SafeRelease();
	}
};
TGlobalResource<FMaterialTileVertexBuffer> GTileRendererVertexBuffer;


/**
* Vertex factory for rendering tiles.
*/
	/** Default constructor. */
FCanvasTileRendererItem::FTileVertexFactory::FTileVertexFactory(ERHIFeatureLevel::Type InFeatureLevel) : FLocalVertexFactory(InFeatureLevel, "FTileVertexFactory")
{
	
}

/**
 * Mesh used to render tiles.
 */
FCanvasTileRendererItem::FTileMesh::FTileMesh(FCanvasTileRendererItem::FTileVertexFactory* InVertexFactory)
	: VertexFactory(InVertexFactory)
{
}

void FCanvasTileRendererItem::FTileMesh::InitRHI()
{
	FMeshBatchElement& BatchElement = MeshElement.Elements[0];
	MeshElement.VertexFactory = VertexFactory;
	BatchElement.FirstIndex = 0;
	BatchElement.NumPrimitives = 2;
	BatchElement.MinVertexIndex = 0;
	BatchElement.MaxVertexIndex = NUM_MATERIAL_TILE_VERTS - 1;
	MeshElement.ReverseCulling = false;
	MeshElement.Type = PT_TriangleList;
	MeshElement.DepthPriorityGroup = SDPG_Foreground;
	BatchElement.PrimitiveUniformBufferResource = &GIdentityPrimitiveUniformBuffer;
}

void FCanvasTileRendererItem::FTileMesh::ReleaseRHI()
{
	MeshElement.Elements[0].PrimitiveUniformBuffer.SafeRelease();
}

FCanvasTileRendererItem::FRenderData::FRenderData(ERHIFeatureLevel::Type InFeatureLevel,
	const FMaterialRenderProxy* InMaterialRenderProxy,
	const FCanvas::FTransformEntry& InTransform )
	: VertexFactory(InFeatureLevel)
	, TileMesh(&VertexFactory)
	, MaterialRenderProxy(InMaterialRenderProxy)
	, Transform(InTransform)
{

	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(FCanvasTileRendererItemFTileVertexFactoryInit,
		FTileVertexFactory*, TileVertexFactory, &VertexFactory,
		{
			FLocalVertexFactory::FDataType VertexData;
			VertexData.NumTexCoords = 1;

			{
				VertexData.LightMapCoordinateIndex = 0;
				VertexData.TangentsSRV = GTileRendererVertexBuffer.TangentBufferSRV;
				VertexData.TextureCoordinatesSRV = GTileRendererVertexBuffer.TexCoordBufferSRV;
				VertexData.ColorComponentsSRV = GTileRendererVertexBuffer.ColorBufferSRV;
				VertexData.PositionComponentSRV = GTileRendererVertexBuffer.PositionBufferSRV;
			}

			{
				// position
				VertexData.PositionComponent = FVertexStreamComponent(
					&GTileRendererVertexBuffer.PositionBuffer,
					0,
					sizeof(FVector),
					VET_Float3,
					EVertexStreamUsage::Default
				);

				// tangents
				VertexData.TangentBasisComponents[0] = FVertexStreamComponent(&GTileRendererVertexBuffer.TangentBuffer, 0, 2 * sizeof(FPackedNormal), VET_PackedNormal, EVertexStreamUsage::ManualFetch);
				VertexData.TangentBasisComponents[1] = FVertexStreamComponent(&GTileRendererVertexBuffer.TangentBuffer, sizeof(FPackedNormal), 2 * sizeof(FPackedNormal), VET_PackedNormal, EVertexStreamUsage::ManualFetch);

				// color
				VertexData.ColorComponent = FVertexStreamComponent(&GTileRendererVertexBuffer.ColorBuffer, 0, sizeof(FColor), VET_Color, EVertexStreamUsage::ManualFetch);
				// UVs
				VertexData.TextureCoordinates.Add(FVertexStreamComponent(
					&GTileRendererVertexBuffer.TexCoordBuffer,
					0,
					sizeof(FVector2D),
					VET_Float2,
					EVertexStreamUsage::ManualFetch
				));

				// update the data
				TileVertexFactory->SetData(VertexData);
			}
	});
}

FCanvasTileRendererItem::~FCanvasTileRendererItem()
{
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(FCanvasTileRendererItemDeleteRenderData,
		FCanvasTileRendererItem::FRenderData*, RenderData, Data,
		{
			delete RenderData;
		});
	
	Data = nullptr;
}

void FCanvasTileRendererItem::InitTileBuffers(FLocalVertexFactory* VertexFactory, TArray<FTileInst>& Tiles, const FSceneView& View, bool bNeedsToSwitchVerticalAxis)
{
	static_assert(NUM_MATERIAL_TILE_VERTS == 6, "Invalid tile tri-list size.");
	Data->StaticMeshVertexBuffers.PositionVertexBuffer.Init(Tiles.Num() * NUM_MATERIAL_TILE_VERTS);
	Data->StaticMeshVertexBuffers.StaticMeshVertexBuffer.Init(Tiles.Num() * NUM_MATERIAL_TILE_VERTS, 1);
	Data->StaticMeshVertexBuffers.ColorVertexBuffer.Init(Tiles.Num() * NUM_MATERIAL_TILE_VERTS);

	for (int32 i = 0; i < Tiles.Num(); i++)
	{
		const FTileInst& Tile = Tiles[i];

		const float X = Tile.X;
		const float Y = Tile.Y;
		const float U = Tile.U;
		const float V = Tile.V;
		const float SizeX = Tile.SizeX;
		const float SizeY = Tile.SizeY;
		const float SizeU = Tile.SizeU;
		const float SizeV = Tile.SizeV;

		if (bNeedsToSwitchVerticalAxis)
		{
			Data->StaticMeshVertexBuffers.PositionVertexBuffer.VertexPosition(i * NUM_MATERIAL_TILE_VERTS + 0) = FVector(X + SizeX, View.UnscaledViewRect.Height() - (Y + SizeY), 0.0f);
			Data->StaticMeshVertexBuffers.PositionVertexBuffer.VertexPosition(i * NUM_MATERIAL_TILE_VERTS + 1) = FVector(X, View.UnscaledViewRect.Height() - (Y + SizeY), 0.0f);
			Data->StaticMeshVertexBuffers.PositionVertexBuffer.VertexPosition(i * NUM_MATERIAL_TILE_VERTS + 2) = FVector(X + SizeX, View.UnscaledViewRect.Height() - Y, 0.0f);
			Data->StaticMeshVertexBuffers.PositionVertexBuffer.VertexPosition(i * NUM_MATERIAL_TILE_VERTS + 3) = FVector(X + SizeX, View.UnscaledViewRect.Height() - Y, 0.0f);
			Data->StaticMeshVertexBuffers.PositionVertexBuffer.VertexPosition(i * NUM_MATERIAL_TILE_VERTS + 4) = FVector(X, View.UnscaledViewRect.Height() - (Y + SizeY), 0.0f);
			Data->StaticMeshVertexBuffers.PositionVertexBuffer.VertexPosition(i * NUM_MATERIAL_TILE_VERTS + 5) = FVector(X, View.UnscaledViewRect.Height() - Y, 0.0f);

			Data->StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i * NUM_MATERIAL_TILE_VERTS + 0, 0, FVector2D(U + SizeU, V + SizeV));
			Data->StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i * NUM_MATERIAL_TILE_VERTS + 1, 0, FVector2D(U, V + SizeV));
			Data->StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i * NUM_MATERIAL_TILE_VERTS + 2, 0, FVector2D(U + SizeU, V));
			Data->StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i * NUM_MATERIAL_TILE_VERTS + 3, 0, FVector2D(U + SizeU, V));
			Data->StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i * NUM_MATERIAL_TILE_VERTS + 4, 0, FVector2D(U, V + SizeV));
			Data->StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i * NUM_MATERIAL_TILE_VERTS + 5, 0, FVector2D(U, V));
		}
		else
		{
			Data->StaticMeshVertexBuffers.PositionVertexBuffer.VertexPosition(i * NUM_MATERIAL_TILE_VERTS + 0) = FVector(X + SizeX, Y, 0.0f);
			Data->StaticMeshVertexBuffers.PositionVertexBuffer.VertexPosition(i * NUM_MATERIAL_TILE_VERTS + 1) = FVector(X, Y, 0.0f);
			Data->StaticMeshVertexBuffers.PositionVertexBuffer.VertexPosition(i * NUM_MATERIAL_TILE_VERTS + 2) = FVector(X + SizeX, Y + SizeY, 0.0f);
			Data->StaticMeshVertexBuffers.PositionVertexBuffer.VertexPosition(i * NUM_MATERIAL_TILE_VERTS + 3) = FVector(X + SizeX, Y + SizeY, 0.0f);
			Data->StaticMeshVertexBuffers.PositionVertexBuffer.VertexPosition(i * NUM_MATERIAL_TILE_VERTS + 4) = FVector(X, Y, 0.0f);
			Data->StaticMeshVertexBuffers.PositionVertexBuffer.VertexPosition(i * NUM_MATERIAL_TILE_VERTS + 5) = FVector(X, Y + SizeY, 0.0f);

			Data->StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i * NUM_MATERIAL_TILE_VERTS + 0, 0, FVector2D(U + SizeU, V));
			Data->StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i * NUM_MATERIAL_TILE_VERTS + 1, 0, FVector2D(U, V));
			Data->StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i * NUM_MATERIAL_TILE_VERTS + 2, 0, FVector2D(U + SizeU, V + SizeV));
			Data->StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i * NUM_MATERIAL_TILE_VERTS + 3, 0, FVector2D(U + SizeU, V + SizeV));
			Data->StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i * NUM_MATERIAL_TILE_VERTS + 4, 0, FVector2D(U, V));
			Data->StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i * NUM_MATERIAL_TILE_VERTS + 5, 0, FVector2D(U, V + SizeV));
		}

		for (int j = 0; j < NUM_MATERIAL_TILE_VERTS; j++)
		{
			Data->StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(i * NUM_MATERIAL_TILE_VERTS + j, FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f), FVector(0.0f, 0.0f, 1.0f));
			Data->StaticMeshVertexBuffers.ColorVertexBuffer.VertexColor(i * NUM_MATERIAL_TILE_VERTS + j) = Tile.InColor;
		}
	}

	FCanvasTileRendererItem::FRenderData* RenderData = Data;
	ENQUEUE_RENDER_COMMAND(CreateTileVerticesBuffersLegacyInit)(
		[VertexFactory, RenderData](FRHICommandListImmediate& RHICmdList)
	{
		RenderData->StaticMeshVertexBuffers.PositionVertexBuffer.InitResource();
		RenderData->StaticMeshVertexBuffers.StaticMeshVertexBuffer.InitResource();
		RenderData->StaticMeshVertexBuffers.ColorVertexBuffer.InitResource();

		FLocalVertexFactory::FDataType RData;
		RenderData->StaticMeshVertexBuffers.PositionVertexBuffer.BindPositionVertexBuffer(VertexFactory, RData);
		RenderData->StaticMeshVertexBuffers.StaticMeshVertexBuffer.BindTangentVertexBuffer(VertexFactory, RData);
		RenderData->StaticMeshVertexBuffers.StaticMeshVertexBuffer.BindPackedTexCoordVertexBuffer(VertexFactory, RData);
		RenderData->StaticMeshVertexBuffers.StaticMeshVertexBuffer.BindLightMapVertexBuffer(VertexFactory, RData, 0);
		RenderData->StaticMeshVertexBuffers.ColorVertexBuffer.BindColorVertexBuffer(VertexFactory, RData);
		VertexFactory->SetData(RData);

		RenderData->VertexFactory.InitResource();
		RenderData->TileMesh.InitResource();
	});
}

bool FCanvasTileRendererItem::Render_RenderThread(FRHICommandListImmediate& RHICmdList, FDrawingPolicyRenderState& DrawRenderState, const FCanvas* Canvas)
{
	float CurrentRealTime = 0.f;
	float CurrentWorldTime = 0.f;
	float DeltaWorldTime = 0.f;

	if (!bFreezeTime)
	{
		CurrentRealTime = Canvas->GetCurrentRealTime();
		CurrentWorldTime = Canvas->GetCurrentWorldTime();
		DeltaWorldTime = Canvas->GetCurrentDeltaWorldTime();
	}

	checkSlow(Data);
	// current render target set for the canvas
	const FRenderTarget* CanvasRenderTarget = Canvas->GetRenderTarget();
	FSceneViewFamily* ViewFamily = new FSceneViewFamily(FSceneViewFamily::ConstructionValues(
		CanvasRenderTarget,
		nullptr,
		FEngineShowFlags(ESFIM_Game))
		.SetWorldTimes(CurrentWorldTime, DeltaWorldTime, CurrentRealTime)
		.SetGammaCorrection(CanvasRenderTarget->GetDisplayGamma()));

	FIntRect ViewRect(FIntPoint(0, 0), CanvasRenderTarget->GetSizeXY());

	// make a temporary view
	FSceneViewInitOptions ViewInitOptions;
	ViewInitOptions.ViewFamily = ViewFamily;
	ViewInitOptions.SetViewRectangle(ViewRect);
	ViewInitOptions.ViewOrigin = FVector::ZeroVector;
	ViewInitOptions.ViewRotationMatrix = FMatrix::Identity;
	ViewInitOptions.ProjectionMatrix = Data->Transform.GetMatrix();
	ViewInitOptions.BackgroundColor = FLinearColor::Black;
	ViewInitOptions.OverlayColor = FLinearColor::White;

	FSceneView* View = new FSceneView(ViewInitOptions);
	
	bool bNeedsToSwitchVerticalAxis = RHINeedsToSwitchVerticalAxis(Canvas->GetShaderPlatform()) && IsMobileHDR();

	Data->TileMesh.InitResource();
	InitTileBuffers(&Data->VertexFactory, Data->Tiles, *View, bNeedsToSwitchVerticalAxis);

	for (int32 TileIdx = 0; TileIdx < Data->Tiles.Num(); TileIdx++)
	{
		FRenderData::FTileInst& Tile = Data->Tiles[TileIdx];

		// update the FMeshBatch
		FMeshBatch& Mesh = Data->TileMesh.MeshElement;
		Mesh.MaterialRenderProxy = Data->MaterialRenderProxy;
		Mesh.Elements[0].BaseVertexIndex = NUM_MATERIAL_TILE_VERTS * TileIdx;

		GetRendererModule().DrawTileMesh(RHICmdList, DrawRenderState, *View, Mesh, Canvas->IsHitTesting(), Tile.HitProxyId);
	}

	Data->StaticMeshVertexBuffers.PositionVertexBuffer.ReleaseResource();
	Data->StaticMeshVertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
	Data->StaticMeshVertexBuffers.ColorVertexBuffer.ReleaseResource();
	Data->TileMesh.ReleaseResource();
	Data->VertexFactory.ReleaseResource();

	delete View->Family;
	delete View;
	if (Canvas->GetAllowedModes() & FCanvas::Allow_DeleteOnRender)
	{
		delete Data;
		Data = NULL;
	}
	return true;
}

bool FCanvasTileRendererItem::Render_GameThread(const FCanvas* Canvas)
{
	float CurrentRealTime = 0.f;
	float CurrentWorldTime = 0.f;
	float DeltaWorldTime = 0.f;

	if (!bFreezeTime)
	{
		CurrentRealTime = Canvas->GetCurrentRealTime();
		CurrentWorldTime = Canvas->GetCurrentWorldTime();
		DeltaWorldTime = Canvas->GetCurrentDeltaWorldTime();
	}

	checkSlow(Data);
	// current render target set for the canvas
	const FRenderTarget* CanvasRenderTarget = Canvas->GetRenderTarget();
	FSceneViewFamily* ViewFamily = new FSceneViewFamily(FSceneViewFamily::ConstructionValues(
		CanvasRenderTarget,
		Canvas->GetScene(),
		FEngineShowFlags(ESFIM_Game))
		.SetWorldTimes(CurrentWorldTime, DeltaWorldTime, CurrentRealTime)
		.SetGammaCorrection(CanvasRenderTarget->GetDisplayGamma()));

	FIntRect ViewRect(FIntPoint(0, 0), CanvasRenderTarget->GetSizeXY());

	// make a temporary view
	FSceneViewInitOptions ViewInitOptions;
	ViewInitOptions.ViewFamily = ViewFamily;
	ViewInitOptions.SetViewRectangle(ViewRect);
	ViewInitOptions.ViewOrigin = FVector::ZeroVector;
	ViewInitOptions.ViewRotationMatrix = FMatrix::Identity;
	ViewInitOptions.ProjectionMatrix = Data->Transform.GetMatrix();
	ViewInitOptions.BackgroundColor = FLinearColor::Black;
	ViewInitOptions.OverlayColor = FLinearColor::White;

	FSceneView* View = new FSceneView(ViewInitOptions);

	bool bNeedsToSwitchVerticalAxis = RHINeedsToSwitchVerticalAxis(Canvas->GetShaderPlatform()) && IsMobileHDR();

	struct FDrawTileParameters
	{
		FSceneView* View;
		FRenderData* RenderData;
		uint32 bIsHitTesting : 1;
		uint32 AllowedCanvasModes;
	};
	FDrawTileParameters DrawTileParameters =
	{
		View,
		Data,
		Canvas->IsHitTesting() ? uint32(1) : uint32(0),
		Canvas->GetAllowedModes()
	};

	InitTileBuffers(&Data->VertexFactory, Data->Tiles, *View, bNeedsToSwitchVerticalAxis);

	ENQUEUE_RENDER_COMMAND(DrawTileCommand)(
		[DrawTileParameters](FRHICommandListImmediate& RHICmdList)
		{
			SCOPED_DRAW_EVENTF(RHICmdList, CanvasDrawTile, *DrawTileParameters.RenderData->MaterialRenderProxy->GetMaterial(GMaxRHIFeatureLevel)->GetFriendlyName());

			FDrawingPolicyRenderState DrawRenderState(*DrawTileParameters.View);

			// disable depth test & writes
			DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());

			for (int32 TileIdx = 0; TileIdx < DrawTileParameters.RenderData->Tiles.Num(); TileIdx++)
			{
				FRenderData::FTileInst& Tile = DrawTileParameters.RenderData->Tiles[TileIdx];

				// update the FMeshBatch
				FMeshBatch& Mesh = DrawTileParameters.RenderData->TileMesh.MeshElement;
				Mesh.MaterialRenderProxy = DrawTileParameters.RenderData->MaterialRenderProxy;
				Mesh.Elements[0].BaseVertexIndex = NUM_MATERIAL_TILE_VERTS * TileIdx;

				GetRendererModule().DrawTileMesh(RHICmdList, DrawRenderState, *DrawTileParameters.View, Mesh, DrawTileParameters.bIsHitTesting, Tile.HitProxyId);
			}

			DrawTileParameters.RenderData->StaticMeshVertexBuffers.PositionVertexBuffer.ReleaseResource();
			DrawTileParameters.RenderData->StaticMeshVertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
			DrawTileParameters.RenderData->StaticMeshVertexBuffers.ColorVertexBuffer.ReleaseResource();
			DrawTileParameters.RenderData->TileMesh.ReleaseResource();
			DrawTileParameters.RenderData->VertexFactory.ReleaseResource();

			delete DrawTileParameters.View->Family;
			delete DrawTileParameters.View;
			if (DrawTileParameters.AllowedCanvasModes & FCanvas::Allow_DeleteOnRender)
			{
				delete DrawTileParameters.RenderData;
			}
		});

	if (Canvas->GetAllowedModes() & FCanvas::Allow_DeleteOnRender)
	{
		Data = NULL;
	}
	return true;
}
