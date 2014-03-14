// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SceneRendering.cpp: Scene rendering.
=============================================================================*/

#include "RendererPrivate.h"
#include "EnginePrivate.h"
#include "ScenePrivate.h"
#include "ScreenRendering.h"
#include "SceneFilterRendering.h"
#include "VisualizeTexture.h"
#include "PostProcessEyeAdaptation.h"
#include "CompositionLighting.h"
#include "FXSystem.h"
#include "SceneViewExtension.h"
#include "PostProcessBusyWait.h"

/*-----------------------------------------------------------------------------
	Globals
-----------------------------------------------------------------------------*/

extern ENGINE_API FLightMap2D* GDebugSelectedLightmap;
extern ENGINE_API UPrimitiveComponent* GDebugSelectedComponent;

/**
 * Console variable controlling whether or not occlusion queries are allowed.
 */
static TAutoConsoleVariable<int32> CVarAllowOcclusionQueries(
	TEXT("r.AllowOcclusionQueries"),
	1,
	TEXT("If zero, occlusion queries will not be used to cull primitives."),
	ECVF_RenderThreadSafe
	);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
static TAutoConsoleVariable<float> CVarGeneralPurposeTweak(
	TEXT("r.GeneralPurposeTweak"),
	1.0f,
	TEXT("Useful for low level shader development to get quick iteration time without having to change any c++ code.\n")
	TEXT("Value maps to View.GeneralPurposeTweak inside the shaders.\n")
	TEXT("Example usage: Multiplier on some value to tweak, toggle to switch between different algorithms (Default: 1.0)\n")
	TEXT("DON'T USE THIS FOR ANYTHING THAT IS CHECKED IN. Compiled out in SHIPPING to make cheating a bit harder."),
	ECVF_RenderThreadSafe);
#endif

/**
 * Console variable controlling the maximum number of shadow cascades to render with.
 *   DO NOT READ ON THE RENDERING THREAD. Use FSceneView::MaxShadowCascades.
 */
static TAutoConsoleVariable<int32> CVarMaxShadowCascades(
	TEXT("r.Shadow.CSM.MaxCascades"),
	10,
	TEXT("The maximum number of cascades with which to render dynamic directional light shadows."),
	ECVF_Scalability | ECVF_RenderThreadSafe
	);

static TAutoConsoleVariable<float> CVarTessellationAdaptivePixelsPerTriangle(
	TEXT("r.TessellationAdaptivePixelsPerTriangle"),
	48.0f,
	TEXT("Global tessellation factor multiplier"),
	ECVF_RenderThreadSafe);

/*-----------------------------------------------------------------------------
	FViewInfo
-----------------------------------------------------------------------------*/

/** 
 * Initialization constructor. Passes all parameters to FSceneView constructor
 */
FViewInfo::FViewInfo(const FSceneViewInitOptions& InitOptions)
	:	FSceneView(InitOptions)
	,	bPrevTransformsReset( false )
	,	bIgnoreExistingQueries( false )
	,	bDisableQuerySubmissions( false )
	,	bDisableDistanceBasedFadeTransitions( false )
	,	NumVisibleStaticMeshElements(0)
	,	PrecomputedVisibilityData(NULL)
	,	IndividualOcclusionQueries((FSceneViewState*)InitOptions.SceneViewStateInterface, 1)
	,	GroupedOcclusionQueries((FSceneViewState*)InitOptions.SceneViewStateInterface, FOcclusionQueryBatcher::OccludedPrimitiveQueryBatchSize)
{
	Init();
}

/** 
 * Initialization constructor. 
 * @param InView - copy to init with
 */
FViewInfo::FViewInfo(const FSceneView* InView)
	:	FSceneView(*InView)
	,	bHasTranslucentViewMeshElements( 0 )
	,	bPrevTransformsReset( false )
	,	bIgnoreExistingQueries( false )
	,	bDisableQuerySubmissions( false )
	,	bDisableDistanceBasedFadeTransitions( false )
	,	NumVisibleStaticMeshElements(0)
	,	PrecomputedVisibilityData(NULL)
	,	IndividualOcclusionQueries((FSceneViewState*)InView->State,1)
	,	GroupedOcclusionQueries((FSceneViewState*)InView->State,FOcclusionQueryBatcher::OccludedPrimitiveQueryBatchSize)
{
	Init();
}

void FViewInfo::Init()
{
	bIsViewInfo = true;
	PrevViewProjMatrix.SetIdentity();
	PrevViewRotationProjMatrix.SetIdentity();
	
	ExponentialFogParameters = FVector4(0,1,1,0);
	ExponentialFogColor = FVector::ZeroVector;
	FogMaxOpacity = 1;

	bUseDirectionalInscattering = false;
	DirectionalInscatteringExponent = 0;
	DirectionalInscatteringStartDistance = 0;
	InscatteringLightDirection = FVector(0);
	DirectionalInscatteringColor = FLinearColor(ForceInit);

	for (int32 CascadeIndex = 0; CascadeIndex < TVC_MAX; CascadeIndex++)
	{
		TranslucencyLightingVolumeMin[CascadeIndex] = FVector(0);
		TranslucencyVolumeVoxelSize[CascadeIndex] = 0;
		TranslucencyLightingVolumeSize[CascadeIndex] = FVector(0);
	}

	MaxShadowCascades = FMath::Clamp<int32>(CVarMaxShadowCascades.GetValueOnAnyThread(), 1, 10);
}

FViewInfo::~FViewInfo()
{
	for(int32 ResourceIndex = 0;ResourceIndex < DynamicResources.Num();ResourceIndex++)
	{
		DynamicResources[ResourceIndex]->ReleasePrimitiveResource();
	}
}

