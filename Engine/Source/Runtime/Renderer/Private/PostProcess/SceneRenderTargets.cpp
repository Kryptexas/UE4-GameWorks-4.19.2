// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SceneRenderTargets.cpp: Scene render target implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"
#include "ReflectionEnvironment.h"
#include "LightPropagationVolume.h"

// for LightPropagationVolume feature, could be exposed
const int ReflectiveShadowMapResolution = 256;

/*-----------------------------------------------------------------------------
FSceneRenderTargets
-----------------------------------------------------------------------------*/

int32 GDownsampledOcclusionQueries = 0;
static FAutoConsoleVariableRef CVarDownsampledOcclusionQueries(
	TEXT("r.DownsampledOcclusionQueries"),
	GDownsampledOcclusionQueries,
	TEXT("Whether to issue occlusion queries to a downsampled depth buffer"),
	ECVF_RenderThreadSafe
	);

static TAutoConsoleVariable<int32> CVarSceneTargetsResizingMethod(
	TEXT("r.SceneRenderTargetResizeMethod"),
	0,
	TEXT("Control the scene render target resize method:\n")
	TEXT("(This value is only used in game mode and on windowing platforms.)\n")
	TEXT("0: Resize to match requested render size (Default) (Least memory use, can cause stalls when size changes e.g. ScreenPercentage)\n")
	TEXT("1: Fixed to screen resolution.\n")
	TEXT("2: Expands to encompass the largest requested render dimension. (Most memory use, least prone to allocation stalls.)"),	
	ECVF_RenderThreadSafe
	);

/** The global render targets used for scene rendering. */
TGlobalResource<FSceneRenderTargets> GSceneRenderTargets;

FIntPoint FSceneRenderTargets::GetSceneRenderTargetSize(const FSceneViewFamily& ViewFamily) const
{
	// Don't expose Clamped to the cvar since you need to at least grow to the initial state.
	enum ESizingMethods { RequestedSize, ScreenRes, Grow, VisibleSizingMethodsCount, Clamped};
	ESizingMethods SceneTargetsSizingMethod = Grow;

	bool bSceneCapture = false;
	for (int32 ViewIndex = 0; ViewIndex < ViewFamily.Views.Num(); ++ViewIndex)
	{
		bSceneCapture |= ViewFamily.Views[ViewIndex]->bIsSceneCapture;
	}

	if(!FPlatformProperties::SupportsWindowedMode())
	{
		// Force ScreenRes on non windowed platforms.
		SceneTargetsSizingMethod = RequestedSize;
	}
	else if (GIsEditor)
	{
		// Always grow scene render targets in the editor.
		SceneTargetsSizingMethod = Grow;
	}	
	else
	{
		// Otherwise use the setting specified by the console variable.
		SceneTargetsSizingMethod = (ESizingMethods) FMath::Clamp(CVarSceneTargetsResizingMethod.GetValueOnRenderThread(), 0, (int32)VisibleSizingMethodsCount);
	}
	
	if (bSceneCapture)
	{
		// In general, we don't want scenecapture to grow our buffers, because depending on the cvar for our game, we may not recover that memory.  This can be changed if necessary.
		// However, in the editor a user might have a small editor window, but be capturing cubemaps or other dynamic assets for data distribution, 
		// in which case we need to grow for correctness.
		// We also don't want to reallocate all our buffers for a temporary use case like a capture.  So we just clamp the biggest capture size to the currently available buffers.
		if (GIsEditor)
		{
			SceneTargetsSizingMethod = Grow;
		}
		else
		{			
			SceneTargetsSizingMethod = Clamped;
		}
	}

	switch (SceneTargetsSizingMethod)
	{
		case RequestedSize:
			return FIntPoint(ViewFamily.FamilySizeX, ViewFamily.FamilySizeY);
		case ScreenRes:
			return FIntPoint(GSystemResolution.ResX, GSystemResolution.ResY);
		case Grow:
			return FIntPoint(FMath::Max((uint32)GetBufferSizeXY().X, ViewFamily.FamilySizeX),
					FMath::Max((uint32)GetBufferSizeXY().Y, ViewFamily.FamilySizeY));
		case Clamped:
			if (((uint32)BufferSize.X < ViewFamily.FamilySizeX) || ((uint32)BufferSize.Y < ViewFamily.FamilySizeY))
			{
				UE_LOG(LogRenderer, Warning, TEXT("Capture target size: %ux%u clamped to %ux%u."), ViewFamily.FamilySizeX, ViewFamily.FamilySizeY, BufferSize.X, BufferSize.Y);
			}
			return FIntPoint(GetBufferSizeXY().X, GetBufferSizeXY().Y);
		default:
			checkNoEntry();
			return FIntPoint::ZeroValue;
	}
}

void FSceneRenderTargets::Allocate(const FSceneViewFamily& ViewFamily)
{
	check(IsInRenderingThread());

	FIntPoint DesiredBufferSize = GetSceneRenderTargetSize(ViewFamily);
	check(DesiredBufferSize.X > 0 && DesiredBufferSize.Y > 0);
	QuantizeBufferSize(DesiredBufferSize.X, DesiredBufferSize.Y);

	int GBufferFormat;
	{
		static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.GBufferFormat"));
	
		GBufferFormat = CVar->GetValueOnRenderThread();
	}

	int SceneColorFormat;
	{
		static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.SceneColorFormat"));
	
		SceneColorFormat = CVar->GetValueOnRenderThread();
	}
		
	bool bNewAllowStaticLighting;
	{
		static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.AllowStaticLighting"));

		bNewAllowStaticLighting = CVar->GetValueOnRenderThread() != 0;
	}

	bool bDownsampledOcclusionQueries = GDownsampledOcclusionQueries != 0;

	int32 MaxShadowResolution = GetCachedScalabilityCVars().MaxShadowResolution;

	int32 TranslucencyLightingVolumeDim = GTranslucencyLightingVolumeDim;

	uint32 Mobile32bpp = !IsMobileHDR() || IsMobileHDR32bpp();

	bool bLightPropagationVolume = UseLightPropagationVolumeRT();

	uint32 MinShadowResolution;
	{
		static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.Shadow.MinResolution"));

		MinShadowResolution = CVar->GetValueOnRenderThread();
	}

	if( (BufferSize.X != DesiredBufferSize.X) ||
		(BufferSize.Y != DesiredBufferSize.Y) ||
		(CurrentGBufferFormat != GBufferFormat) ||
		(CurrentSceneColorFormat != SceneColorFormat) ||
		(bAllowStaticLighting != bNewAllowStaticLighting) ||
		(bUseDownsizedOcclusionQueries != bDownsampledOcclusionQueries) ||
 		(CurrentMaxShadowResolution != MaxShadowResolution) ||
		(CurrentTranslucencyLightingVolumeDim != TranslucencyLightingVolumeDim) ||
		(CurrentMobile32bpp != Mobile32bpp) ||
		(bCurrentLightPropagationVolume != bLightPropagationVolume) ||
		(CurrentMinShadowResolution != MinShadowResolution))
	{
		CurrentGBufferFormat = GBufferFormat;
		CurrentSceneColorFormat = SceneColorFormat;
		bAllowStaticLighting = bNewAllowStaticLighting;
		bUseDownsizedOcclusionQueries = bDownsampledOcclusionQueries;
		CurrentMaxShadowResolution = MaxShadowResolution;
		CurrentTranslucencyLightingVolumeDim = TranslucencyLightingVolumeDim;
		CurrentMobile32bpp = Mobile32bpp;
		CurrentMinShadowResolution = MinShadowResolution;
		bCurrentLightPropagationVolume = bLightPropagationVolume;
		
		// Reinitialize the render targets for the given size.
		SetBufferSize(DesiredBufferSize.X, DesiredBufferSize.Y);

		UE_LOG(LogRenderer, Warning, TEXT("Reallocating scene render targets to support %ux%u."), BufferSize.X, BufferSize.Y);

		UpdateRHI();
	}
}

