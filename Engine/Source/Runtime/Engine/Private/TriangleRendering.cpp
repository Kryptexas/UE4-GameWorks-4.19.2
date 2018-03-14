// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	TriangleRendering.cpp: Simple triangle rendering implementation.
=============================================================================*/

#include "ShowFlags.h"
#include "RHI.h"
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

/** 
* Vertex buffer
*/
class FMaterialTriangleVertexBuffer : public FVertexBuffer
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
		uint32 PositionSize = 3 * sizeof(FVector);
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
		uint32 TangentSize = 3 * 2 * sizeof(FPackedNormal);
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
		uint32 TexCoordSize = 3 * sizeof(FVector2D);
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
		uint32 ColorSize = 3 * sizeof(uint32);
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
		PositionBufferData[0] = FVector(1, -1, 0);
		PositionBufferData[1] = FVector(1, 1, 0);
		PositionBufferData[2] = FVector(-1, -1, 0);

		TexCoordBufferData[0] = FVector2D(1, 1);
		TexCoordBufferData[1] = FVector2D(1, 0);
		TexCoordBufferData[2] = FVector2D(0, 1);

		for (int i = 0; i < 3; i++)
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
TGlobalResource<FMaterialTriangleVertexBuffer> GTriangleRendererVertexBuffer;

/**
* Vertex factory for rendering tiles.
*/
FCanvasTriangleRendererItem::FTriangleVertexFactory::FTriangleVertexFactory(ERHIFeatureLevel::Type InFeatureLevel)
	: FLocalVertexFactory(InFeatureLevel, "FTriangleVertexFactory")
{
	FLocalVertexFactory::FDataType VertexData;
	VertexData.NumTexCoords = 1;

	{
		VertexData.LightMapCoordinateIndex = 0;
		VertexData.TangentsSRV = GTriangleRendererVertexBuffer.TangentBufferSRV;
		VertexData.TextureCoordinatesSRV = GTriangleRendererVertexBuffer.TexCoordBufferSRV;
		VertexData.ColorComponentsSRV = GTriangleRendererVertexBuffer.ColorBufferSRV;
		VertexData.PositionComponentSRV = GTriangleRendererVertexBuffer.PositionBufferSRV;
	}
		
	{
		// position
		Data.PositionComponent = FVertexStreamComponent(
			&GTriangleRendererVertexBuffer.PositionBuffer,
			0,
			sizeof(FVector),
			VET_Float3,
			EVertexStreamUsage::Default
		);

		// tangents
		VertexData.TangentBasisComponents[0] = FVertexStreamComponent(&GTriangleRendererVertexBuffer.TangentBuffer, 0, 2 * sizeof(FPackedNormal), VET_PackedNormal, EVertexStreamUsage::ManualFetch);
		VertexData.TangentBasisComponents[1] = FVertexStreamComponent(&GTriangleRendererVertexBuffer.TangentBuffer, sizeof(FPackedNormal), 2 * sizeof(FPackedNormal), VET_PackedNormal, EVertexStreamUsage::ManualFetch);

		// color
		VertexData.ColorComponent = FVertexStreamComponent(&GTriangleRendererVertexBuffer.ColorBuffer, 0, sizeof(FColor), VET_Color, EVertexStreamUsage::ManualFetch);
		// UVs
		VertexData.TextureCoordinates.Add(FVertexStreamComponent(
			&GTriangleRendererVertexBuffer.TexCoordBuffer,
			0,
			sizeof(FVector2D),
			VET_Float2,
			EVertexStreamUsage::ManualFetch
		));

		FCanvasTriangleRendererItem::FTriangleVertexFactory* Self = this;
		ENQUEUE_RENDER_COMMAND(FCanvasTriangleRendererItemInit)(
			[Self, VertexData](FRHICommandListImmediate& RHICmdList)
		{
			// update the data
			Self->SetData(VertexData);
		});
	}
}

/**
 * Mesh used to render triangles.
 */
FCanvasTriangleRendererItem::FTriangleMesh::FTriangleMesh(FTriangleVertexFactory* InVertexFactory)
	: VertexFactory(InVertexFactory)
{

}

/** The mesh element. */
FMeshBatch TriMeshElement;

void FCanvasTriangleRendererItem::FTriangleMesh::InitRHI()
{
	FMeshBatchElement& BatchElement = TriMeshElement.Elements[0];
	TriMeshElement.VertexFactory = VertexFactory;
	BatchElement.FirstIndex = 0;
	BatchElement.NumPrimitives = 1;
	BatchElement.MinVertexIndex = 0;
	BatchElement.MaxVertexIndex = 2;
	TriMeshElement.ReverseCulling = false;
	TriMeshElement.bDisableBackfaceCulling = true;
	TriMeshElement.Type = PT_TriangleList;
	TriMeshElement.DepthPriorityGroup = SDPG_Foreground;
	BatchElement.PrimitiveUniformBufferResource = &GIdentityPrimitiveUniformBuffer;
}
	