void FViewInfo::SetupSkyIrradianceEnvironmentMapConstants(FVector4* OutSkyIrradianceEnvironmentMap) const
{
	FScene* Scene = (FScene*)Family->Scene;

	if (Scene 
		&& Scene->SkyLight 
		&& Family->EngineShowFlags.SkyLighting)
	{
		const FSHVectorRGB3& SkyIrradiance = Scene->SkyLight->IrradianceEnvironmentMap;

		const float SqrtPI = FMath::Sqrt(PI);
		const float Coefficient0 = 1.0f / (2 * SqrtPI);
		const float Coefficient1 = FMath::Sqrt(3) / (3 * SqrtPI);
		const float Coefficient2 = FMath::Sqrt(15) / (8 * SqrtPI);
		const float Coefficient3 = FMath::Sqrt(5) / (16 * SqrtPI);
		const float Coefficient4 = .5f * Coefficient2;

		// Pack the SH coefficients in a way that makes applying the lighting use the least shader instructions
		// This has the diffuse convolution coefficients baked in
		// See "Stupid Spherical Harmonics (SH) Tricks"
		OutSkyIrradianceEnvironmentMap[0].X = -Coefficient1 * SkyIrradiance.R.V[3];
		OutSkyIrradianceEnvironmentMap[0].Y = -Coefficient1 * SkyIrradiance.R.V[1];
		OutSkyIrradianceEnvironmentMap[0].Z = Coefficient1 * SkyIrradiance.R.V[2];
		OutSkyIrradianceEnvironmentMap[0].W = Coefficient0 * SkyIrradiance.R.V[0] - Coefficient3 * SkyIrradiance.R.V[6];

		OutSkyIrradianceEnvironmentMap[1].X = -Coefficient1 * SkyIrradiance.G.V[3];
		OutSkyIrradianceEnvironmentMap[1].Y = -Coefficient1 * SkyIrradiance.G.V[1];
		OutSkyIrradianceEnvironmentMap[1].Z = Coefficient1 * SkyIrradiance.G.V[2];
		OutSkyIrradianceEnvironmentMap[1].W = Coefficient0 * SkyIrradiance.G.V[0] - Coefficient3 * SkyIrradiance.G.V[6];

		OutSkyIrradianceEnvironmentMap[2].X = -Coefficient1 * SkyIrradiance.B.V[3];
		OutSkyIrradianceEnvironmentMap[2].Y = -Coefficient1 * SkyIrradiance.B.V[1];
		OutSkyIrradianceEnvironmentMap[2].Z = Coefficient1 * SkyIrradiance.B.V[2];
		OutSkyIrradianceEnvironmentMap[2].W = Coefficient0 * SkyIrradiance.B.V[0] - Coefficient3 * SkyIrradiance.B.V[6];

		OutSkyIrradianceEnvironmentMap[3].X = Coefficient2 * SkyIrradiance.R.V[4];
		OutSkyIrradianceEnvironmentMap[3].Y = -Coefficient2 * SkyIrradiance.R.V[5];
		OutSkyIrradianceEnvironmentMap[3].Z = 3 * Coefficient3 * SkyIrradiance.R.V[6];
		OutSkyIrradianceEnvironmentMap[3].W = -Coefficient2 * SkyIrradiance.R.V[7];

		OutSkyIrradianceEnvironmentMap[4].X = Coefficient2 * SkyIrradiance.G.V[4];
		OutSkyIrradianceEnvironmentMap[4].Y = -Coefficient2 * SkyIrradiance.G.V[5];
		OutSkyIrradianceEnvironmentMap[4].Z = 3 * Coefficient3 * SkyIrradiance.G.V[6];
		OutSkyIrradianceEnvironmentMap[4].W = -Coefficient2 * SkyIrradiance.G.V[7];

		OutSkyIrradianceEnvironmentMap[5].X = Coefficient2 * SkyIrradiance.B.V[4];
		OutSkyIrradianceEnvironmentMap[5].Y = -Coefficient2 * SkyIrradiance.B.V[5];
		OutSkyIrradianceEnvironmentMap[5].Z = 3 * Coefficient3 * SkyIrradiance.B.V[6];
		OutSkyIrradianceEnvironmentMap[5].W = -Coefficient2 * SkyIrradiance.B.V[7];

		OutSkyIrradianceEnvironmentMap[6].X = Coefficient4 * SkyIrradiance.R.V[8];
		OutSkyIrradianceEnvironmentMap[6].Y = Coefficient4 * SkyIrradiance.G.V[8];
		OutSkyIrradianceEnvironmentMap[6].Z = Coefficient4 * SkyIrradiance.B.V[8];
		OutSkyIrradianceEnvironmentMap[6].W = 1;
	}
	else
	{
		FMemory::Memzero(OutSkyIrradianceEnvironmentMap, sizeof(FVector4) * 7);
	}
}