/** Clears the GBuffer render targets to default values. */
void FSceneRenderTargets::ClearGBufferTargets(const FLinearColor& ClearColor)
{
	SCOPED_DRAW_EVENT(ClearGBufferTargets, DEC_SCENE_ITEMS);

	// Clear GBufferA, GBufferB, GBufferC, GBufferD, GBufferE
	{
		GSceneRenderTargets.BeginRenderingSceneColor(true);

		int32 NumToClear = GSceneRenderTargets.GetNumGBufferTargets();
		if (NumToClear > 1)
		{
			// Using 0 and 1 ensures we go through the fast path on Intel integrated GPUs.
			// Normal could be 0.5,0.5,0.5 but then it would not use the fast path.
			FLinearColor ClearColors[6] = {ClearColor, FLinearColor(0, 0, 0, 0), FLinearColor(0,0,0,0), FLinearColor(0,0,0,0), FLinearColor(0,1,1,1), FLinearColor(1,1,1,1)};
			RHIClearMRT(true, NumToClear, ClearColors, false, 0, false, 0, FIntRect());
		}
		else
		{
			RHIClear(true, ClearColor, false, 0, false, 0, FIntRect());
		}
	}
}

void FSceneRenderTargets::BeginRenderingSceneColor( bool bGBufferPass )
{
	SCOPED_DRAW_EVENT(BeginRenderingSceneColor, DEC_SCENE_ITEMS);

	if(IsSimpleDynamicLightingEnabled())
	{
		bGBufferPass = false;
	}

	// Set the scene color surface as the render target, and the scene depth surface as the depth-stencil target.
	if (bGBufferPass && GRHIFeatureLevel >= ERHIFeatureLevel::SM4)
	{
		FTextureRHIParamRef RenderTargets[6] = {0};
		RenderTargets[0] = GetSceneColorSurface();
		RenderTargets[1] = GSceneRenderTargets.GBufferA->GetRenderTargetItem().TargetableTexture;
		RenderTargets[2] = GSceneRenderTargets.GBufferB->GetRenderTargetItem().TargetableTexture;
		RenderTargets[3] = GSceneRenderTargets.GBufferC->GetRenderTargetItem().TargetableTexture;
		RenderTargets[4] = GSceneRenderTargets.GBufferD->GetRenderTargetItem().TargetableTexture;

		uint32 MRTCount = ARRAY_COUNT(RenderTargets);

		if (bAllowStaticLighting)
		{
			RenderTargets[5] = GSceneRenderTargets.GBufferE->GetRenderTargetItem().TargetableTexture;
		}
		else
		{
			MRTCount--;
		}
		
		RHISetRenderTargets(MRTCount, RenderTargets, GetSceneDepthSurface(), 0, NULL);
	}
	else
	{
		RHISetRenderTarget(GetSceneColorTexture(), GetSceneDepthTexture());
	}
} 

int32 FSceneRenderTargets::GetNumGBufferTargets() const
{
	int32 NumGBufferTargets = 1;

	if (GRHIFeatureLevel >= ERHIFeatureLevel::SM4 && !IsSimpleDynamicLightingEnabled())
	{
		NumGBufferTargets = bAllowStaticLighting ? 6 : 5;
	}
	return NumGBufferTargets;
}

void FSceneRenderTargets::FinishRenderingSceneColor(bool bKeepChanges, const FResolveRect& ResolveRect)
{
	SCOPED_DRAW_EVENT(FinishRenderingSceneColor, DEC_SCENE_ITEMS);

	if(bKeepChanges)
	{
		ResolveSceneColor();
	}
}

bool FSceneRenderTargets::BeginRenderingCustomDepth(bool bPrimitives)
{
	IPooledRenderTarget* CustomDepth = RequestCustomDepth(bPrimitives);

	if(CustomDepth)
	{
		SCOPED_DRAW_EVENT(BeginRenderingCustomDepth, DEC_SCENE_ITEMS);

		RHISetRenderTarget(FTextureRHIRef(), CustomDepth->GetRenderTargetItem().ShaderResourceTexture);

		return true;
	}

	return false;
}

void FSceneRenderTargets::FinishRenderingCustomDepth(const FResolveRect& ResolveRect)
{
	SCOPED_DRAW_EVENT(FinishRenderingCustomDepth, DEC_SCENE_ITEMS);

	RHICopyToResolveTarget(SceneColor->GetRenderTargetItem().TargetableTexture, SceneColor->GetRenderTargetItem().ShaderResourceTexture, true, FResolveParams(ResolveRect));

	bCustomDepthIsValid = true;
}

/**
* Saves a previously rendered scene color target
*/
void FSceneRenderTargets::ResolveSceneColor(const FResolveRect& ResolveRect)
{
    SCOPED_DRAW_EVENT(ResolveSceneColor, DEC_SCENE_ITEMS);

	RHICopyToResolveTarget(GetSceneColorSurface(), GetSceneColorTexture(), true, FResolveParams(ResolveRect));
}

/** Resolves the GBuffer targets so that their resolved textures can be sampled. */
void FSceneRenderTargets::ResolveGBufferSurfaces(const FResolveRect& ResolveRect)
{
	if (GRHIFeatureLevel >= ERHIFeatureLevel::SM4)
	{
		SCOPED_DRAW_EVENT(ResolveGBufferSurfaces, DEC_SCENE_ITEMS);

		RHICopyToResolveTarget(GSceneRenderTargets.GBufferA->GetRenderTargetItem().TargetableTexture, GSceneRenderTargets.GBufferA->GetRenderTargetItem().ShaderResourceTexture, true, FResolveParams(ResolveRect));
		RHICopyToResolveTarget(GSceneRenderTargets.GBufferB->GetRenderTargetItem().TargetableTexture, GSceneRenderTargets.GBufferB->GetRenderTargetItem().ShaderResourceTexture, true, FResolveParams(ResolveRect));
		RHICopyToResolveTarget(GSceneRenderTargets.GBufferC->GetRenderTargetItem().TargetableTexture, GSceneRenderTargets.GBufferC->GetRenderTargetItem().ShaderResourceTexture, true, FResolveParams(ResolveRect));
		RHICopyToResolveTarget(GSceneRenderTargets.GBufferD->GetRenderTargetItem().TargetableTexture, GSceneRenderTargets.GBufferD->GetRenderTargetItem().ShaderResourceTexture, true, FResolveParams(ResolveRect));
		
		if (bAllowStaticLighting)
		{
			RHICopyToResolveTarget(GSceneRenderTargets.GBufferE->GetRenderTargetItem().TargetableTexture, GSceneRenderTargets.GBufferE->GetRenderTargetItem().ShaderResourceTexture, true, FResolveParams(ResolveRect));
		}
	}
}

void FSceneRenderTargets::BeginRenderingPrePass()
{
	SCOPED_DRAW_EVENT(BeginRenderingPrePass, DEC_SCENE_ITEMS);

	// Set the scene depth surface and a DUMMY buffer as color buffer
	// (as long as it's the same dimension as the depth buffer),
	RHISetRenderTarget( GetLightAttenuationSurface(), GetSceneDepthSurface());
}

void FSceneRenderTargets::FinishRenderingPrePass()
{
	SCOPED_DRAW_EVENT(FinishRenderingPrePass, DEC_SCENE_ITEMS);

	GRenderTargetPool.VisualizeTexture.SetCheckPoint(SceneDepthZ);
}

void FSceneRenderTargets::BeginRenderingShadowDepth()
{
	GRenderTargetPool.VisualizeTexture.SetCheckPoint(ShadowDepthZ);
	
	RHISetRenderTarget(FTextureRHIRef(), GetShadowDepthZSurface());
}

void FSceneRenderTargets::BeginRenderingCubeShadowDepth(int32 ShadowResolution)
{
	SCOPED_DRAW_EVENT(BeginRenderingCubeShadowDepth, DEC_SCENE_ITEMS);
	RHISetRenderTarget(FTextureRHIRef(), GetCubeShadowDepthZSurface(ShadowResolution));   
}