void FCanvasTriangleRendererItem::FTriangleMesh::ReleaseRHI()
{
	TriMeshElement.Elements[0].PrimitiveUniformBuffer.SafeRelease();
}

void FCanvasTriangleRendererItem::InitTriangleBuffers(FLocalVertexFactory* VertexFactory, TArray<FTriangleInst>& Triangles, const FSceneView& View, bool bNeedsToSwitchVerticalAxis)
{
	Data->StaticMeshVertexBuffers.PositionVertexBuffer.Init(Triangles.Num() * 3);
	Data->StaticMeshVertexBuffers.StaticMeshVertexBuffer.Init(Triangles.Num() * 3, 1);
	Data->StaticMeshVertexBuffers.ColorVertexBuffer.Init(Triangles.Num() * 3);

	for (int32 i = 0; i < Triangles.Num(); i++)
	{
		const FCanvasUVTri& Tri = Triangles[i].Tri;

		// create verts. Notice the order is (1, 0, 2)
		if (bNeedsToSwitchVerticalAxis)
		{
			Data->StaticMeshVertexBuffers.PositionVertexBuffer.VertexPosition(i * 3 + 0) = FVector(Tri.V1_Pos.X, View.UnscaledViewRect.Height() - Tri.V1_Pos.Y, 0.0f);
			Data->StaticMeshVertexBuffers.PositionVertexBuffer.VertexPosition(i * 3 + 1) = FVector(Tri.V0_Pos.X, View.UnscaledViewRect.Height() - Tri.V0_Pos.Y, 0.0f);
			Data->StaticMeshVertexBuffers.PositionVertexBuffer.VertexPosition(i * 3 + 2) = FVector(Tri.V2_Pos.X, View.UnscaledViewRect.Height() - Tri.V2_Pos.Y, 0.0f);
		}
		else
		{
			Data->StaticMeshVertexBuffers.PositionVertexBuffer.VertexPosition(i * 3 + 0) = FVector(Tri.V1_Pos.X, Tri.V1_Pos.Y, 0.0f);
			Data->StaticMeshVertexBuffers.PositionVertexBuffer.VertexPosition(i * 3 + 1) = FVector(Tri.V0_Pos.X, Tri.V0_Pos.Y, 0.0f);
			Data->StaticMeshVertexBuffers.PositionVertexBuffer.VertexPosition(i * 3 + 2) = FVector(Tri.V2_Pos.X, Tri.V2_Pos.Y, 0.0f);
		}

		Data->StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(i * 3 + 0, FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f), FVector(0.0f, 0.0f, 1.0f));
		Data->StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(i * 3 + 1, FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f), FVector(0.0f, 0.0f, 1.0f));
		Data->StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(i * 3 + 2, FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f), FVector(0.0f, 0.0f, 1.0f));

		Data->StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i * 3 + 0, 0, FVector2D(Tri.V1_UV.X, Tri.V1_UV.Y));
		Data->StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i * 3 + 1, 0, FVector2D(Tri.V0_UV.X, Tri.V0_UV.Y));
		Data->StaticMeshVertexBuffers.StaticMeshVertexBuffer.SetVertexUV(i * 3 + 2, 0, FVector2D(Tri.V2_UV.X, Tri.V2_UV.Y));

		Data->StaticMeshVertexBuffers.ColorVertexBuffer.VertexColor(i * 3 + 0) = Tri.V1_Color.ToFColor(true);
		Data->StaticMeshVertexBuffers.ColorVertexBuffer.VertexColor(i * 3 + 1) = Tri.V0_Color.ToFColor(true);
		Data->StaticMeshVertexBuffers.ColorVertexBuffer.VertexColor(i * 3 + 2) = Tri.V2_Color.ToFColor(true);
	}

	FCanvasTriangleRendererItem::FRenderData* RenderData = Data;
	ENQUEUE_RENDER_COMMAND(StaticMeshVertexBuffersLegacyInit)(
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
		RenderData->TriMesh.InitResource();
	});
};