/** Creates the view's uniform buffer given a set of view transforms. */
TUniformBufferRef<FViewUniformShaderParameters> FViewInfo::CreateUniformBuffer(
	const FMatrix& EffectiveTranslatedViewMatrix, 
	FBox* OutTranslucentCascadeBoundsArray, 
	int32 NumTranslucentCascades) const
{
	check(Family);

	// Calculate the vector used by shaders to convert clip space coordinates to texture space.
	const FIntPoint BufferSize = GSceneRenderTargets.GetBufferSizeXY();
	const float InvBufferSizeX = 1.0f / BufferSize.X;
	const float InvBufferSizeY = 1.0f / BufferSize.Y;
	const FVector4 ScreenPositionScaleBias(
		ViewRect.Width() * InvBufferSizeX / +2.0f,
		ViewRect.Height() * InvBufferSizeY / (-2.0f * GProjectionSignY),
		(ViewRect.Height() / 2.0f + GPixelCenterOffset + ViewRect.Min.Y) * InvBufferSizeY,
		(ViewRect.Width() / 2.0f + GPixelCenterOffset + ViewRect.Min.X) * InvBufferSizeX
		);
	
	const bool bIsUnlitView = !Family->EngineShowFlags.Lighting;

	// Create the view's uniform buffer.
	FViewUniformShaderParameters ViewUniformShaderParameters;
	ViewUniformShaderParameters.TranslatedWorldToClip = ViewMatrices.TranslatedViewProjectionMatrix;
	ViewUniformShaderParameters.WorldToClip = ViewProjectionMatrix;
	ViewUniformShaderParameters.TranslatedWorldToView = EffectiveTranslatedViewMatrix;
	ViewUniformShaderParameters.ViewToTranslatedWorld = InvViewMatrix * FTranslationMatrix(ViewMatrices.PreViewTranslation);
	ViewUniformShaderParameters.ViewToClip = ViewMatrices.ProjMatrix;
	ViewUniformShaderParameters.ClipToTranslatedWorld = ViewMatrices.InvTranslatedViewProjectionMatrix;
	ViewUniformShaderParameters.ViewForward = EffectiveTranslatedViewMatrix.GetColumn(2);
	ViewUniformShaderParameters.ViewUp = EffectiveTranslatedViewMatrix.GetColumn(1);
	ViewUniformShaderParameters.ViewRight = EffectiveTranslatedViewMatrix.GetColumn(0);
	ViewUniformShaderParameters.InvDeviceZToWorldZTransform = InvDeviceZToWorldZTransform;
	ViewUniformShaderParameters.ScreenPositionScaleBias = ScreenPositionScaleBias;
	ViewUniformShaderParameters.ScreenTexelBias = FVector4(ViewRect.Min.X, ViewRect.Min.Y, 0.0f, 0.0f);
	ViewUniformShaderParameters.ViewSizeAndSceneTexelSize = FVector4(ViewRect.Width(), ViewRect.Height(), InvBufferSizeX, InvBufferSizeY);
	ViewUniformShaderParameters.ViewOrigin = ViewMatrices.ViewOrigin;
	ViewUniformShaderParameters.TranslatedViewOrigin = ViewMatrices.ViewOrigin + ViewMatrices.PreViewTranslation;
	ViewUniformShaderParameters.DiffuseOverrideParameter = DiffuseOverrideParameter;
	ViewUniformShaderParameters.SpecularOverrideParameter = SpecularOverrideParameter;
	ViewUniformShaderParameters.NormalOverrideParameter = NormalOverrideParameter;
	ViewUniformShaderParameters.RoughnessOverrideParameter = RoughnessOverrideParameter;
	ViewUniformShaderParameters.PreViewTranslation = ViewMatrices.PreViewTranslation;
	ViewUniformShaderParameters.ViewOriginDelta = ViewMatrices.ViewOrigin - PrevViewMatrices.ViewOrigin;
	ViewUniformShaderParameters.CullingSign = bReverseCulling ? -1.0f : 1.0f;
	ViewUniformShaderParameters.NearPlane = GNearClippingPlane;
	ViewUniformShaderParameters.PrevProjection = PrevViewMatrices.ProjMatrix;
	ViewUniformShaderParameters.PrevViewProj = PrevViewProjMatrix;
	ViewUniformShaderParameters.PrevViewRotationProj = PrevViewRotationProjMatrix;
	ViewUniformShaderParameters.PrevTranslatedWorldToClip = PrevViewMatrices.TranslatedViewProjectionMatrix;
	ViewUniformShaderParameters.PrevViewOrigin = PrevViewMatrices.ViewOrigin;
	ViewUniformShaderParameters.PrevPreViewTranslation = PrevViewMatrices.PreViewTranslation;
	// can be optimized
	ViewUniformShaderParameters.PrevInvViewProj = PrevViewProjMatrix.InverseSafe();

	ViewUniformShaderParameters.ScreenToWorld = FMatrix(
		FPlane(1,0,0,0),
		FPlane(0,1,0,0),
		FPlane(0,0,ProjectionMatrixUnadjustedForRHI.M[2][2],1),
		FPlane(0,0,ProjectionMatrixUnadjustedForRHI.M[3][2],0))
		* InvViewProjectionMatrix;

	ViewUniformShaderParameters.ScreenToTranslatedWorld = FMatrix(
		FPlane(1,0,0,0),
		FPlane(0,1,0,0),
		FPlane(0,0,ProjectionMatrixUnadjustedForRHI.M[2][2],1),
		FPlane(0,0,ProjectionMatrixUnadjustedForRHI.M[3][2],0))
		* ViewMatrices.InvTranslatedViewProjectionMatrix;

	ViewUniformShaderParameters.PrevScreenToTranslatedWorld = FMatrix(
		FPlane(1,0,0,0),
		FPlane(0,1,0,0),
		FPlane(0,0,ProjectionMatrixUnadjustedForRHI.M[2][2],1),
		FPlane(0,0,ProjectionMatrixUnadjustedForRHI.M[3][2],0))
		* PrevViewMatrices.InvTranslatedViewProjectionMatrix;

	// is getting clamped in the shader to a value larger than 0 (we don't want the triangles to disappear)
	ViewUniformShaderParameters.AdaptiveTessellationFactor = 0.0f;

	if(Family->EngineShowFlags.Tessellation)
	{
		// CVar setting is pixels/tri which is nice and intuitive.  But we want pixels/tessellated edge.  So use a heuristic.
		float TessellationAdaptivePixelsPerEdge = FMath::Sqrt(2.f * CVarTessellationAdaptivePixelsPerTriangle.GetValueOnRenderThread());

		ViewUniformShaderParameters.AdaptiveTessellationFactor = 0.5f * float(ViewRect.Height()) / TessellationAdaptivePixelsPerEdge;
	}

	if (Family->Scene)
	{
		FScene* Scene = (FScene*)Family->Scene;
		ViewUniformShaderParameters.DirectionalLightColor = Scene->SimpleDirectionalLight ? Scene->SimpleDirectionalLight->Proxy->GetColor() / PI : FLinearColor::Black;
		ViewUniformShaderParameters.DirectionalLightDirection = Scene->SimpleDirectionalLight ? -Scene->SimpleDirectionalLight->Proxy->GetDirection() : FVector::ZeroVector;
		ViewUniformShaderParameters.UpperSkyColor = Scene->UpperDynamicSkylightColor;
		ViewUniformShaderParameters.LowerSkyColor = Scene->LowerDynamicSkylightColor;

		// Atmospheric fog parameters
		if (ShouldRenderAtmosphere(*Family) && Scene->AtmosphericFog)
		{
			ViewUniformShaderParameters.AtmosphericFogSunPower = Scene->AtmosphericFog->SunMultiplier;
			ViewUniformShaderParameters.AtmosphericFogPower = Scene->AtmosphericFog->FogMultiplier;
			ViewUniformShaderParameters.AtmosphericFogDensityScale = Scene->AtmosphericFog->InvDensityMultiplier;
			ViewUniformShaderParameters.AtmosphericFogDensityOffset = Scene->AtmosphericFog->DensityOffset;
			ViewUniformShaderParameters.AtmosphericFogGroundOffset = Scene->AtmosphericFog->GroundOffset;
			ViewUniformShaderParameters.AtmosphericFogDistanceScale = Scene->AtmosphericFog->DistanceScale;
			ViewUniformShaderParameters.AtmosphericFogAltitudeScale = Scene->AtmosphericFog->AltitudeScale;
			ViewUniformShaderParameters.AtmosphericFogHeightScaleRayleigh =  Scene->AtmosphericFog->RHeight;
			ViewUniformShaderParameters.AtmosphericFogStartDistance = Scene->AtmosphericFog->StartDistance;
			ViewUniformShaderParameters.AtmosphericFogDistanceOffset = Scene->AtmosphericFog->DistanceOffset;
			ViewUniformShaderParameters.AtmosphericFogSunColor = Scene->SunLight ? Scene->SunLight->Proxy->GetColor() : Scene->AtmosphericFog->DefaultSunColor;
			ViewUniformShaderParameters.AtmosphericFogSunDirection = Scene->SunLight ? -Scene->SunLight->Proxy->GetDirection() : -Scene->AtmosphericFog->DefaultSunDirection;
			ViewUniformShaderParameters.AtmosphericFogRenderMask = Scene->AtmosphericFog->RenderFlag & (EAtmosphereRenderFlag::E_DisableGroundScattering | EAtmosphereRenderFlag::E_DisableSunDisk);
			ViewUniformShaderParameters.AtmosphericFogInscatterAltitudeSampleNum = Scene->AtmosphericFog->InscatterAltitudeSampleNum;
		}
		else
		{
			ViewUniformShaderParameters.AtmosphericFogSunPower = 0.f;
			ViewUniformShaderParameters.AtmosphericFogPower = 0.f;
			ViewUniformShaderParameters.AtmosphericFogDensityScale = 0.f;
			ViewUniformShaderParameters.AtmosphericFogDensityOffset = 0.f;
			ViewUniformShaderParameters.AtmosphericFogGroundOffset = 0.f;
			ViewUniformShaderParameters.AtmosphericFogDistanceScale = 0.f;
			ViewUniformShaderParameters.AtmosphericFogAltitudeScale = 0.f;
			ViewUniformShaderParameters.AtmosphericFogHeightScaleRayleigh =  0.f;
			ViewUniformShaderParameters.AtmosphericFogStartDistance = 0.f;
			ViewUniformShaderParameters.AtmosphericFogDistanceOffset = 0.f;
			ViewUniformShaderParameters.AtmosphericFogSunColor = FLinearColor::Black;
			ViewUniformShaderParameters.AtmosphericFogSunDirection = FVector::ZeroVector;
			ViewUniformShaderParameters.AtmosphericFogRenderMask = EAtmosphereRenderFlag::E_EnableAll;
			ViewUniformShaderParameters.AtmosphericFogInscatterAltitudeSampleNum = 0;
		}
	}
	else
	{
		ViewUniformShaderParameters.DirectionalLightDirection = FVector::ZeroVector;
		ViewUniformShaderParameters.DirectionalLightColor = FLinearColor::Black;
		ViewUniformShaderParameters.UpperSkyColor = FLinearColor::Black;
		ViewUniformShaderParameters.LowerSkyColor = FLinearColor::Black;

		// Atmospheric fog parameters
		ViewUniformShaderParameters.AtmosphericFogSunPower = 0.f;
		ViewUniformShaderParameters.AtmosphericFogPower = 0.f;
		ViewUniformShaderParameters.AtmosphericFogDensityScale = 0.f;
		ViewUniformShaderParameters.AtmosphericFogDensityOffset = 0.f;
		ViewUniformShaderParameters.AtmosphericFogGroundOffset = 0.f;
		ViewUniformShaderParameters.AtmosphericFogDistanceScale = 0.f;
		ViewUniformShaderParameters.AtmosphericFogAltitudeScale = 0.f;
		ViewUniformShaderParameters.AtmosphericFogHeightScaleRayleigh =  0.f;
		ViewUniformShaderParameters.AtmosphericFogStartDistance = 0.f;
		ViewUniformShaderParameters.AtmosphericFogDistanceOffset = 0.f;
		ViewUniformShaderParameters.AtmosphericFogSunColor = FLinearColor::Black;
		ViewUniformShaderParameters.AtmosphericFogSunDirection = FVector::ZeroVector;
		ViewUniformShaderParameters.AtmosphericFogRenderMask = EAtmosphereRenderFlag::E_EnableAll;
		ViewUniformShaderParameters.AtmosphericFogInscatterAltitudeSampleNum = 0;
	}

	ViewUniformShaderParameters.UnlitViewmodeMask = bIsUnlitView ? 1 : 0;
	ViewUniformShaderParameters.ReflectionLightmapMixingMask = Family->EngineShowFlags.ReflectionEnvironmentLightmapMixing ? 1 : 0;
	ViewUniformShaderParameters.OutOfBoundsMask = Family->EngineShowFlags.VisualizeOutOfBoundsPixels ? 1 : 0;

	ViewUniformShaderParameters.GameTime = Family->CurrentWorldTime;
	ViewUniformShaderParameters.RealTime = Family->CurrentRealTime;
	ViewUniformShaderParameters.Random = FMath::Rand();
	ViewUniformShaderParameters.FrameNumber = FrameNumber;

	if(State)
	{
		// safe to cast on the renderer side
		FSceneViewState* ViewState = (FSceneViewState*)State;
		ViewUniformShaderParameters.TemporalAAParams = FVector4(
			ViewState->GetCurrentTemporalAASampleIndex(), 
			ViewState->GetCurrentTemporalAASampleCount(),
			TemporalJitterPixelsX,
			TemporalJitterPixelsY);
	}
	else
	{
		ViewUniformShaderParameters.TemporalAAParams = FVector4(0, 1, 0, 0);
	}

	{
		// If rendering in stereo, the right eye uses the left eye's translucency lighting volume.
		const FViewInfo* PrimaryView = this;
		if (StereoPass == eSSP_RIGHT_EYE)
		{
			int32 ViewIndex = Family->Views.Find(this);
			if (Family->Views.IsValidIndex(ViewIndex) && Family->Views.IsValidIndex(ViewIndex - 1))
			{
				const FSceneView* LeftEyeView = Family->Views[ViewIndex - 1];
				if (LeftEyeView->bIsViewInfo && LeftEyeView->StereoPass == eSSP_LEFT_EYE)
				{
					PrimaryView = static_cast<const FViewInfo*>(LeftEyeView);
				}
			}
		}
		PrimaryView->CalcTranslucencyLightingVolumeBounds(OutTranslucentCascadeBoundsArray, NumTranslucentCascades);
	}

	for (int32 CascadeIndex = 0; CascadeIndex < NumTranslucentCascades; CascadeIndex++)
	{
		const float VolumeVoxelSize = (OutTranslucentCascadeBoundsArray[CascadeIndex].Max.X - OutTranslucentCascadeBoundsArray[CascadeIndex].Min.X) / GTranslucencyLightingVolumeDim;
		const FVector VolumeSize = OutTranslucentCascadeBoundsArray[CascadeIndex].Max - OutTranslucentCascadeBoundsArray[CascadeIndex].Min;
		ViewUniformShaderParameters.TranslucencyLightingVolumeMin[CascadeIndex] = FVector4(OutTranslucentCascadeBoundsArray[CascadeIndex].Min, 1.0f / GTranslucencyLightingVolumeDim);
		ViewUniformShaderParameters.TranslucencyLightingVolumeInvSize[CascadeIndex] = FVector4(FVector(1.0f) / VolumeSize, VolumeVoxelSize);
	}
	
	ViewUniformShaderParameters.RenderTargetSize = BufferSize;
	ViewUniformShaderParameters.ExposureScale = FRCPassPostProcessEyeAdaptation::ComputeExposureScaleValue( *this );
	ViewUniformShaderParameters.DepthOfFieldFocalDistance = FinalPostProcessSettings.DepthOfFieldFocalDistance;
	ViewUniformShaderParameters.DepthOfFieldFocalRegion = FinalPostProcessSettings.DepthOfFieldFocalRegion;
	ViewUniformShaderParameters.DepthOfFieldNearTransitionRegion = FinalPostProcessSettings.DepthOfFieldNearTransitionRegion;
	ViewUniformShaderParameters.DepthOfFieldFarTransitionRegion = FinalPostProcessSettings.DepthOfFieldFarTransitionRegion;
	ViewUniformShaderParameters.DepthOfFieldScale = FinalPostProcessSettings.DepthOfFieldScale;
	ViewUniformShaderParameters.DepthOfFieldFocalLength = 50.0f;
	ViewUniformShaderParameters.MotionBlurNormalizedToPixel = FinalPostProcessSettings.MotionBlurMax * ViewRect.Width() / 100.0f;

	{
		// This is the CVar default
		float Value = 1.0f;

		// Compiled out in SHIPPING to make cheating a bit harder.
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		Value = CVarGeneralPurposeTweak.GetValueOnRenderThread();
#endif

		ViewUniformShaderParameters.GeneralPurposeTweak = Value;
	}

	ViewUniformShaderParameters.DemosaicVposOffset = 0.0f;
	{
		static auto* DemosaicVposOffsetCvar = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.DemosaicVposOffset"));
		if (DemosaicVposOffsetCvar)
		{
			ViewUniformShaderParameters.DemosaicVposOffset = DemosaicVposOffsetCvar->GetValueOnRenderThread();
		}
	}

	ViewUniformShaderParameters.IndirectLightingColorScale = FVector(FinalPostProcessSettings.IndirectLightingColor.R * FinalPostProcessSettings.IndirectLightingIntensity,
		FinalPostProcessSettings.IndirectLightingColor.G * FinalPostProcessSettings.IndirectLightingIntensity,
		FinalPostProcessSettings.IndirectLightingColor.B * FinalPostProcessSettings.IndirectLightingIntensity);

	ViewUniformShaderParameters.AmbientCubemapTint = FinalPostProcessSettings.AmbientCubemapTint;
	ViewUniformShaderParameters.AmbientCubemapIntensity = FinalPostProcessSettings.AmbientCubemapIntensity;

	{
		// Enables toggle of HDR Mosaic mode without recompile of all PC shaders during ES2 emulation.
		ViewUniformShaderParameters.HdrMosaic = IsMobileHDR32bpp() ? 1.0f : 0.0f;
	}
	
	FVector2D OneScenePixelUVSize = FVector2D(1.0f / BufferSize.X, 1.0f / BufferSize.Y);
	FVector4 SceneTexMinMax(	((float)ViewRect.Min.X / BufferSize.X), 
								((float)ViewRect.Min.Y / BufferSize.Y), 
								(((float)ViewRect.Max.X / BufferSize.X) - OneScenePixelUVSize.X) , 
								(((float)ViewRect.Max.Y / BufferSize.Y) - OneScenePixelUVSize.Y) );
	ViewUniformShaderParameters.SceneTextureMinMax = SceneTexMinMax;

	if (Family->Scene && ((FScene*)Family->Scene)->SkyLight)
	{
		FScene* Scene = (FScene*)Family->Scene;
		ViewUniformShaderParameters.SkyLightColor = Scene->SkyLight->LightColor;
		ViewUniformShaderParameters.SkyLightParameters = Scene->SkyLight->bCastShadows && Scene->SkyLight->bPrecomputedLightingIsValid ? 1 : 0;
	}
	else
	{
		ViewUniformShaderParameters.SkyLightColor = FLinearColor::Black;
		ViewUniformShaderParameters.SkyLightParameters = 0;
	}

	// Make sure there's no padding since we're going to cast to FVector4*
	checkSlow(sizeof(ViewUniformShaderParameters.SkyIrradianceEnvironmentMap) == sizeof(FVector4) * 7);
	SetupSkyIrradianceEnvironmentMapConstants((FVector4*)&ViewUniformShaderParameters.SkyIrradianceEnvironmentMap);

	return TUniformBufferRef<FViewUniformShaderParameters>::CreateUniformBufferImmediate(ViewUniformShaderParameters, UniformBuffer_SingleUse);
}