void FSceneRenderTargets::FinishRenderingShadowDepth(const FResolveRect& ResolveRect)
{
	// Resolve the shadow depth z surface.
	RHICopyToResolveTarget(GetShadowDepthZSurface(), GetShadowDepthZTexture(), false, FResolveParams(ResolveRect));
}

void FSceneRenderTargets::BeginRenderingReflectiveShadowMap( FLightPropagationVolume* Lpv )
{
	FTextureRHIParamRef RenderTargets[2];
	RenderTargets[0] = GetReflectiveShadowMapNormalSurface();
	RenderTargets[1] = GetReflectiveShadowMapDiffuseSurface();

	// Hook up the geometry volume UAVs
	FUnorderedAccessViewRHIParamRef Uavs[4];
	Uavs[0] = Lpv->GetGvListBufferUav();
	Uavs[1] = Lpv->GetGvListHeadBufferUav();
	Uavs[2] = Lpv->GetVplListBufferUav();
	Uavs[3] = Lpv->GetVplListHeadBufferUav();
	RHISetRenderTargets( ARRAY_COUNT(RenderTargets), RenderTargets, GetReflectiveShadowMapDepthSurface(), 4, Uavs);
}

void FSceneRenderTargets::FinishRenderingReflectiveShadowMap(const FResolveRect& ResolveRect)
{
	// Resolve the shadow depth z surface.
	RHICopyToResolveTarget(GetReflectiveShadowMapDepthSurface(), GetReflectiveShadowMapDepthTexture(), false, FResolveParams(ResolveRect));
	RHICopyToResolveTarget(GetReflectiveShadowMapDiffuseSurface(), GetReflectiveShadowMapDiffuseTexture(), false, FResolveParams(ResolveRect));
	RHICopyToResolveTarget(GetReflectiveShadowMapNormalSurface(), GetReflectiveShadowMapNormalTexture(), false, FResolveParams(ResolveRect));

	// Unset render targets
	FTextureRHIParamRef RenderTargets[2] = {NULL};
	FUnorderedAccessViewRHIParamRef Uavs[2] = {NULL};
	RHISetRenderTargets( ARRAY_COUNT(RenderTargets), RenderTargets, FTextureRHIParamRef(), 2, Uavs);
}

void FSceneRenderTargets::FinishRenderingCubeShadowDepth(int32 ShadowResolution, const FResolveParams& ResolveParams)
{
	SCOPED_DRAW_EVENT(FinishRenderingCubeShadowDepth, DEC_SCENE_ITEMS);
	RHICopyToResolveTarget(GetCubeShadowDepthZSurface(ShadowResolution), GetCubeShadowDepthZTexture(ShadowResolution), false, ResolveParams);
}

void FSceneRenderTargets::BeginRenderingSceneAlphaCopy()
{
	SCOPED_DRAW_EVENT(BeginRenderingSceneAlphaCopy, DEC_SCENE_ITEMS);
	GRenderTargetPool.VisualizeTexture.SetCheckPoint(GSceneRenderTargets.SceneAlphaCopy);
	RHISetRenderTarget(GetSceneAlphaCopySurface(), 0);
}

void FSceneRenderTargets::FinishRenderingSceneAlphaCopy()
{
	SCOPED_DRAW_EVENT(FinishRenderingSceneAlphaCopy, DEC_SCENE_ITEMS);
	RHICopyToResolveTarget(GetSceneAlphaCopySurface(), SceneAlphaCopy->GetRenderTargetItem().ShaderResourceTexture, false, FResolveParams(FResolveRect()));
	GRenderTargetPool.VisualizeTexture.SetCheckPoint(GSceneRenderTargets.SceneAlphaCopy);
}


void FSceneRenderTargets::BeginRenderingLightAttenuation()
{
	SCOPED_DRAW_EVENT(BeginRenderingLightAttenuation, DEC_SCENE_ITEMS);

	GRenderTargetPool.VisualizeTexture.SetCheckPoint(GSceneRenderTargets.LightAttenuation);

	// Set the light attenuation surface as the render target, and the scene depth buffer as the depth-stencil surface.
	RHISetRenderTarget(GetLightAttenuationSurface(),GetSceneDepthSurface());
}

void FSceneRenderTargets::FinishRenderingLightAttenuation()
{
	SCOPED_DRAW_EVENT(FinishRenderingLightAttenuation, DEC_SCENE_ITEMS);

	// Resolve the light attenuation surface.
	RHICopyToResolveTarget(GetLightAttenuationSurface(), LightAttenuation->GetRenderTargetItem().ShaderResourceTexture, false, FResolveParams(FResolveRect()));
	
	GRenderTargetPool.VisualizeTexture.SetCheckPoint(GSceneRenderTargets.LightAttenuation);
}

void FSceneRenderTargets::BeginRenderingTranslucency(const FViewInfo& View)
{
	// Use the scene color buffer.
	GSceneRenderTargets.BeginRenderingSceneColor();
	
	// viewport to match view size
	RHISetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);
}


bool FSceneRenderTargets::BeginRenderingSeparateTranslucency(const FViewInfo& View, bool bFirstTimeThisFrame)
{
	if(IsSeparateTranslucencyActive(View))
	{
		SCOPED_DRAW_EVENT(BeginSeparateTranslucency, DEC_SCENE_ITEMS);

		FSceneViewState* ViewState = (FSceneViewState*)View.State;

		// the RT should only be available for a short period during rendering
		if(bFirstTimeThisFrame)
		{
			check(!ViewState->SeparateTranslucencyRT);
		}

		TRefCountPtr<IPooledRenderTarget>& SeparateTranslucency = ViewState->GetSeparateTranslucency(View);

		// Use a separate render target for translucency
		RHISetRenderTarget(SeparateTranslucency->GetRenderTargetItem().TargetableTexture, GetSceneDepthSurface());
		RHISetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);
		
		if(bFirstTimeThisFrame)
		{
			RHIClear(true, FLinearColor(0, 0, 0, 1), false, 0, false, 0, FIntRect());
		}

		return true;
	}

	return false;
}

void FSceneRenderTargets::FinishRenderingSeparateTranslucency(const FViewInfo& View)
{
	if(IsSeparateTranslucencyActive(View))
	{
		FSceneViewState* ViewState = (FSceneViewState*)View.State;
		TRefCountPtr<IPooledRenderTarget>& SeparateTranslucency = ViewState->GetSeparateTranslucency(View);

		RHICopyToResolveTarget(SeparateTranslucency->GetRenderTargetItem().TargetableTexture, SeparateTranslucency->GetRenderTargetItem().ShaderResourceTexture, true, FResolveParams());
	}
}

void FSceneRenderTargets::BeginRenderingDistortionAccumulation()
{
	SCOPED_DRAW_EVENT(BeginRenderingDistortionAccumulation, DEC_SCENE_ITEMS);

	// use RGBA8 light target for accumulating distortion offsets	
	// R = positive X offset
	// G = positive Y offset
	// B = negative X offset
	// A = negative Y offset

	RHISetRenderTarget(GetLightAttenuationSurface(),GetSceneDepthSurface());
}

void FSceneRenderTargets::FinishRenderingDistortionAccumulation()
{
	SCOPED_DRAW_EVENT(FinishRenderingDistortionAccumulation, DEC_SCENE_ITEMS);

	RHICopyToResolveTarget(GetLightAttenuationSurface(), GetLightAttenuationTexture(), false, FResolveParams());

	// to be able to observe results with VisualizeTexture
	GRenderTargetPool.VisualizeTexture.SetCheckPoint(LightAttenuation);
}

void FSceneRenderTargets::ResolveSceneDepthTexture()
{
	SCOPED_DRAW_EVENT(ResolveSceneDepthTexture, DEC_SCENE_ITEMS);

	RHICopyToResolveTarget(GetSceneDepthSurface(), GetSceneDepthTexture(), true, FResolveParams());
}

