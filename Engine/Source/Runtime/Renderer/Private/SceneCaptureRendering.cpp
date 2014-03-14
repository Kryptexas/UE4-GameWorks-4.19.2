// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"
#include "UniformBuffer.h"
#include "ShaderParameters.h"
#include "ScreenRendering.h"
#include "EngineDecalClasses.h"
#include "PostProcessAmbient.h"
#include "PostProcessing.h"

void UpdateSceneCaptureContent_RenderThread(FSceneRenderer* SceneRenderer, FTextureRenderTargetResource* TextureRenderTarget, const FName OwnerName, const FResolveParams& ResolveParams, bool bUseSceneColorTexture)
{
	FMemMark MemStackMark(FMemStack::Get());

	// update any resources that needed a deferred update
	FDeferredUpdateResource::UpdateResources();

	{
#if WANTS_DRAW_MESH_EVENTS
		FString EventName;
		OwnerName.ToString(EventName);
		SCOPED_DRAW_EVENTF(SceneCapture, DEC_SCENE_ITEMS, TEXT("SceneCapture %s"), *EventName);
#endif

		// Render the scene normally
		const FRenderTarget* Target = SceneRenderer->ViewFamily.RenderTarget;
		FIntRect ViewRect = SceneRenderer->Views[0].ViewRect;
		FIntRect UnconstrainedViewRect = SceneRenderer->Views[0].UnconstrainedViewRect;
		RHISetRenderTarget(Target->GetRenderTargetTexture(), NULL);
		RHIClear(true, FLinearColor::Black, false, 1.0f, false, 0, ViewRect);
		SceneRenderer->Render();

		// Copy the captured scene into the destination texture
		if (bUseSceneColorTexture)
		{
			// Copy the captured scene into the destination texture
			RHISetRenderTarget(Target->GetRenderTargetTexture(), NULL);

			RHISetRasterizerState(TStaticRasterizerState<FM_Solid, CM_None>::GetRHI());
			RHISetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
			RHISetBlendState(TStaticBlendState<>::GetRHI());

			TShaderMapRef<FScreenVS> VertexShader(GetGlobalShaderMap());
			TShaderMapRef<FScreenPS> PixelShader(GetGlobalShaderMap());
			static FGlobalBoundShaderState BoundShaderState;
			SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

			FRenderingCompositePassContext Context(SceneRenderer->Views[0]);

			VertexShader->SetParameters(SceneRenderer->Views[0]);
			PixelShader->SetParameters(TStaticSamplerState<SF_Point>::GetRHI(), GSceneRenderTargets.GetSceneColorTexture());

			FIntPoint TargetSize(UnconstrainedViewRect.Width(), UnconstrainedViewRect.Height());

			DrawRectangle(
				ViewRect.Min.X, ViewRect.Min.Y,
				ViewRect.Width(), ViewRect.Height(),
				ViewRect.Min.X, ViewRect.Min.Y,
				ViewRect.Width(), ViewRect.Height(),
				TargetSize,
				GSceneRenderTargets.GetBufferSizeXY(),
				EDRF_UseTriangleOptimization);
		}

		RHICopyToResolveTarget(TextureRenderTarget->GetRenderTargetTexture(), TextureRenderTarget->TextureRHI, false, ResolveParams);
	}

	delete SceneRenderer;
}


FSceneRenderer* FScene::CreateSceneRenderer( UTextureRenderTarget* TextureTarget, const FMatrix& ViewMatrix, const FVector& ViewLocation, float FOV, float MaxViewDistance, bool bCaptureSceneColour, FPostProcessSettings* PostProcessSettings, float PostProcessBlendWeight )
{
	FIntPoint CaptureSize(TextureTarget->GetSurfaceWidth(), TextureTarget->GetSurfaceHeight());

	FTextureRenderTargetResource* Resource = TextureTarget->GameThread_GetRenderTargetResource();
	FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(
		Resource,
		this,
		FEngineShowFlags(ESFIM_Game))
		.SetResolveScene(!bCaptureSceneColour));

	// Disable features that are not desired when capturing the scene
	ViewFamily.EngineShowFlags.MotionBlur = 0; // motion blur doesn't work correctly with scene captures.
	ViewFamily.EngineShowFlags.SeparateTranslucency = 0;
	ViewFamily.EngineShowFlags.HMDDistortion = 0;
	FSceneViewInitOptions ViewInitOptions;
	ViewInitOptions.SetViewRectangle(FIntRect(0, 0, CaptureSize.X, CaptureSize.Y));
	ViewInitOptions.ViewFamily = &ViewFamily;
	ViewInitOptions.ViewMatrix = ViewMatrix;
	ViewInitOptions.BackgroundColor = FLinearColor::Black;
	ViewInitOptions.OverrideFarClippingPlaneDistance = MaxViewDistance;

	if (bCaptureSceneColour)
	{
		ViewFamily.EngineShowFlags.PostProcessing = 0;
		ViewInitOptions.OverlayColor = FLinearColor::Black;
	}

	// Build projection matrix
	{
		float XAxisMultiplier;
		float YAxisMultiplier;

		if (CaptureSize.X > CaptureSize.Y)
		{
			// if the viewport is wider than it is tall
			XAxisMultiplier = 1.0f;
			YAxisMultiplier = CaptureSize.X / (float)CaptureSize.Y;
		}
		else
		{
			// if the viewport is taller than it is wide
			XAxisMultiplier = CaptureSize.Y / (float)CaptureSize.X;
			YAxisMultiplier = 1.0f;
		}

		ViewInitOptions.ProjectionMatrix = FReversedZPerspectiveMatrix (
			FOV,
			FOV,
			XAxisMultiplier,
			YAxisMultiplier,
			GNearClippingPlane,
			GNearClippingPlane
			);
	}

	FSceneView* View = new FSceneView(ViewInitOptions);

	View->bIsSceneCapture = true;

	ViewFamily.Views.Add(View);

	View->StartFinalPostprocessSettings(ViewLocation);
	if (!bCaptureSceneColour)
	{
		View->OverridePostProcessSettings(*PostProcessSettings, PostProcessBlendWeight);
	}
	View->EndFinalPostprocessSettings();

	return FSceneRenderer::CreateSceneRenderer(&ViewFamily, NULL);
}