bool FCanvasTriangleRendererItem::Render_RenderThread(FRHICommandListImmediate& RHICmdList, FDrawingPolicyRenderState& DrawRenderState, const FCanvas* Canvas)
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

	bool bNeedsToSwitchVerticalAxis = RHINeedsToSwitchVerticalAxis(Canvas->GetShaderPlatform()) && XOR(IsMobileHDR(), Canvas->GetAllowSwitchVerticalAxis());
	FSceneView* View = new FSceneView(ViewInitOptions);

	InitTriangleBuffers(&Data->VertexFactory, Data->Triangles, *View, bNeedsToSwitchVerticalAxis);

	for (int32 TriIdx = 0; TriIdx < Data->Triangles.Num(); TriIdx++)
	{
		const FRenderData::FTriangleInst& Tri = Data->Triangles[TriIdx];

		// update the FMeshBatch
		FMeshBatch& TriMesh = Data->TriMesh.TriMeshElement;
		TriMesh.VertexFactory = &Data->VertexFactory;
		TriMesh.MaterialRenderProxy = Data->MaterialRenderProxy;
		TriMesh.Elements[0].BaseVertexIndex = 3 * TriIdx;

		GetRendererModule().DrawTileMesh(RHICmdList, DrawRenderState, *View, TriMesh, Canvas->IsHitTesting(), Tri.HitProxyId);
	}

	Data->StaticMeshVertexBuffers.PositionVertexBuffer.ReleaseResource();
	Data->StaticMeshVertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
	Data->StaticMeshVertexBuffers.ColorVertexBuffer.ReleaseResource();
	Data->TriMesh.ReleaseResource();
	Data->VertexFactory.ReleaseResource();

	delete View->Family;
	delete View;
	if (Canvas->GetAllowedModes() & FCanvas::Allow_DeleteOnRender)
	{
		delete Data;
		Data = nullptr;
	}
	return true;
}

bool FCanvasTriangleRendererItem::Render_GameThread(const FCanvas* Canvas)
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

	bool bNeedsToSwitchVerticalAxis = RHINeedsToSwitchVerticalAxis(Canvas->GetShaderPlatform()) && XOR(IsMobileHDR(),Canvas->GetAllowSwitchVerticalAxis());

	struct FDrawTriangleParameters
	{
		FSceneView* View;
		FRenderData* RenderData;
		uint32 bIsHitTesting : 1;
		uint32 AllowedCanvasModes;
	};
	FDrawTriangleParameters DrawTriangleParameters =
	{
		View,
		Data,
		(uint32)Canvas->IsHitTesting(),
		Canvas->GetAllowedModes()
	};

	InitTriangleBuffers(&Data->VertexFactory, Data->Triangles, *View, bNeedsToSwitchVerticalAxis);

	FDrawTriangleParameters Parameters = DrawTriangleParameters;
	ENQUEUE_RENDER_COMMAND(DrawTriangleCommand)(
		[Parameters](FRHICommandListImmediate& RHICmdList)	
		{
			FDrawingPolicyRenderState DrawRenderState(*Parameters.View);

			// disable depth test & writes
			DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());

			SCOPED_DRAW_EVENT(RHICmdList, CanvasDrawTriangle);
			for (int32 TriIdx = 0; TriIdx < Parameters.RenderData->Triangles.Num(); TriIdx++)
			{
				const FRenderData::FTriangleInst& Tri = Parameters.RenderData->Triangles[TriIdx];
				// update the FMeshBatch
				FMeshBatch& TriMesh = Parameters.RenderData->TriMesh.TriMeshElement;
				TriMesh.VertexFactory = &Parameters.RenderData->VertexFactory;
				TriMesh.MaterialRenderProxy = Parameters.RenderData->MaterialRenderProxy;
				TriMesh.Elements[0].BaseVertexIndex = 3 * TriIdx;

				GetRendererModule().DrawTileMesh(RHICmdList, DrawRenderState, *Parameters.View, TriMesh, Parameters.bIsHitTesting, Tri.HitProxyId);
			}

			Parameters.RenderData->StaticMeshVertexBuffers.PositionVertexBuffer.ReleaseResource();
			Parameters.RenderData->StaticMeshVertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
			Parameters.RenderData->StaticMeshVertexBuffers.ColorVertexBuffer.ReleaseResource();
			Parameters.RenderData->TriMesh.ReleaseResource();
			Parameters.RenderData->VertexFactory.ReleaseResource();

			delete Parameters.View->Family;
			delete Parameters.View;
			if (Parameters.AllowedCanvasModes & FCanvas::Allow_DeleteOnRender)
			{
				delete Parameters.RenderData;
			}
		});

	if (Canvas->GetAllowedModes() & FCanvas::Allow_DeleteOnRender)
	{
		Data = nullptr;
	}
	return true;
}