void FSceneRenderTargets::ResolveSceneDepthToAuxiliaryTexture()
{
	SCOPED_DRAW_EVENT(ResolveSceneDepthToAuxiliaryTexture, DEC_SCENE_ITEMS);

	RHICopyToResolveTarget(GetSceneDepthSurface(), GetAuxiliarySceneDepthTexture(), true, FResolveParams());
}

void FSceneRenderTargets::BeginRenderingHitProxies()
{
	RHISetRenderTarget(GetHitProxySurface(),GetSceneDepthSurface());
}

void FSceneRenderTargets::FinishRenderingHitProxies()
{
	RHICopyToResolveTarget(GetHitProxySurface(), GetHitProxyTexture(), false, FResolveParams());
}

void FSceneRenderTargets::CleanUpEditorPrimitiveTargets()
{
	EditorPrimitivesDepth.SafeRelease();
	EditorPrimitivesColor.SafeRelease();
}

int32 FSceneRenderTargets::GetEditorMSAACompositingSampleCount() const
{
	int32 Value = 1;

	// only supported on SM5 yet (SM4 doesn't have MSAA sample load functionality which makes it harder to implement)
	if(GRHIFeatureLevel >= ERHIFeatureLevel::SM5)
	{
		static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.MSAA.CompositingSampleCount"));

		Value = CVar->GetValueOnRenderThread();

		if(Value <= 1)
		{
			Value = 1;
		}
		else if(Value <= 2)
		{
			Value = 2;
		}
		else if(Value <= 4)
		{
			Value = 4;
		}
		else
		{
			Value = 8;
		}
	}

	return Value;
}

const FTexture2DRHIRef& FSceneRenderTargets::GetEditorPrimitivesColor()
{
	const bool bIsValid = IsValidRef(EditorPrimitivesColor);

	if( !bIsValid || EditorPrimitivesColor->GetDesc().NumSamples != GetEditorMSAACompositingSampleCount() )
	{
		// If the target is does not match the MSAA settings it needs to be recreated
		InitEditorPrimitivesColor();
	}

	return (const FTexture2DRHIRef&)EditorPrimitivesColor->GetRenderTargetItem().TargetableTexture;
}


const FTexture2DRHIRef& FSceneRenderTargets::GetEditorPrimitivesDepth()
{
	const bool bIsValid = IsValidRef(EditorPrimitivesDepth);

	if( !bIsValid || EditorPrimitivesDepth->GetDesc().NumSamples != GetEditorMSAACompositingSampleCount() )
	{
		// If the target is does not match the MSAA settings it needs to be recreated
		InitEditorPrimitivesDepth();
	}

	return (const FTexture2DRHIRef&)EditorPrimitivesDepth->GetRenderTargetItem().TargetableTexture;
}


bool FSceneRenderTargets::IsSeparateTranslucencyActive(const FViewInfo& View) const
{
	static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.SeparateTranslucency"));
	int32 Value = CVar->GetValueOnRenderThread();

	return (Value != 0) && GRHIFeatureLevel >= ERHIFeatureLevel::SM4
		&& View.Family->EngineShowFlags.PostProcessing
		&& View.Family->EngineShowFlags.SeparateTranslucency
		&& View.State != NULL;	// We require a ViewState in order for separate translucency to work (it keeps track of our SeparateTranslucencyRT)
}

void FSceneRenderTargets::InitEditorPrimitivesColor()
{
	FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(BufferSize, 
		PF_B8G8R8A8, 
		TexCreate_None, 
		TexCreate_ShaderResource | TexCreate_RenderTargetable,
		false));

	Desc.NumSamples = GetEditorMSAACompositingSampleCount();

	GRenderTargetPool.FindFreeElement(Desc, EditorPrimitivesColor, TEXT("EditorPrimitivesColor"));
}

void FSceneRenderTargets::InitEditorPrimitivesDepth()
{
	FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(BufferSize, 
		PF_DepthStencil, 
		TexCreate_None, 
		TexCreate_ShaderResource | TexCreate_DepthStencilTargetable,
		false));

	Desc.NumSamples = GetEditorMSAACompositingSampleCount();

	GRenderTargetPool.FindFreeElement(Desc, EditorPrimitivesDepth, TEXT("EditorPrimitivesDepth"));
}

void FSceneRenderTargets::QuantizeBufferSize(int32& InOutBufferSizeX, int32& InOutBufferSizeY) const
{
	// ensure sizes are dividable by DividableBy to get post processing effects with lower resolution working well
	const uint32 DividableBy = 8;

	const uint32 Mask = ~(DividableBy - 1);
	InOutBufferSizeX = (InOutBufferSizeX + DividableBy - 1) & Mask;
	InOutBufferSizeY = (InOutBufferSizeY + DividableBy - 1) & Mask;
}

void FSceneRenderTargets::SetBufferSize(int32 InBufferSizeX, int32 InBufferSizeY)
{
	QuantizeBufferSize(InBufferSizeX, InBufferSizeY);
	BufferSize.X = InBufferSizeX;
	BufferSize.Y = InBufferSizeY;
}

uint8 Quantize8SignedByte(float x)
{
	// -1..1 -> 0..1
	float y = x * 0.5f + 0.5f;

	// 0..1 -> 0..255
	int32 Ret = (int32)(y * 255.999f);

	check(Ret >= 0);
	check(Ret <= 255);

	return Ret;
}

void FSceneRenderTargets::AllocateForwardShadingPathRenderTargets()
{
	{
		// Create a texture to store the resolved scene depth, and a render-targetable surface to hold the unresolved scene depth.
		FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(BufferSize, PF_DepthStencil, TexCreate_None, TexCreate_DepthStencilTargetable, false));
		GRenderTargetPool.FindFreeElement(Desc, SceneDepthZ, TEXT("SceneDepthZ"));
	}

	EPixelFormat Format = GSupportsRenderTargetFormat_PF_FloatRGBA ? PF_FloatRGBA : PF_B8G8R8A8;
	if (!IsMobileHDR() || IsMobileHDR32bpp()) 
	{
			Format = PF_B8G8R8A8;
	}
	// Create the scene color.
	{
		FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(BufferSize, Format, TexCreate_None, TexCreate_RenderTargetable, false));
		GRenderTargetPool.FindFreeElement(Desc, SceneColor, TEXT("SceneColor"));
	}

	// For 64-bit ES2 without framebuffer fetch, create extra render target for copy of alpha channel.
	if((Format == PF_FloatRGBA) && (GSupportsShaderFramebufferFetch == false)) 
	{
#if PLATFORM_HTML5 || PLATFORM_ANDROID
		// creating a PF_R16F (a true one-channel renderable fp texture) is only supported on GL if EXT_texture_rg is available.  It's present
		// on iOS, but not in WebGL or Android.
		FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(BufferSize, PF_FloatRGBA, TexCreate_None, TexCreate_RenderTargetable, false));
#else
		FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(BufferSize, PF_R16F, TexCreate_None, TexCreate_RenderTargetable, false));
#endif
		GRenderTargetPool.FindFreeElement(Desc, SceneAlphaCopy, TEXT("SceneAlphaCopy"));
	}
	else
	{
		SceneAlphaCopy = GSystemTextures.MaxFP16Depth;
	}
}

// for easier use of "VisualizeTexture"
static TCHAR* const GetVolumeName(uint32 Id, bool bDirectional)
{
	// (TCHAR*) for non VisualStudio
	switch(Id)
	{
		case 0: return bDirectional ? (TCHAR*)TEXT("TranslucentVolumeDir0") : (TCHAR*)TEXT("TranslucentVolume0");
		case 1: return bDirectional ? (TCHAR*)TEXT("TranslucentVolumeDir1") : (TCHAR*)TEXT("TranslucentVolume1");
		case 2: return bDirectional ? (TCHAR*)TEXT("TranslucentVolumeDir2") : (TCHAR*)TEXT("TranslucentVolume2");

		default:
			check(0);
	}
	return (TCHAR*)TEXT("InvalidName");
}