void FViewInfo::InitRHIResources()
{
	FBox VolumeBounds[TVC_MAX];

	/** The view transform, starting from world-space points translated by -ViewOrigin. */
	FMatrix TranslatedViewMatrix = FTranslationMatrix(-ViewMatrices.PreViewTranslation) * ViewMatrices.ViewMatrix;

	UniformBuffer = CreateUniformBuffer(
		TranslatedViewMatrix,
		VolumeBounds,
		TVC_MAX);

	for (int32 CascadeIndex = 0; CascadeIndex < TVC_MAX; CascadeIndex++)
	{
		TranslucencyLightingVolumeMin[CascadeIndex] = VolumeBounds[CascadeIndex].Min;
		TranslucencyVolumeVoxelSize[CascadeIndex] = (VolumeBounds[CascadeIndex].Max.X - VolumeBounds[CascadeIndex].Min.X) / GTranslucencyLightingVolumeDim;
		TranslucencyLightingVolumeSize[CascadeIndex] = VolumeBounds[CascadeIndex].Max - VolumeBounds[CascadeIndex].Min;
	}

	// Initialize the dynamic resources used by the view's FViewElementDrawer.
	for(int32 ResourceIndex = 0;ResourceIndex < DynamicResources.Num();ResourceIndex++)
	{
		DynamicResources[ResourceIndex]->InitPrimitiveResource();
	}
}