void FScene::UpdateSceneCaptureContents(USceneCaptureComponent2D* CaptureComponent)
{
	check(CaptureComponent);

	if (CaptureComponent->TextureTarget)
	{
		const FTransform& Transform = CaptureComponent->GetComponentToWorld();
		FMatrix ViewMatrix = Transform.ToInverseMatrixWithScale();
		FVector ViewLocation = Transform.GetTranslation();
		// swap axis st. x=z,y=x,z=y (unreal coord space) so that z is up
		ViewMatrix = ViewMatrix * FMatrix(
			FPlane(0,	0,	1,	0),
			FPlane(1,	0,	0,	0),
			FPlane(0,	1,	0,	0),
			FPlane(0,	0,	0,	1));
		const float FOV = CaptureComponent->FOVAngle * (float)PI / 360.0f;
		const bool bUseSceneColorTexture = CaptureComponent->CaptureSource == SCS_SceneColorHDR;
		FSceneRenderer* SceneRenderer = CreateSceneRenderer(CaptureComponent->TextureTarget, ViewMatrix , ViewLocation, FOV, CaptureComponent->MaxViewDistanceOverride, bUseSceneColorTexture, &CaptureComponent->PostProcessSettings, CaptureComponent->PostProcessBlendWeight);

		FTextureRenderTargetResource* TextureRenderTarget = CaptureComponent->TextureTarget->GameThread_GetRenderTargetResource();
		const FName OwnerName = CaptureComponent->GetOwner() ? CaptureComponent->GetOwner()->GetFName() : NAME_None;

		ENQUEUE_UNIQUE_RENDER_COMMAND_FOURPARAMETER( 
			CaptureCommand,
			FSceneRenderer*, SceneRenderer, SceneRenderer,
			FTextureRenderTargetResource*, TextureRenderTarget, TextureRenderTarget,
			FName, OwnerName, OwnerName,
			bool, bUseSceneColorTexture, bUseSceneColorTexture,
		{
			UpdateSceneCaptureContent_RenderThread(SceneRenderer, TextureRenderTarget, OwnerName, FResolveParams(), bUseSceneColorTexture);
		});
	}
}

void FScene::UpdateSceneCaptureContents(USceneCaptureComponentCube* CaptureComponent)
{
	struct FLocal
	{
		/** Creates a transformation for a cubemap face, following the D3D cubemap layout. */
		static FMatrix CalcCubeFaceTransform(ECubeFace Face, FVector WorldLocation)
		{
			static const FVector XAxis(1.f, 0.f, 0.f);
			static const FVector YAxis(0.f, 1.f, 0.f);
			static const FVector ZAxis(0.f, 0.f, 1.f);

			// vectors we will need for our basis
			FVector vUp(YAxis);
			FVector vDir;
			switch (Face)
			{
				case CubeFace_PosX:
					vDir = XAxis;
					break;
				case CubeFace_NegX:
					vDir = -XAxis;
					break;
				case CubeFace_PosY:
					vUp = -ZAxis;
					vDir = YAxis;
					break;
				case CubeFace_NegY:
					vUp = ZAxis;
					vDir = -YAxis;
					break;
				case CubeFace_PosZ:
					vDir = ZAxis;
					break;
				case CubeFace_NegZ:
					vDir = -ZAxis;
					break;
			}
			// derive right vector
			FVector vRight(vUp ^ vDir);
			// create matrix from the 3 axes
			return FBasisVectorMatrix(vRight, vUp, vDir, -WorldLocation);
		}
	} ;

	check(CaptureComponent);

	if (GRHIFeatureLevel >= ERHIFeatureLevel::SM4 && CaptureComponent->TextureTarget)
	{
		const float FOV = 90 * (float)PI / 360.0f;
		for (int32 faceidx = 0; faceidx < (int32)ECubeFace::CubeFace_MAX; faceidx++)
		{
			const ECubeFace TargetFace = (ECubeFace)faceidx;
			const FVector Location = CaptureComponent->GetComponentToWorld().GetTranslation();
			const FMatrix ViewMatrix = FLocal::CalcCubeFaceTransform(TargetFace, Location);
			FSceneRenderer* SceneRenderer = CreateSceneRenderer(CaptureComponent->TextureTarget, ViewMatrix, Location, FOV, CaptureComponent->MaxViewDistanceOverride);

			FTextureRenderTargetCubeResource* TextureRenderTarget = static_cast<FTextureRenderTargetCubeResource*>(CaptureComponent->TextureTarget->GameThread_GetRenderTargetResource());
			const FName OwnerName = CaptureComponent->GetOwner() ? CaptureComponent->GetOwner()->GetFName() : NAME_None;

			ENQUEUE_UNIQUE_RENDER_COMMAND_FOURPARAMETER( 
				CaptureCommand,
				FSceneRenderer*, SceneRenderer, SceneRenderer,
				FTextureRenderTargetCubeResource*, TextureRenderTarget, TextureRenderTarget,
				FName, OwnerName, OwnerName,
				ECubeFace, TargetFace, TargetFace,
			{
				UpdateSceneCaptureContent_RenderThread(SceneRenderer, TextureRenderTarget, OwnerName, FResolveParams(FResolveRect(), TargetFace), true);
			});
		}
	}
}