void FSceneRenderTargets::AllocateDeferredShadingPathRenderTargets()
{
	{
		// Create a texture to store the resolved scene depth, and a render-targetable surface to hold the unresolved scene depth.
		FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(BufferSize, PF_DepthStencil, TexCreate_None, TexCreate_DepthStencilTargetable, false));
		Desc.Flags |= TexCreate_FastVRAM;
		GRenderTargetPool.FindFreeElement(Desc, SceneDepthZ, TEXT("SceneDepthZ"));
	}
		
	// When targeting DX Feature Level 10, create an auxiliary texture to store the resolved scene depth, and a render-targetable surface to hold the unresolved scene depth.
	if(!GSupportsDepthFetchDuringDepthTest)
	{
		FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(BufferSize, PF_DepthStencil, TexCreate_None, TexCreate_DepthStencilTargetable, false));
		GRenderTargetPool.FindFreeElement(Desc, AuxiliarySceneDepthZ, TEXT("AuxiliarySceneDepthZ"));
	}

	// Potentially allocate an alpha channel in the scene color texture to store the resolved scene depth.
	EPixelFormat SceneColorBufferFormat = PF_FloatRGBA;

	switch(CurrentSceneColorFormat)
	{
		case 0:
			SceneColorBufferFormat = PF_R8G8B8A8; break;
		case 1:
			SceneColorBufferFormat = PF_A2B10G10R10; break;
		case 2:	
			SceneColorBufferFormat = PF_FloatR11G11B10; break;
		case 3:	
			SceneColorBufferFormat = PF_FloatRGB; break;
		case 4:
			// default
			break;
		case 5:
			SceneColorBufferFormat = PF_A32B32G32R32F; break;
	}

	// Create a quarter-sized version of the scene depth.
	{
		FIntPoint SmallDepthZSize(FMath::Max<uint32>(BufferSize.X / SmallColorDepthDownsampleFactor, 1), FMath::Max<uint32>(BufferSize.Y / SmallColorDepthDownsampleFactor, 1));

		FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(SmallDepthZSize, PF_DepthStencil, TexCreate_None, TexCreate_DepthStencilTargetable, true));
		GRenderTargetPool.FindFreeElement(Desc, SmallDepthZ, TEXT("SmallDepthZ"));
	}

	// Create the scene color.
	{
		FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(BufferSize, SceneColorBufferFormat, TexCreate_None, TexCreate_RenderTargetable, false));
		Desc.Flags |= TexCreate_FastVRAM;
		GRenderTargetPool.FindFreeElement(Desc, SceneColor, TEXT("SceneColor"));
	}

	// Create a texture to store the resolved light attenuation values, and a render-targetable surface to hold the unresolved light attenuation values.
	{
		FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(BufferSize, PF_B8G8R8A8, TexCreate_None, TexCreate_RenderTargetable, false));
		Desc.Flags |= TexCreate_FastVRAM;
		GRenderTargetPool.FindFreeElement(Desc, LightAttenuation, TEXT("LightAttenuation"));

		// the channel assignment is documented in ShadowRendering.cpp (look for Light Attenuation channel assignment)
	}

	// Set up quarter size scene color shared texture
	const FIntPoint ShadowBufferResolution = GetShadowDepthTextureResolution();

	const FIntPoint TranslucentShadowBufferResolution = GetTranslucentShadowDepthTextureResolution();

	if (GRHIFeatureLevel >= ERHIFeatureLevel::SM4)
	{
		for (int32 SurfaceIndex = 0; SurfaceIndex < NumTranslucencyShadowSurfaces; SurfaceIndex++)
		{
			if (!TranslucencyShadowTransmission[SurfaceIndex])
			{
				// Using PF_FloatRGBA because Fourier coefficients used by Fourier opacity maps have a large range and can be negative
				FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(TranslucentShadowBufferResolution, PF_FloatRGBA, TexCreate_None, TexCreate_RenderTargetable, false));
				GRenderTargetPool.FindFreeElement(Desc, TranslucencyShadowTransmission[SurfaceIndex], TEXT("TranslucencyShadowTransmission"));
			}
		}
	}

	if (GRHIFeatureLevel >= ERHIFeatureLevel::SM4)
	{
		// Create several shadow depth cube maps with different resolutions, to handle different sized shadows on the screen
		for (int32 SurfaceIndex = 0; SurfaceIndex < NumCubeShadowDepthSurfaces; SurfaceIndex++)
		{
			const int32 SurfaceResolution = GetCubeShadowDepthZResolution(SurfaceIndex);

			FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::CreateCubemapDesc(SurfaceResolution, PF_ShadowDepth, TexCreate_None, TexCreate_DepthStencilTargetable, false));
			GRenderTargetPool.FindFreeElement(Desc, CubeShadowDepthZ[SurfaceIndex], TEXT("CubeShadowDepthZ[]"));
		}
	}

	//create the shadow depth texture and/or surface
	{
		FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(ShadowBufferResolution, PF_ShadowDepth, TexCreate_None, TexCreate_DepthStencilTargetable, false));
		GRenderTargetPool.FindFreeElement(Desc, ShadowDepthZ, TEXT("ShadowDepthZ"));
	}
		
	{
		FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(GetPreShadowCacheTextureResolution(), PF_ShadowDepth, TexCreate_None, TexCreate_DepthStencilTargetable, false));
		GRenderTargetPool.FindFreeElement(Desc, PreShadowCacheDepthZ, TEXT("PreShadowCacheDepthZ"));
		// Mark the preshadow cache as newly allocated, so the cache will know to update
		bPreshadowCacheNewlyAllocated = true;
	}

	// Create the required render targets if running Highend.
	if (GRHIFeatureLevel >= ERHIFeatureLevel::SM4)
	{
		// good to see the quality loss due to precision in the gbuffer
		bool bHighPrecisionGBuffers = (CurrentGBufferFormat >= 5);
		// good to profile the impact of non 8 bit formats
		bool bEnforce8BitPerChannel = (CurrentGBufferFormat == 0);

		// Create the world-space normal g-buffer.
		{
			EPixelFormat NormalGBufferFormat = bHighPrecisionGBuffers ? PF_FloatRGBA : PF_A2B10G10R10;

			if(bEnforce8BitPerChannel)
			{
				NormalGBufferFormat = PF_B8G8R8A8;
			}

			FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(BufferSize, NormalGBufferFormat, TexCreate_None, TexCreate_RenderTargetable, false));
			GRenderTargetPool.FindFreeElement(Desc, GBufferA, TEXT("GBufferA"));
		}

		// Create the specular color and power g-buffer.
		{
			const EPixelFormat SpecularGBufferFormat = bHighPrecisionGBuffers ? PF_FloatRGBA : PF_B8G8R8A8;

			FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(BufferSize, SpecularGBufferFormat, TexCreate_None, TexCreate_RenderTargetable, false));
			GRenderTargetPool.FindFreeElement(Desc, GBufferB, TEXT("GBufferB"));
		}

		// Create the diffuse color g-buffer.
		{
			const EPixelFormat DiffuseGBufferFormat = bHighPrecisionGBuffers ? PF_FloatRGBA : PF_B8G8R8A8;

			FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(BufferSize, DiffuseGBufferFormat, TexCreate_None, TexCreate_RenderTargetable, false));
			GRenderTargetPool.FindFreeElement(Desc, GBufferC, TEXT("GBufferC"));
		}

		// Create the mask g-buffer (e.g. SSAO, subsurface scattering, wet surface mask, skylight mask, ...).
		{
			FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(BufferSize, PF_B8G8R8A8, TexCreate_None, TexCreate_RenderTargetable, false));
			GRenderTargetPool.FindFreeElement(Desc, GBufferD, TEXT("GBufferD"));
		}

		if (bAllowStaticLighting)
		{
			FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(BufferSize, PF_B8G8R8A8, TexCreate_None, TexCreate_RenderTargetable, false));
			GRenderTargetPool.FindFreeElement(Desc, GBufferE, TEXT("GBufferE"));
		}

		// Create the screen space ambient occlusion buffer
		{
			FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(BufferSize, PF_B8G8R8A8, TexCreate_None, TexCreate_RenderTargetable, false));
			GRenderTargetPool.FindFreeElement(Desc, ScreenSpaceAO, TEXT("ScreenSpaceAO"));
		}

		{
			for (int32 RTSetIndex = 0; RTSetIndex < NumTranslucentVolumeRenderTargetSets; RTSetIndex++)
			{
				GRenderTargetPool.FindFreeElement(
					FPooledRenderTargetDesc(FPooledRenderTargetDesc::CreateVolumeDesc(
						GTranslucencyLightingVolumeDim,
						GTranslucencyLightingVolumeDim,
						GTranslucencyLightingVolumeDim,
						PF_FloatRGBA,
						0,
						TexCreate_ShaderResource | TexCreate_RenderTargetable,
						false)),
					TranslucencyLightingVolumeAmbient[RTSetIndex],
					GetVolumeName(RTSetIndex, false)
					);

				GRenderTargetPool.FindFreeElement(
					FPooledRenderTargetDesc(FPooledRenderTargetDesc::CreateVolumeDesc(
						GTranslucencyLightingVolumeDim,
						GTranslucencyLightingVolumeDim,
						GTranslucencyLightingVolumeDim,
						PF_FloatRGBA,
						0,
						TexCreate_ShaderResource | TexCreate_RenderTargetable,
						false)),
					TranslucencyLightingVolumeDirectional[RTSetIndex],
					GetVolumeName(RTSetIndex, true)
					);
			}
		}

		extern int32 GUseIndirectLightingCacheInLightingVolume;

		if (GUseIndirectLightingCacheInLightingVolume)
		{
			GRenderTargetPool.FindFreeElement(
				FPooledRenderTargetDesc(FPooledRenderTargetDesc::CreateVolumeDesc(
					GTranslucencyLightingVolumeDim / TranslucentVolumeGISratchDownsampleFactor,
					GTranslucencyLightingVolumeDim / TranslucentVolumeGISratchDownsampleFactor,
					GTranslucencyLightingVolumeDim / TranslucentVolumeGISratchDownsampleFactor,
					PF_FloatRGB,
					0,
					TexCreate_ShaderResource | TexCreate_RenderTargetable,
					false)),
				TranslucencyLightingVolumeGIScratch,
				TEXT("TranslucentVolumeGIScratch")
				);
		}
	}

	extern ENGINE_API int32 GReflectionCaptureSize;
	const int32 NumReflectionCaptureMips = FMath::CeilLogTwo(GReflectionCaptureSize) + 1;

	{
		uint32 TexFlags = TexCreate_None;

		if (!GSupportsGSRenderTargetLayerSwitchingToMips)
		{
			TexFlags = TexCreate_TargetArraySlicesIndependently;
		}

		// Create scratch cubemaps for filtering passes
		FPooledRenderTargetDesc Desc2(FPooledRenderTargetDesc::CreateCubemapDesc(GReflectionCaptureSize, PF_FloatRGBA, TexFlags, TexCreate_RenderTargetable, false, 1, NumReflectionCaptureMips));
		GRenderTargetPool.FindFreeElement(Desc2, ReflectionColorScratchCubemap[0], TEXT("ReflectionColorScratchCubemap0"));
		GRenderTargetPool.FindFreeElement(Desc2, ReflectionColorScratchCubemap[1], TEXT("ReflectionColorScratchCubemap1"));

		FPooledRenderTargetDesc Desc3(FPooledRenderTargetDesc::Create2DDesc(FIntPoint(1, 1), PF_R32_FLOAT, TexCreate_None, TexCreate_RenderTargetable, false));
		GRenderTargetPool.FindFreeElement(Desc3, ReflectionBrightness, TEXT("ReflectionBrightness"));
	}

	extern int32 GDiffuseIrradianceCubemapSize;
	const int32 NumDiffuseIrradianceMips = FMath::CeilLogTwo(GDiffuseIrradianceCubemapSize) + 1;

	{
		FPooledRenderTargetDesc Desc2(FPooledRenderTargetDesc::CreateCubemapDesc(GDiffuseIrradianceCubemapSize, PF_FloatRGBA, TexCreate_None, TexCreate_RenderTargetable, false, 1, NumDiffuseIrradianceMips));
		GRenderTargetPool.FindFreeElement(Desc2, DiffuseIrradianceScratchCubemap[0], TEXT("DiffuseIrradianceScratchCubemap0"));
		GRenderTargetPool.FindFreeElement(Desc2, DiffuseIrradianceScratchCubemap[1], TEXT("DiffuseIrradianceScratchCubemap1"));
	}

	{
		FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(FIntPoint(FSHVector3::MaxSHBasis, 1), PF_FloatRGBA, TexCreate_None, TexCreate_RenderTargetable, false));
		GRenderTargetPool.FindFreeElement(Desc, SkySHIrradianceMap, TEXT("SkySHIrradianceMap"));
	}

	{
		uint32 LightAccumulationUAVFlag = GRHIFeatureLevel == ERHIFeatureLevel::SM5 ? TexCreate_UAV : 0;
		FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(BufferSize, SceneColorBufferFormat, TexCreate_None, TexCreate_RenderTargetable | LightAccumulationUAVFlag, false));
		GRenderTargetPool.FindFreeElement(Desc, LightAccumulation, TEXT("LightAccumulation"));
	}

	if (GRHIFeatureLevel == ERHIFeatureLevel::SM5)
	{
		LightAccumulationUAV = RHICreateUnorderedAccessView(LightAccumulation->GetRenderTargetItem().TargetableTexture);

		// Create the reflective shadow map textures for LightPropagationVolume feature
		if(bCurrentLightPropagationVolume)
		{
			{
				FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(GetReflectiveShadowMapTextureResolution(), PF_R8G8B8A8, TexCreate_None, TexCreate_RenderTargetable, false));
				GRenderTargetPool.FindFreeElement(Desc, ReflectiveShadowMapNormal, TEXT("RSMNormal"));
			}

			{
				FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(GetReflectiveShadowMapTextureResolution(), PF_FloatR11G11B10, TexCreate_None, TexCreate_RenderTargetable, false));
				GRenderTargetPool.FindFreeElement(Desc, ReflectiveShadowMapDiffuse, TEXT("RSMDiffuse"));
			}

			{
				FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(GetReflectiveShadowMapTextureResolution(), PF_DepthStencil, TexCreate_None, TexCreate_DepthStencilTargetable , false));
				GRenderTargetPool.FindFreeElement(Desc, ReflectiveShadowMapDepth, TEXT("RSMDepth"));
			}
		}
	}
}