IPooledRenderTarget* FViewInfo::GetEyeAdaptation() const
{
	FSceneViewState* ViewState = (FSceneViewState*)State;

	// When rendering in stereo we want to use the same exposure for both eyes.
	if (StereoPass == eSSP_RIGHT_EYE)
	{
		int32 ViewIndex = Family->Views.Find(this);
		if (Family->Views.IsValidIndex(ViewIndex))
		{
			// The left eye is always added before the right eye.
			ViewIndex = ViewIndex - 1;
			if (Family->Views.IsValidIndex(ViewIndex))
			{
				const FSceneView* PrimaryView = Family->Views[ViewIndex];
				if (PrimaryView->StereoPass == eSSP_LEFT_EYE)
				{
					ViewState = (FSceneViewState*)PrimaryView->State;
				}
			}
		}
	}

	if (ViewState)
	{
		TRefCountPtr<IPooledRenderTarget>& EyeAdaptRef = ViewState->GetEyeAdaptation();
		if( IsValidRef(EyeAdaptRef) )
		{
			return EyeAdaptRef.GetReference();
		}
	}
	return NULL;
}
/*-----------------------------------------------------------------------------
	FSceneRenderer
-----------------------------------------------------------------------------*/

FSceneRenderer::FSceneRenderer(const FSceneViewFamily* InViewFamily,FHitProxyConsumer* HitProxyConsumer)
:	Scene(InViewFamily->Scene ? InViewFamily->Scene->GetRenderScene() : NULL)
,	ViewFamily(*InViewFamily)
,	bUsedPrecomputedVisibility(false)
,	FrameNumber(GFrameNumber)
{
	check(Scene != NULL);

	// Copy the individual views.
	Views.Empty(InViewFamily->Views.Num());
	for(int32 ViewIndex = 0;ViewIndex < InViewFamily->Views.Num();ViewIndex++)
	{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		for(int32 ViewIndex2 = 0;ViewIndex2 < InViewFamily->Views.Num();ViewIndex2++)
		{
			if (ViewIndex != ViewIndex2 && InViewFamily->Views[ViewIndex]->State != NULL)
			{
				// Verify that each view has a unique view state, as the occlusion query mechanism depends on it.
				check(InViewFamily->Views[ViewIndex]->State != InViewFamily->Views[ViewIndex2]->State);
			}
		}
#endif
		// Construct a FViewInfo with the FSceneView properties.
		FViewInfo* ViewInfo = new(Views) FViewInfo(InViewFamily->Views[ViewIndex]);
		ViewFamily.Views[ViewIndex] = ViewInfo;
		ViewInfo->Family = &ViewFamily;

#if WITH_EDITOR
		// Should we allow the user to select translucent primitives?
		ViewInfo->bAllowTranslucentPrimitivesInHitProxy =
			GEngine->AllowSelectTranslucent() ||		// User preference enabled?
			!ViewInfo->IsPerspectiveProjection();		// Is orthographic view?
#endif

		// Batch the view's elements for later rendering.
		if(ViewInfo->Drawer)
		{
			FViewElementPDI ViewElementPDI(ViewInfo,HitProxyConsumer);
			ViewInfo->Drawer->Draw(ViewInfo,&ViewElementPDI);
		}
	}
	
	if(HitProxyConsumer)
	{
		// Set the hit proxies show flag.
		ViewFamily.EngineShowFlags.HitProxies = 1;
	}

	ViewFamily.ComputeFamilySize();

	// copy off the requests
	// (I apologize for the const_cast, but didn't seem worth refactoring just for the freezerendering command)
	bHasRequestedToggleFreeze = const_cast<FRenderTarget*>(InViewFamily->RenderTarget)->HasToggleFreezeCommand();

}