void FSceneRenderTargets::InitDynamicRHI()
{
	if(BufferSize.X > 0 && BufferSize.Y > 0)
	{
		// start with a defined state for the scissor rect (D3D11 was returning (0,0,0,0) which caused a clear to not execute correctly)
		// todo: move this to an earlier place (for dx9 is has to be after device creation which is after window creation)
		RHISetScissorRect(false, 0, 0, 0, 0);

		if (GRHIFeatureLevel == ERHIFeatureLevel::ES2)
		{
			AllocateForwardShadingPathRenderTargets();
		}
		else
		{
			AllocateDeferredShadingPathRenderTargets();
		}
	}
}

void FSceneRenderTargets::ReleaseDynamicRHI()
{ 
	SceneColor.SafeRelease();
	SceneAlphaCopy.SafeRelease();
	SceneDepthZ.SafeRelease();
	AuxiliarySceneDepthZ.SafeRelease();
	SmallDepthZ.SafeRelease();
	GBufferA.SafeRelease();
	GBufferB.SafeRelease();
	GBufferC.SafeRelease();
	GBufferD.SafeRelease();
	GBufferE.SafeRelease();
	DBufferA.SafeRelease();
	DBufferB.SafeRelease();
	DBufferC.SafeRelease();
	ScreenSpaceAO.SafeRelease();
	LightAttenuation.SafeRelease();
	CustomDepth.SafeRelease();
	LightAccumulation.SafeRelease();
	ReflectiveShadowMapNormal.SafeRelease();
	ReflectiveShadowMapDiffuse.SafeRelease();
	ReflectiveShadowMapDepth.SafeRelease();

	for (int32 SurfaceIndex = 0; SurfaceIndex < NumTranslucencyShadowSurfaces; SurfaceIndex++)
	{
		TranslucencyShadowTransmission[SurfaceIndex].SafeRelease();
	}

	ShadowDepthZ.SafeRelease();
	PreShadowCacheDepthZ.SafeRelease();
	LightAccumulationUAV.SafeRelease();
	
	for(int32 Index = 0; Index < NumCubeShadowDepthSurfaces; ++Index)
	{
		CubeShadowDepthZ[Index].SafeRelease();
	}

	for (int32 i = 0; i < ARRAY_COUNT(ReflectionColorScratchCubemap); i++)
	{
		ReflectionColorScratchCubemap[i].SafeRelease();
	}

	ReflectionBrightness.SafeRelease();

	for (int32 i = 0; i < ARRAY_COUNT(DiffuseIrradianceScratchCubemap); i++)
	{
		DiffuseIrradianceScratchCubemap[i].SafeRelease();
	}

	SkySHIrradianceMap.SafeRelease();

	for (int32 RTSetIndex = 0; RTSetIndex < NumTranslucentVolumeRenderTargetSets; RTSetIndex++)
	{
		TranslucencyLightingVolumeAmbient[RTSetIndex].SafeRelease();
		TranslucencyLightingVolumeDirectional[RTSetIndex].SafeRelease();
	}

	TranslucencyLightingVolumeGIScratch.SafeRelease();
	
	EditorPrimitivesColor.SafeRelease();
	EditorPrimitivesDepth.SafeRelease();

	GRenderTargetPool.FreeUnusedResources();
}

/** Returns the size of the shadow depth buffer, taking into account platform limitations and game specific resolution limits. */
FIntPoint FSceneRenderTargets::GetShadowDepthTextureResolution() const
{
	int32 MaxShadowRes = GetCachedScalabilityCVars().MaxShadowResolution;
	const FIntPoint ShadowBufferResolution(
			FMath::Clamp(MaxShadowRes,1,GMaxShadowDepthBufferSizeX),
			FMath::Clamp(MaxShadowRes,1,GMaxShadowDepthBufferSizeY));
	
	return ShadowBufferResolution;
}

FIntPoint FSceneRenderTargets::GetReflectiveShadowMapTextureResolution() const
{
	return FIntPoint( ReflectiveShadowMapResolution, ReflectiveShadowMapResolution );
}

FIntPoint FSceneRenderTargets::GetPreShadowCacheTextureResolution() const
{
	const FIntPoint ShadowDepthResolution = GetShadowDepthTextureResolution();
	// Higher numbers increase cache hit rate but also memory usage
	const int32 ExpandFactor = 2;

	auto CVarPreShadowResolutionFactor = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.Shadow.PreShadowResolutionFactor"));
	return FIntPoint(FMath::Trunc(ShadowDepthResolution.X * CVarPreShadowResolutionFactor->GetValueOnRenderThread()), FMath::Trunc(ShadowDepthResolution.Y * CVarPreShadowResolutionFactor->GetValueOnRenderThread())) * ExpandFactor;
}

FIntPoint FSceneRenderTargets::GetTranslucentShadowDepthTextureResolution() const
{
	const FIntPoint ShadowDepthResolution = GetShadowDepthTextureResolution();
	return ShadowDepthResolution;
}

bool FSceneRenderTargets::ShouldDoMSAAInDeferredPasses() const
{
	return false;
}

static int32 GetCustomDepthCVar()
{
	static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.CustomDepth"));
	
	check(CVar);
	return CVar->GetValueOnRenderThread();
}

IPooledRenderTarget* FSceneRenderTargets::RequestCustomDepth(bool bPrimitives)
{
	int Value = GetCustomDepthCVar();

	if((Value == 1  && bPrimitives) || Value == 2)
	{
		FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(BufferSize, PF_DepthStencil, TexCreate_None, TexCreate_DepthStencilTargetable, false));
		GRenderTargetPool.FindFreeElement(Desc, CustomDepth, TEXT("CustomDepth"));
		return CustomDepth;
	}

	//
	return 0;
}

/** Returns an index in the range [0, NumCubeShadowDepthSurfaces) given an input resolution. */
int32 FSceneRenderTargets::GetCubeShadowDepthZIndex(int32 ShadowResolution) const
{
	static auto CVarMinShadowResolution = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.Shadow.MinResolution"));
	FIntPoint ObjectShadowBufferResolution = GetShadowDepthTextureResolution();

	// Use a lower resolution because cubemaps use a lot of memory
	ObjectShadowBufferResolution.X /= 2;
	ObjectShadowBufferResolution.Y /= 2;
	const int32 SurfaceSizes[NumCubeShadowDepthSurfaces] =
	{
		ObjectShadowBufferResolution.X,
		ObjectShadowBufferResolution.X / 2,
		ObjectShadowBufferResolution.X / 4,
		ObjectShadowBufferResolution.X / 8,
		CVarMinShadowResolution->GetValueOnRenderThread()
	};

	for (int32 SearchIndex = 0; SearchIndex < NumCubeShadowDepthSurfaces; SearchIndex++)
	{
		if (ShadowResolution >= SurfaceSizes[SearchIndex])
		{
			return SearchIndex;
		}
	}

	check(0);
	return 0;
}

/** Returns the appropriate resolution for a given cube shadow index. */
int32 FSceneRenderTargets::GetCubeShadowDepthZResolution(int32 ShadowIndex) const
{
	checkSlow(ShadowIndex >= 0 && ShadowIndex < NumCubeShadowDepthSurfaces);

	static auto CVarMinShadowResolution = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.Shadow.MinResolution"));
	FIntPoint ObjectShadowBufferResolution = GetShadowDepthTextureResolution();

	// Use a lower resolution because cubemaps use a lot of memory
	ObjectShadowBufferResolution.X = FMath::Max(ObjectShadowBufferResolution.X / 2, 1);
	ObjectShadowBufferResolution.Y = FMath::Max(ObjectShadowBufferResolution.Y / 2, 1);
	const int32 SurfaceSizes[NumCubeShadowDepthSurfaces] =
	{
		ObjectShadowBufferResolution.X,
		FMath::Max(ObjectShadowBufferResolution.X / 2, 1),
		FMath::Max(ObjectShadowBufferResolution.X / 4, 1),
		FMath::Max(ObjectShadowBufferResolution.X / 8, 1),
		CVarMinShadowResolution->GetValueOnRenderThread()
	};
	return SurfaceSizes[ShadowIndex];
}

/*-----------------------------------------------------------------------------
FSceneTextureShaderParameters
-----------------------------------------------------------------------------*/

//
void FSceneTextureShaderParameters::Bind(const FShaderParameterMap& ParameterMap)
{
	// only used if Material has an expression that requires SceneColorTexture
	SceneColorTextureParameter.Bind(ParameterMap,TEXT("SceneColorTexture"));
	SceneColorTextureParameterSampler.Bind(ParameterMap,TEXT("SceneColorTextureSampler"));
	// only used if Material has an expression that requires SceneDepthTexture
	SceneDepthTextureParameter.Bind(ParameterMap,TEXT("SceneDepthTexture"));
	SceneDepthTextureParameterSampler.Bind(ParameterMap,TEXT("SceneDepthTextureSampler"));
	// Only used if Material has an expression that requires SceneAlphaCopyTexture
	SceneAlphaCopyTextureParameter.Bind(ParameterMap,TEXT("SceneAlphaCopyTexture"));
	SceneAlphaCopyTextureParameterSampler.Bind(ParameterMap,TEXT("SceneAlphaCopyTextureSampler"));
	//
	SceneDepthTextureNonMS.Bind(ParameterMap,TEXT("SceneDepthTextureNonMS"));
	SceneColorSurfaceParameter.Bind(ParameterMap,TEXT("SceneColorSurface"));
	// only used if Material has an expression that requires SceneColorTextureMSAA
	SceneDepthSurfaceParameter.Bind(ParameterMap,TEXT("SceneDepthSurface"));
}