bool FSceneRenderer::DoOcclusionQueries() const
{
	return !IsES2Platform(GRHIShaderPlatform)
		&& CVarAllowOcclusionQueries.GetValueOnRenderThread() != 0;
}

FSceneRenderer::~FSceneRenderer()
{
	if(Scene)
	{
		// Destruct the projected shadow infos.
		for(TSparseArray<FLightSceneInfoCompact>::TConstIterator LightIt(Scene->Lights);LightIt;++LightIt)
		{
			if( VisibleLightInfos.IsValidIndex(LightIt.GetIndex()) )
			{
				FVisibleLightInfo& VisibleLightInfo = VisibleLightInfos[LightIt.GetIndex()];
				for(int32 ShadowIndex = 0;ShadowIndex < VisibleLightInfo.MemStackProjectedShadows.Num();ShadowIndex++)
				{
					// FProjectedShadowInfo's in MemStackProjectedShadows were allocated on the rendering thread mem stack, 
					// Their memory will be freed when the stack is freed with no destructor call, so invoke the destructor explicitly
					VisibleLightInfo.MemStackProjectedShadows[ShadowIndex]->~FProjectedShadowInfo();
				}
			}
		}
	}
}

/** 
* Finishes the view family rendering.
*/
void FSceneRenderer::RenderFinish()
{
	SCOPED_DRAW_EVENT(RenderFinish, DEC_SCENE_ITEMS);

	if(FRCPassPostProcessBusyWait::IsPassRequired())
	{
		// mostly view independent but to be safe we use the first view
		const FViewInfo& View = Views[0];

		FMemMark Mark(FMemStack::Get());
		FRenderingCompositePassContext CompositeContext(View);

		FRenderingCompositeOutputRef BusyWait;
		{
			// for debugging purpose, can be controlled by console variable
			FRenderingCompositePass* Node = CompositeContext.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessBusyWait());
			BusyWait = FRenderingCompositeOutputRef(Node);
		}		
		
		if(BusyWait.IsValid())
		{
			CompositeContext.Root->AddDependency(BusyWait);
		}

		CompositeContext.Process(TEXT("RenderFinish"));
	}
	
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	{
		for(int32 ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
		{	
			SCOPED_CONDITIONAL_DRAW_EVENTF(EventView, Views.Num() > 1, DEC_SCENE_ITEMS, TEXT("View%d"), ViewIndex);
			FViewInfo& View = Views[ViewIndex];

			bool bShowPrecomputedVisibilityWarning = false;
			static const auto* CVarPrecomputedVisibilityWarning = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.PrecomputedVisibilityWarning"));
			if (CVarPrecomputedVisibilityWarning && CVarPrecomputedVisibilityWarning->GetValueOnRenderThread() == 1)
			{
				bShowPrecomputedVisibilityWarning = !bUsedPrecomputedVisibility;
			}

			// display a message saying we're frozen
			FSceneViewState* ViewState = (FSceneViewState*)View.State;
			bool bViewParentOrFrozen = ViewState && (ViewState->HasViewParent() || ViewState->bIsFrozen);
			if (bViewParentOrFrozen || bShowPrecomputedVisibilityWarning)
			{
				// this is a helper class for FCanvas to be able to get screen size
				class FRenderTargetTemp : public FRenderTarget
				{
				public:
					FViewInfo& View;

					FRenderTargetTemp(FViewInfo& InView) : View(InView)
					{
					}
					virtual FIntPoint GetSizeXY() const
					{
						return View.ViewRect.Size();
					};
					virtual const FTexture2DRHIRef& GetRenderTargetTexture() const
					{
						return View.Family->RenderTarget->GetRenderTargetTexture();
					}
				} TempRenderTarget(View);

				// create a temporary FCanvas object with the temp render target
				// so it can get the screen size
				int32 Y = 130;
				FCanvas Canvas(&TempRenderTarget, NULL, View.Family->CurrentRealTime, View.Family->CurrentWorldTime, View.Family->DeltaWorldTime);
				if (bViewParentOrFrozen)
				{
					const FText StateText =
						ViewState->bIsFrozen ?
						NSLOCTEXT("SceneRendering", "RenderingFrozen", "Rendering frozen...")
						:
						NSLOCTEXT("SceneRendering", "OcclusionChild", "Occlusion Child");
					Canvas.DrawShadowedText( 10, Y, StateText, GetStatsFont(), FLinearColor(0.8,1.0,0.2,1.0));
					Y += 14;
				}
				if (bShowPrecomputedVisibilityWarning)
				{
					const FText Message = NSLOCTEXT("Renderer", "NoPrecomputedVisibility", "NO PRECOMPUTED VISIBILITY");
					Canvas.DrawShadowedText( 10, Y, Message, GetStatsFont(), FLinearColor(1.0,0.05,0.05,1.0));
					Y += 14;
				}
				Canvas.Flush();
			}
		}
	}
	
#endif //!(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	// Save the post-occlusion visibility stats for the frame and freezing info
	for(int32 ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		const FViewInfo& View = Views[ViewIndex];
		INC_DWORD_STAT_BY(STAT_VisibleStaticMeshElements,View.NumVisibleStaticMeshElements);
		INC_DWORD_STAT_BY(STAT_VisibleDynamicPrimitives,View.VisibleDynamicPrimitives.Num());

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		// update freezing info
		FSceneViewState* ViewState = (FSceneViewState*)View.State;
		if (ViewState)
		{
			// if we're finished freezing, now we are frozen
			if (ViewState->bIsFreezing)
			{
				ViewState->bIsFreezing = false;
				ViewState->bIsFrozen = true;
			}

			// handle freeze toggle request
			if (bHasRequestedToggleFreeze)
			{
				// do we want to start freezing?
				if (!ViewState->bIsFrozen)
				{
					ViewState->bIsFrozen = false;
					ViewState->bIsFreezing = true;
					ViewState->FrozenPrimitives.Empty();
				}
				// or stop?
				else
				{
					ViewState->bIsFrozen = false;
					ViewState->bIsFreezing = false;
					ViewState->FrozenPrimitives.Empty();
				}
			}
		}
#endif
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	// clear the commands
	bHasRequestedToggleFreeze = false;

	if(ViewFamily.EngineShowFlags.OnScreenDebug)
	{
		for(int32 ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
		{
			const FViewInfo& View = Views[ViewIndex];

			if(!View.IsPerspectiveProjection())
			{
				continue;
			}

			GRenderTargetPool.VisualizeTexture.PresentContent(View);
		}
	}

#endif
}

FSceneRenderer* FSceneRenderer::CreateSceneRenderer(const FSceneViewFamily* InViewFamily, FHitProxyConsumer* HitProxyConsumer)
{
	if (GRHIFeatureLevel == ERHIFeatureLevel::ES2)
	{
		return new FForwardShadingSceneRenderer(InViewFamily, HitProxyConsumer);
	}
	else
	{
		return new FDeferredShadingSceneRenderer(InViewFamily, HitProxyConsumer);
	}
}

void FSceneRenderer::RenderCustomDepthPass()
{
	if(!IsFeatureLevelSupported(GRHIShaderPlatform, ERHIFeatureLevel::SM3))
	{
		// not yet supported on lower end platforms
		return;
	}

	// do we have primitives in this pass?
	bool bPrimitives = false;

	if(!Scene->World || Scene->World->WorldType != EWorldType::Preview)
	{
		for(int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
		{
			const FViewInfo& View = Views[ViewIndex];
			if(View.CustomDepthSet.NumPrims())
			{
				bPrimitives = true;
				break;
			}
		}
	}

	// Render CustomDepth
	if(GSceneRenderTargets.BeginRenderingCustomDepth(bPrimitives))
	{
		SCOPED_DRAW_EVENT(CustomDepth, DEC_SCENE_ITEMS);

		for(int32 ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
		{
			SCOPED_CONDITIONAL_DRAW_EVENTF(EventView, Views.Num() > 1, DEC_SCENE_ITEMS, TEXT("View%d"), ViewIndex);

			FViewInfo& View = Views[ViewIndex];

			RHISetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);

			// Note, this is a reversed Z depth surface, so 0.0f is the far plane.
			RHIClear(false, FLinearColor(0,0,0,0), true, 0.0f, false, 0, FIntRect());
			
			// seems this is set each draw call anyway
			RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());

			// Note, this is a reversed Z depth surface, so CF_GreaterEqual.
			RHISetDepthStencilState(TStaticDepthStencilState<true,CF_GreaterEqual>::GetRHI());
			RHISetBlendState(TStaticBlendState<>::GetRHI());

			View.CustomDepthSet.DrawPrims(&View, false);
		}

		// resolve using the current ResolveParams 
		GSceneRenderTargets.FinishRenderingCustomDepth();
	}
}

void FSceneRenderer::OnStartFrame()
{
	GRenderTargetPool.VisualizeTexture.OnStartFrame(Views[0]);
	CompositionGraph_OnStartFrame();
	GSceneRenderTargets.bScreenSpaceAOIsValid = false;
	GSceneRenderTargets.bCustomDepthIsValid = false;

	for(int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
	{	
		FSceneView& View = Views[ViewIndex];
		FSceneViewStateInterface* State = View.State;

		if(State)
		{
			State->OnStartFrame(View);
		}
	}
}

/*-----------------------------------------------------------------------------
	FRendererModule::BeginRenderingViewFamily
-----------------------------------------------------------------------------*/

/**
 * Helper function performing actual work in render thread.
 *
 * @param SceneRenderer	Scene renderer to use for rendering.
 */
static void RenderViewFamily_RenderThread( FSceneRenderer* SceneRenderer )
{
    FMemMark MemStackMark(FMemStack::Get());

	// update any resources that needed a deferred update
	FDeferredUpdateResource::UpdateResources();

    for( int ViewExt = 0; ViewExt < SceneRenderer->ViewFamily.ViewExtensions.Num(); ViewExt++ )
    {
        for( int ViewIndex = 0; ViewIndex < SceneRenderer->ViewFamily.Views.Num(); ViewIndex++ )
        {
            SceneRenderer->ViewFamily.ViewExtensions[ViewExt]->PreRenderView_RenderThread(SceneRenderer->Views[ViewIndex]);
        }
    }

    {
		SCOPE_CYCLE_COUNTER(STAT_TotalSceneRenderingTime);

		if(SceneRenderer->ViewFamily.EngineShowFlags.HitProxies)
		{
			// Render the scene's hit proxies.
			SceneRenderer->RenderHitProxies();
		}
		else
		{
			// Render the scene.
			SceneRenderer->Render();
		}

#if STATS
		// Update scene memory stats that couldn't be tracked continuously
		SET_MEMORY_STAT(STAT_StaticDrawListMemory, FStaticMeshDrawListBase::TotalBytesUsed);
		SET_MEMORY_STAT(STAT_RenderingSceneMemory, SceneRenderer->Scene->GetSizeBytes());

		SIZE_T ViewStateMemory = 0;
		for (int32 ViewIndex = 0; ViewIndex < SceneRenderer->Views.Num(); ViewIndex++)
		{
			if (SceneRenderer->Views[ViewIndex].State)
			{
				ViewStateMemory += SceneRenderer->Views[ViewIndex].State->GetSizeBytes();
			}
		}
		SET_MEMORY_STAT(STAT_ViewStateMemory, ViewStateMemory);
		SET_MEMORY_STAT(STAT_RenderingMemStackMemory, FMemStack::Get().GetByteCount());
		SET_MEMORY_STAT(STAT_LightInteractionMemory, FLightPrimitiveInteraction::GetMemoryPoolSize());
#endif

        // Delete the scene renderer.
		delete SceneRenderer;
	}

#if STATS
	if (FPlatformProperties::SupportsWindowedMode() == false)
	{
		/** Update STATS with the total GPU time taken to render the last frame. */
		SET_CYCLE_COUNTER(STAT_TotalGPUFrameTime, RHIGetGPUFrameCycles());
	}
#endif
}

void FRendererModule::BeginRenderingViewFamily(FCanvas* Canvas,const FSceneViewFamily* ViewFamily)
{
	// Flush the canvas first.
	Canvas->Flush();

	// Increment FrameNumber before render the scene. Wrapping around is no problem.
	// This is the only spot we change GFrameNumber, other places can only read.
	++GFrameNumber;

	check(ViewFamily->Scene);
	FScene* const Scene = ViewFamily->Scene->GetRenderScene();
	if (Scene)
	{
		UWorld* const World = Scene->GetWorld();
		// Set the world's "needs full lighting rebuild" flag if the scene has any uncached static lighting interactions.
		if(World)
		{
			// Note: reading NumUncachedStaticLightingInteractions on the game thread here which is written to by the rendering thread
			// This is reliable because the RT uses interlocked mechanisms to update it
			World->SetMapNeedsLightingFullyRebuilt(Scene->NumUncachedStaticLightingInteractions);
		}
	
		// Construct the scene renderer.  This copies the view family attributes into its own structures.
		FSceneRenderer* SceneRenderer = FSceneRenderer::CreateSceneRenderer(ViewFamily, Canvas->GetHitProxyConsumer());

		ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
			FDrawSceneCommand,
			FSceneRenderer*,SceneRenderer,SceneRenderer,
		{
			RenderViewFamily_RenderThread(SceneRenderer);
		});
	}
}

void FRendererModule::UpdateMapNeedsLightingFullyRebuiltState(UWorld* World)
{
	World->SetMapNeedsLightingFullyRebuilt(World->Scene->GetRenderScene()->NumUncachedStaticLightingInteractions);
}

bool IsMobileHDR()
{
	static auto* MobileHDRCvar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.MobileHDR"));
	return MobileHDRCvar->GetValueOnRenderThread() == 1;
}

bool IsMobileHDR32bpp()
{
	static auto* MobileHDR32bppCvar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.MobileHDR32bpp"));
	return IsMobileHDR() && (GSupportsRenderTargetFormat_PF_FloatRGBA == false || MobileHDR32bppCvar->GetValueOnRenderThread() == 1);
}