FArchive& operator<<(FArchive& Ar,FSceneTextureShaderParameters& Parameters)
{
	Ar << Parameters.SceneColorTextureParameter;
	Ar << Parameters.SceneColorTextureParameterSampler;
	Ar << Parameters.SceneAlphaCopyTextureParameter;
	Ar << Parameters.SceneAlphaCopyTextureParameterSampler;
	Ar << Parameters.SceneColorSurfaceParameter;
	Ar << Parameters.SceneDepthTextureParameter;
	Ar << Parameters.SceneDepthTextureParameterSampler;
	Ar << Parameters.SceneDepthSurfaceParameter;
	Ar << Parameters.SceneDepthTextureNonMS;
	return Ar;
}

// Note this is not just for Deferred rendering, it also applies to mobile forward rendering.
void FDeferredPixelShaderParameters::Bind(const FShaderParameterMap& ParameterMap)
{
	SceneTextureParameters.Bind(ParameterMap);
	GBufferATextureMS.Bind(ParameterMap,TEXT("GBufferATextureMS"));
	GBufferBTextureMS.Bind(ParameterMap,TEXT("GBufferBTextureMS"));
	GBufferCTextureMS.Bind(ParameterMap,TEXT("GBufferCTextureMS"));
	GBufferDTextureMS.Bind(ParameterMap,TEXT("GBufferDTextureMS"));
	GBufferETextureMS.Bind(ParameterMap,TEXT("GBufferETextureMS"));
	DBufferATextureMS.Bind(ParameterMap,TEXT("DBufferATextureMS"));
	DBufferBTextureMS.Bind(ParameterMap,TEXT("DBufferBTextureMS"));
	DBufferCTextureMS.Bind(ParameterMap,TEXT("DBufferCTextureMS"));
	ScreenSpaceAOTextureMS.Bind(ParameterMap,TEXT("ScreenSpaceAOTextureMS"));
	GBufferATextureNonMS.Bind(ParameterMap,TEXT("GBufferATextureNonMS"));
	GBufferBTextureNonMS.Bind(ParameterMap,TEXT("GBufferBTextureNonMS"));
	GBufferCTextureNonMS.Bind(ParameterMap,TEXT("GBufferCTextureNonMS"));
	GBufferDTextureNonMS.Bind(ParameterMap,TEXT("GBufferDTextureNonMS"));
	GBufferETextureNonMS.Bind(ParameterMap,TEXT("GBufferETextureNonMS"));
	DBufferATextureNonMS.Bind(ParameterMap,TEXT("DBufferATextureNonMS"));
	DBufferBTextureNonMS.Bind(ParameterMap,TEXT("DBufferBTextureNonMS"));
	DBufferCTextureNonMS.Bind(ParameterMap,TEXT("DBufferCTextureNonMS"));
	ScreenSpaceAOTextureNonMS.Bind(ParameterMap,TEXT("ScreenSpaceAOTextureNonMS"));
	CustomDepthTextureNonMS.Bind(ParameterMap,TEXT("CustomDepthTextureNonMS"));
	GBufferATexture.Bind(ParameterMap,TEXT("GBufferATexture"));
	GBufferATextureSampler.Bind(ParameterMap,TEXT("GBufferATextureSampler"));
	GBufferBTexture.Bind(ParameterMap,TEXT("GBufferBTexture"));
	GBufferBTextureSampler.Bind(ParameterMap,TEXT("GBufferBTextureSampler"));
	GBufferCTexture.Bind(ParameterMap,TEXT("GBufferCTexture"));
	GBufferCTextureSampler.Bind(ParameterMap,TEXT("GBufferCTextureSampler"));
	GBufferDTexture.Bind(ParameterMap,TEXT("GBufferDTexture"));
	GBufferDTextureSampler.Bind(ParameterMap,TEXT("GBufferDTextureSampler"));
	GBufferETexture.Bind(ParameterMap,TEXT("GBufferETexture"));
	GBufferETextureSampler.Bind(ParameterMap,TEXT("GBufferETextureSampler"));
	DBufferATexture.Bind(ParameterMap,TEXT("DBufferATexture"));
	DBufferATextureSampler.Bind(ParameterMap,TEXT("DBufferATextureSampler"));
	DBufferBTexture.Bind(ParameterMap,TEXT("DBufferBTexture"));
	DBufferBTextureSampler.Bind(ParameterMap,TEXT("DBufferBTextureSampler"));
	DBufferCTexture.Bind(ParameterMap,TEXT("DBufferCTexture"));
	DBufferCTextureSampler.Bind(ParameterMap,TEXT("DBufferCTextureSampler"));
	ScreenSpaceAOTexture.Bind(ParameterMap,TEXT("ScreenSpaceAOTexture"));
	ScreenSpaceAOTextureSampler.Bind(ParameterMap,TEXT("ScreenSpaceAOTextureSampler"));
	CustomDepthTexture.Bind(ParameterMap,TEXT("CustomDepthTexture"));
	CustomDepthTextureSampler.Bind(ParameterMap,TEXT("CustomDepthTextureSampler"));
}

FArchive& operator<<(FArchive& Ar,FDeferredPixelShaderParameters& Parameters)
{
	Ar << Parameters.SceneTextureParameters;
	Ar << Parameters.GBufferATextureMS;
	Ar << Parameters.GBufferBTextureMS;
	Ar << Parameters.GBufferCTextureMS;
	Ar << Parameters.GBufferDTextureMS;
	Ar << Parameters.GBufferETextureMS;
	Ar << Parameters.DBufferATextureMS;
	Ar << Parameters.DBufferBTextureMS;
	Ar << Parameters.DBufferCTextureMS;
	Ar << Parameters.ScreenSpaceAOTextureMS;
	Ar << Parameters.GBufferATextureNonMS;
	Ar << Parameters.GBufferBTextureNonMS;
	Ar << Parameters.GBufferCTextureNonMS;
	Ar << Parameters.GBufferDTextureNonMS;
	Ar << Parameters.GBufferETextureNonMS;
	Ar << Parameters.DBufferATextureNonMS;
	Ar << Parameters.DBufferBTextureNonMS;
	Ar << Parameters.DBufferCTextureNonMS;
	Ar << Parameters.ScreenSpaceAOTextureNonMS;
	Ar << Parameters.CustomDepthTextureNonMS;
	Ar << Parameters.GBufferATexture;
	Ar << Parameters.GBufferATextureSampler;
	Ar << Parameters.GBufferBTexture;
	Ar << Parameters.GBufferBTextureSampler;
	Ar << Parameters.GBufferCTexture;
	Ar << Parameters.GBufferCTextureSampler;
	Ar << Parameters.GBufferDTexture;
	Ar << Parameters.GBufferDTextureSampler;
	Ar << Parameters.GBufferETexture;
	Ar << Parameters.GBufferETextureSampler;
	Ar << Parameters.DBufferATexture;
	Ar << Parameters.DBufferATextureSampler;
	Ar << Parameters.DBufferBTexture;
	Ar << Parameters.DBufferBTextureSampler;
	Ar << Parameters.DBufferCTexture;
	Ar << Parameters.DBufferCTextureSampler;
	Ar << Parameters.ScreenSpaceAOTexture;
	Ar << Parameters.ScreenSpaceAOTextureSampler;
	Ar << Parameters.CustomDepthTexture;
	Ar << Parameters.CustomDepthTextureSampler;

	return Ar;
}
