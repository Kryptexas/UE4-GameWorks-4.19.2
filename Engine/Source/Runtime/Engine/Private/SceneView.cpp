// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SceneView.cpp: SceneView implementation.
=============================================================================*/

#include "EnginePrivate.h"
#include "SceneManagement.h"
#include "Engine.h"
#include "StereoRendering.h"
#include "HighResScreenshot.h"

DEFINE_LOG_CATEGORY(LogBufferVisualization);

IMPLEMENT_UNIFORM_BUFFER_STRUCT(FPrimitiveUniformShaderParameters,TEXT("Primitive"));
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FViewUniformShaderParameters,TEXT("View"));

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
static TAutoConsoleVariable<float> CVarSSRMaxRoughness(
	TEXT("r.SSR.MaxRoughness"),
	-1.0f,
	TEXT("Allows to override the post process setting ScreenSpaceReflectionMaxRoughness.\n")
	TEXT("It defines until what roughness we fade the screen space reflections, 0.8 works well, smaller can run faster.\n")
	TEXT("(Useful for testing, no scalability or project setting)\n")
	TEXT(" 0..1: use specified max roughness (overrride PostprocessVolume setting)\n")
	TEXT(" -1: no override (default)"),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarSSRRoughnessScale(
	TEXT("r.SSR.RoughnessScale"),
	-1.0f,
	TEXT("Allows to override the post process setting ScreenSpaceReflectionRoughnessScale.\n")
	TEXT("(Useful for testing, no scalability or project setting)\n")
	TEXT("   0: don't do glossy screen space reflections (faster, no noise)\n")
	TEXT("0..1: transiation betwene 0 and 1 (adjustable noise)\n")
	TEXT("   1: realistic and fitting to other reflections\n")
	TEXT("  -1: no override (default)"),
	ECVF_RenderThreadSafe);
#endif

/** Global vertex color view mode setting when SHOW_VertexColors show flag is set */
EVertexColorViewMode::Type GVertexColorViewMode = EVertexColorViewMode::Color;

/** Global primitive uniform buffer resource containing identity transformations. */
TGlobalResource<FIdentityPrimitiveUniformBuffer> GIdentityPrimitiveUniformBuffer;

FSceneViewStateReference::~FSceneViewStateReference()
{
	Destroy();
}

void FSceneViewStateReference::Allocate()
{
	check(!Reference);
	Reference = GetRendererModule().AllocateViewState();
	GlobalListLink = TLinkedList<FSceneViewStateReference*>(this);
	GlobalListLink.Link(GetSceneViewStateList());
}

void FSceneViewStateReference::Destroy()
{
	GlobalListLink.Unlink();

	if (Reference)
	{
		Reference->Destroy();
		Reference = NULL;
	}
}

void FSceneViewStateReference::DestroyAll()
{
	for(TLinkedList<FSceneViewStateReference*>::TIterator ViewStateIt(FSceneViewStateReference::GetSceneViewStateList());ViewStateIt;ViewStateIt.Next())
	{
		FSceneViewStateReference* ViewStateReference = *ViewStateIt;
		ViewStateReference->Reference->Destroy();
		ViewStateReference->Reference = NULL;
	}
}

void FSceneViewStateReference::AllocateAll()
{
	for(TLinkedList<FSceneViewStateReference*>::TIterator ViewStateIt(FSceneViewStateReference::GetSceneViewStateList());ViewStateIt;ViewStateIt.Next())
	{
		FSceneViewStateReference* ViewStateReference = *ViewStateIt;
		ViewStateReference->Reference = GetRendererModule().AllocateViewState();
	}
}

TLinkedList<FSceneViewStateReference*>*& FSceneViewStateReference::GetSceneViewStateList()
{
	static TLinkedList<FSceneViewStateReference*>* List = NULL;
	return List;
}

/**
 * Utility function to create the inverse depth projection transform to be used
 * by the shader system.
 * @param ProjMatrix - used to extract the scene depth ratios
 * @param InvertZ - projection calc is affected by inverted device Z
 * @return vector containing the ratios needed to convert from device Z to world Z
 */
FVector4 CreateInvDeviceZToWorldZTransform(FMatrix const & ProjMatrix)
{
	// The depth projection comes from the the following projection matrix:
	//
	// | 1  0  0  0 |
	// | 0  1  0  0 |
	// | 0  0  A  1 |
	// | 0  0  B  0 |
	//
	// Z' = (Z * A + B) / Z
	// Z' = A + B / Z
	//
	// So to get Z from Z' is just:
	// Z = B / (Z' - A)
	// 
	// Note a reversed Z projection matrix will have A=0. 
	//
	// Done in shader as:
	// Z = 1 / (Z' * C1 - C2)   --- Where C1 = 1/B, C2 = A/B
	//

	float DepthMul = ProjMatrix.M[2][2];
	float DepthAdd = ProjMatrix.M[3][2];

	float SubtractValue = DepthMul / DepthAdd;

	// Subtract a tiny number to avoid divide by 0 errors in the shader when a very far distance is decided from the depth buffer.
	// This fixes fog not being applied to the black background in the editor.
	SubtractValue -= 0.00000001f;

	return FVector4(
		0.0f,			// Unused
		0.0f,			// Unused
		1.f / DepthAdd,	
		SubtractValue
		);
}

FSceneView::FSceneView(const FSceneViewInitOptions& InitOptions)
	: Family(InitOptions.ViewFamily)
	, State(InitOptions.SceneViewStateInterface)
	, ViewActor(InitOptions.ViewActor)
	, Drawer(InitOptions.ViewElementDrawer)
	, ViewRect(InitOptions.GetConstrainedViewRect())
	, UnscaledViewRect(InitOptions.GetConstrainedViewRect())
	, UnconstrainedViewRect(InitOptions.GetViewRect())
	, FrameNumber(UINT_MAX)
	, MaxShadowCascades(10)
	, WorldToMetersScale(InitOptions.WorldToMetersScale)
	, ProjectionMatrixUnadjustedForRHI(InitOptions.ProjectionMatrix)
	, BackgroundColor(InitOptions.BackgroundColor)
	, OverlayColor(InitOptions.OverlayColor)
	, ColorScale(InitOptions.ColorScale)
	, StereoPass(InitOptions.StereoPass)
	, DiffuseOverrideParameter(FVector4(0,0,0,1))
	, SpecularOverrideParameter(FVector4(0,0,0,1))
	, NormalOverrideParameter(FVector4(0,0,0,1))
	, RoughnessOverrideParameter(FVector2D(0,1))
	, HiddenPrimitives(InitOptions.HiddenPrimitives)
	, LODDistanceFactor(InitOptions.LODDistanceFactor)
	, bCameraCut(InitOptions.bInCameraCut)
	, bIsGameView(false)
	, bForceShowMaterials(false)
	, bIsViewInfo(false)
	, bIsSceneCapture(false)
	, bIsReflectionCapture(false)
	, RenderingCompositePassContext(0)
#if WITH_EDITOR
	, OverrideLODViewOrigin(InitOptions.OverrideLODViewOrigin)
	, bAllowTranslucentPrimitivesInHitProxy( true )
#endif
{
	check(UnscaledViewRect.Min.X >= 0);
	check(UnscaledViewRect.Min.Y >= 0);
	check(UnscaledViewRect.Width() > 0);
	check(UnscaledViewRect.Height() > 0);

	ViewMatrices.ViewMatrix = InitOptions.ViewMatrix;

	// Adjust the projection matrix for the current RHI.
	ViewMatrices.ProjMatrix = AdjustProjectionMatrixForRHI(ProjectionMatrixUnadjustedForRHI);

	// Compute the view projection matrix and its inverse.
	ViewProjectionMatrix = ViewMatrices.GetViewProjMatrix();

	// For precision reasons the view matrix inverse is calculated independently.
	InvViewMatrix = ViewMatrices.ViewMatrix.InverseSafe();
	InvViewProjectionMatrix = ViewMatrices.GetInvProjMatrix() * InvViewMatrix;

	bool ApplyPreViewTranslation = true;

	// Calculate the view origin from the view/projection matrices.
	if(IsPerspectiveProjection())
	{
		ViewMatrices.ViewOrigin = InvViewMatrix.GetOrigin();
	}
#if WITH_EDITOR
	else if (InitOptions.bUseFauxOrthoViewPos)
	{
		float DistanceToViewOrigin = WORLD_MAX;
		ViewMatrices.ViewOrigin = FVector4(InvViewMatrix.TransformVector(FVector(0,0,-1)).SafeNormal()*DistanceToViewOrigin,1) + InvViewMatrix.GetOrigin();
	}
#endif
	else
	{
		ViewMatrices.ViewOrigin = FVector4(InvViewMatrix.TransformVector(FVector(0,0,-1)).SafeNormal(),0);
		// to avoid issues with view dependent effect (e.g. Frensel)
		ApplyPreViewTranslation = false;
	}

	// Translate world-space so its origin is at ViewOrigin for improved precision.
	// Note that this isn't exactly right for orthogonal projections (See the above special case), but we still use ViewOrigin
	// in that case so the same value may be used in shaders for both the world-space translation and the camera's world position.
	if(ApplyPreViewTranslation)
	{
		ViewMatrices.PreViewTranslation = -FVector(ViewMatrices.ViewOrigin);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		{
			// console variable override
			static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.PreViewTranslation")); 
			int32 Value = CVar->GetValueOnGameThread();

			static FVector PreViewTranslationBackup;

			if(Value)
			{
				PreViewTranslationBackup = ViewMatrices.PreViewTranslation;
			}
			else
			{
				ViewMatrices.PreViewTranslation = PreViewTranslationBackup;
			}
		}
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	}

	/** The view transform, starting from world-space points translated by -ViewOrigin. */
	FMatrix TranslatedViewMatrix = FTranslationMatrix(-ViewMatrices.PreViewTranslation) * ViewMatrices.ViewMatrix;
	
	// Compute a transform from view origin centered world-space to clip space.
	ViewMatrices.TranslatedViewProjectionMatrix = TranslatedViewMatrix * ViewMatrices.ProjMatrix;
	ViewMatrices.InvTranslatedViewProjectionMatrix = ViewMatrices.TranslatedViewProjectionMatrix.InverseSafe();
	
	ShadowViewMatrices = ViewMatrices;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	{
		// console variable override
		static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.Shadow.FreezeCamera")); 
		int32 Value = CVar->GetValueOnAnyThread();

		static FViewMatrices Backup = ShadowViewMatrices;

		if(Value)
		{
			ShadowViewMatrices = Backup;
		}
		else
		{
			Backup = ShadowViewMatrices;
		}
	}
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

	if (InitOptions.OverrideFarClippingPlaneDistance > 0.0f)
	{
		const FPlane FarPlane(ViewMatrices.ViewOrigin + GetViewDirection() * InitOptions.OverrideFarClippingPlaneDistance, GetViewDirection());
		// Derive the view frustum from the view projection matrix, overriding the far plane
		GetViewFrustumBounds(ViewFrustum,ViewProjectionMatrix,FarPlane,true,false);
	}
	else
	{
		// Derive the view frustum from the view projection matrix.
		GetViewFrustumBounds(ViewFrustum,ViewProjectionMatrix,false);
	}

	// Derive the view's near clipping distance and plane.
	// The GetFrustumFarPlane() is the near plane because of reverse Z projection.
	bHasNearClippingPlane = ViewProjectionMatrix.GetFrustumFarPlane(NearClippingPlane);
	if(ViewMatrices.ProjMatrix.M[2][3] > DELTA)
	{
		// Infinite projection with reversed Z.
		NearClippingDistance = ViewMatrices.ProjMatrix.M[3][2];
	}
	else
	{
		// Ortho projection with reversed Z.
		NearClippingDistance = (1.0f - ViewMatrices.ProjMatrix.M[3][2]) / ViewMatrices.ProjMatrix.M[2][2];
	}

	// Determine whether the view should reverse the cull mode due to a negative determinant.  Only do this for a valid scene
	bReverseCulling = (Family && Family->Scene) ? FMath::IsNegativeFloat(ViewMatrices.ViewMatrix.Determinant()) : false;

	// OpenGL Gamma space output in GLSL flips Y when rendering directly to the back buffer (so not needed on PC, as we never render directly into the back buffer)
	static bool bPlatformRequiresReverseCulling = (IsOpenGLPlatform(GRHIShaderPlatform) && !IsPCPlatform(GRHIShaderPlatform));
	static auto* MobileHDRCvar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.MobileHDR"));
	check(MobileHDRCvar);
	bReverseCulling = (bPlatformRequiresReverseCulling && MobileHDRCvar->GetValueOnAnyThread() == 0) ? !bReverseCulling : bReverseCulling;

	// Setup transformation constants to be used by the graphics hardware to transform device normalized depth samples
	// into world oriented z.
	InvDeviceZToWorldZTransform = CreateInvDeviceZToWorldZTransform(ProjectionMatrixUnadjustedForRHI);

	// As the world is only accessable from the game thread, bIsGameView should be explicitly
	// set on any other thread.
	if(IsInGameThread())
	{
		bIsGameView = (Family && Family->Scene && Family->Scene->GetWorld() ) ? Family->Scene->GetWorld()->IsGameWorld() : false;
	}

#if WITH_EDITOR
	EditorViewBitflag = InitOptions.EditorViewBitflag;

	SelectionOutlineColor = GEngine->GetSelectionOutlineColor();
#endif
}

void FSceneView::UpdateViewMatrix()
{
	FVector StereoViewLocation = ViewLocation;
	if (GEngine->IsStereoscopic3D() && StereoPass != eSSP_FULL)
	{
		GEngine->StereoRenderingDevice->CalculateStereoViewOffset(StereoPass, ViewRotation, WorldToMetersScale, StereoViewLocation);
		ViewLocation = StereoViewLocation;
	}

	ViewMatrices.ViewOrigin = StereoViewLocation;

 	ViewMatrices.ViewMatrix = FTranslationMatrix(-StereoViewLocation);
 	ViewMatrices.ViewMatrix = ViewMatrices.ViewMatrix * FInverseRotationMatrix(ViewRotation);
 	ViewMatrices.ViewMatrix = ViewMatrices.ViewMatrix * FMatrix(
 		FPlane(0,	0,	1,	0),
 		FPlane(1,	0,	0,	0),
 		FPlane(0,	1,	0,	0),
 		FPlane(0,	0,	0,	1));
 
	ViewMatrices.PreViewTranslation = -ViewMatrices.ViewOrigin;
  	FMatrix TranslatedViewMatrix = FTranslationMatrix(-ViewMatrices.PreViewTranslation) * ViewMatrices.ViewMatrix;
	
	// Compute a transform from view origin centered world-space to clip space.
 	ViewMatrices.TranslatedViewProjectionMatrix = TranslatedViewMatrix * ViewMatrices.ProjMatrix;
 	ViewMatrices.InvTranslatedViewProjectionMatrix = ViewMatrices.TranslatedViewProjectionMatrix.InverseSafe();

 	ViewProjectionMatrix = ViewMatrices.GetViewProjMatrix();
 	InvViewMatrix = ViewMatrices.GetInvViewMatrix();
 	InvViewProjectionMatrix = ViewMatrices.GetInvViewProjMatrix();

}

void FSceneView::SetScaledViewRect(FIntRect InScaledViewRect)
{
	check(InScaledViewRect.Min.X >= 0);
	check(InScaledViewRect.Min.Y >= 0);
	check(InScaledViewRect.Width() > 0);
	check(InScaledViewRect.Height() > 0);

	check(ViewRect == UnscaledViewRect);

	ViewRect = InScaledViewRect;
}

FVector4 FSceneView::WorldToScreen(const FVector& WorldPoint) const
{
	return ViewProjectionMatrix.TransformFVector4(FVector4(WorldPoint,1));
}

FVector FSceneView::ScreenToWorld(const FVector4& ScreenPoint) const
{
	return InvViewProjectionMatrix.TransformFVector4(ScreenPoint);
}

bool FSceneView::ScreenToPixel(const FVector4& ScreenPoint,FVector2D& OutPixelLocation) const
{
	if(ScreenPoint.W > 0.0f)
	{
		float InvW = 1.0f / ScreenPoint.W;
		float Y = (GProjectionSignY > 0.0f) ? ScreenPoint.Y : 1.0f - ScreenPoint.Y;
		OutPixelLocation = FVector2D(
			ViewRect.Min.X + (0.5f + ScreenPoint.X * 0.5f * InvW) * ViewRect.Width(),
			ViewRect.Min.Y + (0.5f - Y * 0.5f * InvW) * ViewRect.Height()
			);
		return true;
	}
	else
	{
		return false;
	}
}

FVector4 FSceneView::PixelToScreen(float InX,float InY,float Z) const
{
	if (GProjectionSignY > 0.0f)
	{
		return FVector4(
			-1.0f + InX / ViewRect.Width() * +2.0f,
			+1.0f + InY / ViewRect.Height() * -2.0f,
			Z,
			1
			);
	}
	else
	{
		return FVector4(
			-1.0f + InX / ViewRect.Width() * +2.0f,
			1.0f - (+1.0f + InY / ViewRect.Height() * -2.0f),
			Z,
			1
			);
	}
}

/** Transforms a point from the view's world-space into pixel coordinates relative to the view's X,Y. */
bool FSceneView::WorldToPixel(const FVector& WorldPoint,FVector2D& OutPixelLocation) const
{
	const FVector4 ScreenPoint = WorldToScreen(WorldPoint);
	return ScreenToPixel(ScreenPoint, OutPixelLocation);
}

/** Transforms a point from pixel coordinates relative to the view's X,Y (left, top) into the view's world-space. */
FVector4 FSceneView::PixelToWorld(float X,float Y,float Z) const
{
	const FVector4 ScreenPoint = PixelToScreen(X, Y, Z);
	return ScreenToWorld(ScreenPoint);
}

/**  
 * Transforms a point from the view's world-space into the view's screen-space.  
 * Divides the resulting X, Y, Z by W before returning. 
 */
FPlane FSceneView::Project(const FVector& WorldPoint) const
{
	FPlane Result = WorldToScreen(WorldPoint);
	const float RHW = 1.0f / Result.W;

	return FPlane(Result.X * RHW,Result.Y * RHW,Result.Z * RHW,Result.W);
}

/** 
 * Transforms a point from the view's screen-space into world coordinates
 * multiplies X, Y, Z by W before transforming. 
 */
FVector FSceneView::Deproject(const FPlane& ScreenPoint) const
{
	return InvViewProjectionMatrix.TransformFVector4(FPlane(ScreenPoint.X * ScreenPoint.W,ScreenPoint.Y * ScreenPoint.W,ScreenPoint.Z * ScreenPoint.W,ScreenPoint.W));
}

void FSceneView::DeprojectFVector2D(const FVector2D& ScreenPos, FVector& out_WorldOrigin, FVector& out_WorldDirection) const
{
	const FMatrix InverseViewMatrix = ViewMatrices.ViewMatrix.Inverse();
	const FMatrix InvProjectionMatrix = ViewMatrices.GetInvProjMatrix();
	
	DeprojectScreenToWorld(ScreenPos, UnscaledViewRect, InverseViewMatrix, InvProjectionMatrix, out_WorldOrigin, out_WorldDirection);
}

void FSceneView::DeprojectScreenToWorld(const FVector2D& ScreenPos, const FIntRect& ViewRect, const FMatrix& InvViewMatrix, const FMatrix& InvProjectionMatrix, FVector& out_WorldOrigin, FVector& out_WorldDirection)
{
	int32 PixelX = FMath::Trunc(ScreenPos.X);
	int32 PixelY = FMath::Trunc(ScreenPos.Y);

	// Get the eye position and direction of the mouse cursor in two stages (inverse transform projection, then inverse transform view).
	// This avoids the numerical instability that occurs when a view matrix with large translation is composed with a projection matrix

	// Get the pixel coordinates into 0..1 normalized coordinates within the constrained view rectangle
	const float NormalizedX = (PixelX - ViewRect.Min.X) / ((float)ViewRect.Width());
	const float NormalizedY = (PixelY - ViewRect.Min.Y) / ((float)ViewRect.Height());

	// Get the pixel coordinates into -1..1 projection space
	const float ScreenSpaceX = (NormalizedX - 0.5f) * 2.0f;
	const float ScreenSpaceY = ((1.0f - NormalizedY) - 0.5f) * 2.0f;

	// The start of the raytrace is defined to be at mousex,mousey,1 in projection space (z=1 is near, z=0 is far - this gives us better precision)
	// To get the direction of the raytrace we need to use any z between the near and the far plane, so let's use (mousex, mousey, 0.5)
	const FVector4 RayStartProjectionSpace = FVector4(ScreenSpaceX, ScreenSpaceY, 1.0f, 1.0f);
	const FVector4 RayEndProjectionSpace = FVector4(ScreenSpaceX, ScreenSpaceY, 0.5f, 1.0f);

	// Projection (changing the W coordinate) is not handled by the FMatrix transforms that work with vectors, so multiplications
	// by the projection matrix should use homogeneous coordinates (i.e. FPlane).
	const FVector4 HGRayStartViewSpace = InvProjectionMatrix.TransformFVector4(RayStartProjectionSpace);
	const FVector4 HGRayEndViewSpace = InvProjectionMatrix.TransformFVector4(RayEndProjectionSpace);
	FVector RayStartViewSpace(HGRayStartViewSpace.X, HGRayStartViewSpace.Y, HGRayStartViewSpace.Z);
	FVector RayEndViewSpace(HGRayEndViewSpace.X,   HGRayEndViewSpace.Y,   HGRayEndViewSpace.Z);
	// divide vectors by W to undo any projection and get the 3-space coordinate 
	if (HGRayStartViewSpace.W != 0.0f)
	{
		RayStartViewSpace /= HGRayStartViewSpace.W;
	}
	if (HGRayEndViewSpace.W != 0.0f)
	{
		RayEndViewSpace /= HGRayEndViewSpace.W;
	}
	FVector RayDirViewSpace = RayEndViewSpace - RayStartViewSpace;
	RayDirViewSpace = RayDirViewSpace.SafeNormal();

	// The view transform does not have projection, so we can use the standard functions that deal with vectors and normals (normals
	// are vectors that do not use the translational part of a rotation/translation)
	const FVector RayStartWorldSpace = InvViewMatrix.TransformPosition(RayStartViewSpace);
	const FVector RayDirWorldSpace = InvViewMatrix.TransformVector(RayDirViewSpace);

	// Finally, store the results in the hitcheck inputs.  The start position is the eye, and the end position
	// is the eye plus a long distance in the direction the mouse is pointing.
	out_WorldOrigin = RayStartWorldSpace;
	out_WorldDirection = RayDirWorldSpace.SafeNormal();
}

#define LERP_PP(NAME) if(Src.bOverride_ ## NAME)	Dest . NAME = FMath::Lerp(Dest . NAME, Src . NAME, Weight);
#define IF_PP(NAME) if(Src.bOverride_ ## NAME && Src . NAME)


// @param Weight 0..1
void FSceneView::OverridePostProcessSettings(const FPostProcessSettings& Src, float Weight)
{
	if(Weight <= 0.0f)
	{
		// no need to blend anything
		return;
	}

	if(Weight > 1.0f)
	{
		Weight = 1.0f;
	}

	FFinalPostProcessSettings& Dest = FinalPostProcessSettings;

	// The following code needs to be adjusted when settings in FPostProcessSettings change.
	LERP_PP(FilmWhitePoint);
	LERP_PP(FilmSaturation);
	LERP_PP(FilmChannelMixerRed);
	LERP_PP(FilmChannelMixerGreen);
	LERP_PP(FilmChannelMixerBlue);
	LERP_PP(FilmContrast);
	LERP_PP(FilmDynamicRange);
	LERP_PP(FilmHealAmount);
	LERP_PP(FilmToeAmount);
	LERP_PP(FilmShadowTint);
	LERP_PP(FilmShadowTintBlend);
	LERP_PP(FilmShadowTintAmount);

	LERP_PP(SceneColorTint);
	LERP_PP(SceneFringeIntensity);
	LERP_PP(SceneFringeSaturation);
	LERP_PP(BloomIntensity);
	LERP_PP(BloomThreshold);
	LERP_PP(Bloom1Tint);
	LERP_PP(Bloom1Size);
	LERP_PP(Bloom2Tint);
	LERP_PP(Bloom2Size);
	LERP_PP(Bloom3Tint);
	LERP_PP(Bloom3Size);
	LERP_PP(Bloom4Tint);
	LERP_PP(Bloom4Size);
	LERP_PP(Bloom5Tint);
	LERP_PP(Bloom5Size);
	LERP_PP(BloomDirtMaskIntensity);
	LERP_PP(BloomDirtMaskTint);
	LERP_PP(AmbientCubemapIntensity);
	LERP_PP(AmbientCubemapTint);
	LERP_PP(LPVIntensity);
	LERP_PP(LPVWarpIntensity);
	LERP_PP(LPVTransmissionIntensity);
	LERP_PP(AutoExposureLowPercent);
	LERP_PP(AutoExposureHighPercent);
	LERP_PP(AutoExposureMinBrightness);
	LERP_PP(AutoExposureMaxBrightness);
	LERP_PP(AutoExposureSpeedUp);
	LERP_PP(AutoExposureSpeedDown);
	LERP_PP(AutoExposureBias);
	LERP_PP(HistogramLogMin);
	LERP_PP(HistogramLogMax);
	LERP_PP(LensFlareIntensity);
	LERP_PP(LensFlareTint);
	LERP_PP(LensFlareBokehSize);
	LERP_PP(LensFlareThreshold);
	LERP_PP(VignetteIntensity);
	LERP_PP(VignetteColor);
	LERP_PP(GrainIntensity);
	LERP_PP(GrainJitter);
	LERP_PP(AmbientOcclusionIntensity);
	LERP_PP(AmbientOcclusionStaticFraction);
	LERP_PP(AmbientOcclusionRadius);
	LERP_PP(AmbientOcclusionFadeDistance);
	LERP_PP(AmbientOcclusionFadeRadius);
	LERP_PP(AmbientOcclusionDistance);
	LERP_PP(AmbientOcclusionPower);
	LERP_PP(AmbientOcclusionBias);
	LERP_PP(AmbientOcclusionQuality);
	LERP_PP(AmbientOcclusionMipBlend);
	LERP_PP(AmbientOcclusionMipScale);
	LERP_PP(AmbientOcclusionMipThreshold);
	LERP_PP(IndirectLightingColor);
	LERP_PP(IndirectLightingIntensity);
	LERP_PP(DepthOfFieldFocalDistance);
	LERP_PP(DepthOfFieldFocalRegion);
	LERP_PP(DepthOfFieldNearTransitionRegion);
	LERP_PP(DepthOfFieldFarTransitionRegion);
	LERP_PP(DepthOfFieldScale);
	LERP_PP(DepthOfFieldMaxBokehSize);
	LERP_PP(DepthOfFieldNearBlurSize);
	LERP_PP(DepthOfFieldFarBlurSize);
	LERP_PP(DepthOfFieldOcclusion);
	LERP_PP(DepthOfFieldColorThreshold);
	LERP_PP(DepthOfFieldSizeThreshold);
	LERP_PP(DepthOfFieldSkyFocusDistance);
	LERP_PP(MotionBlurAmount);
	LERP_PP(MotionBlurMax);
	LERP_PP(MotionBlurPerObjectSize);
	LERP_PP(ScreenPercentage);
	LERP_PP(ScreenSpaceReflectionQuality);
	LERP_PP(ScreenSpaceReflectionIntensity);
	LERP_PP(ScreenSpaceReflectionMaxRoughness);
	LERP_PP(ScreenSpaceReflectionRoughnessScale);

	// cubemaps are getting blended additively - in contrast to other properties, maybe we should make that consistent
	if(Src.AmbientCubemap && Src.bOverride_AmbientCubemapIntensity)
	{
		FFinalPostProcessSettings::FCubemapEntry Entry;

		Entry.AmbientCubemapTintMulScaleValue = FLinearColor(1, 1, 1, 1) * Src.AmbientCubemapIntensity;

		if(Src.bOverride_AmbientCubemapTint)
		{
			Entry.AmbientCubemapTintMulScaleValue *= Src.AmbientCubemapTint;
		}

		Entry.AmbientCubemap = Src.AmbientCubemap;
		Dest.UpdateEntry(Entry, Weight);
	}

	IF_PP(ColorGradingLUT)
	{
		float ColorGradingIntensity = FMath::Clamp(Src.ColorGradingIntensity, 0.0f, 1.0f);
		Dest.LerpTo(Src.ColorGradingLUT, ColorGradingIntensity * Weight);
	}

	// actual texture cannot be blended but the intensity can be blended
	IF_PP(BloomDirtMask)
	{
		Dest.BloomDirtMask = Src.BloomDirtMask;
	}

	// actual texture cannot be blended but the intensity can be blended
	IF_PP(DepthOfFieldBokehShape)
	{
		Dest.DepthOfFieldBokehShape = Src.DepthOfFieldBokehShape;
	}

	// actual texture cannot be blended but the intensity can be blended
	IF_PP(LensFlareBokehShape)
	{
		Dest.LensFlareBokehShape = Src.LensFlareBokehShape;
	}

	if(Src.bOverride_LPVSize)
	{
		Dest.LPVSize = Src.LPVSize;
	}
	LERP_PP( LPVSecondaryOcclusionIntensity );
	LERP_PP( LPVSecondaryBounceIntensity );
	LERP_PP( LPVVplInjectionBias );
	LERP_PP( LPVGeometryVolumeBias );
	LERP_PP( LPVEmissiveInjectionIntensity );

	if(Src.bOverride_LensFlareTints)
	{
		for(uint32 i = 0; i < 8; ++i)
		{
			Dest.LensFlareTints[i] = FMath::Lerp(Dest.LensFlareTints[i], Src.LensFlareTints[i], Weight);
		}
	}

	if(Src.bOverride_DepthOfFieldMethod)
	{
		Dest.DepthOfFieldMethod = Src.DepthOfFieldMethod;
	}

	if(Src.bOverride_AmbientOcclusionRadiusInWS)
	{
		Dest.AmbientOcclusionRadiusInWS = Src.AmbientOcclusionRadiusInWS;
	}

	if(Src.bOverride_AntiAliasingMethod)
	{
		Dest.AntiAliasingMethod = Src.AntiAliasingMethod;
	}

	// Blendable objects
	{
		uint32 Count = Src.Blendables.Num();

		for(uint32 i = 0; i < Count; ++i)
		{
			// todo: optimize
			UObject* Object = Src.Blendables[i];

			if(!Object)
			{
				continue;
			}

			IBlendableInterface* BlendableInterface = InterfaceCast<IBlendableInterface>(Object);
			
			if(!BlendableInterface)
			{
				continue;
			}

			BlendableInterface->OverrideBlendableSettings(*this, Weight);
		}
	}
}

/** Dummy class needed to support InterfaceCast<IBlendableInterface>(Object) */
UBlendableInterface::UBlendableInterface(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP) 
{
}

void FSceneView::StartFinalPostprocessSettings(FVector InViewLocation)
{
	check(IsInGameThread());

	// The final settings for the current viewer position (blended together from many volumes).
	// Setup by the main thread, passed to the render thread and never touched again by the main thread.

	// Set values before any override happens.
	FinalPostProcessSettings.SetBaseValues();

	if(State)
	{
		State->OnStartPostProcessing(*this);
	}

	UWorld* World = Family->Scene->GetWorld();

	// Some views have no world (e.g. material preview)
	if(World)
	{
		APostProcessVolume* Volume = World->LowestPriorityPostProcessVolume;
		while(Volume)
		{
			// Volume encompasses
			if(Volume->bEnabled)
			{
				float DistanceToPoint = 0.0f;
				float LocalWeight = FMath::Clamp(Volume->BlendWeight, 0.0f, 1.0f);

				if(!Volume->bUnbound)
				{
					Volume->EncompassesPoint(InViewLocation, 0.0f, &DistanceToPoint);

					if (DistanceToPoint >= 0)
					{
						if(DistanceToPoint > Volume->BlendRadius)
						{
							// outside
							LocalWeight = 0.0f;
						}
						else
						{
							// to avoid div by 0
							if(Volume->BlendRadius >= 1.0f)
							{
								LocalWeight *= 1.0f - DistanceToPoint / Volume->BlendRadius;

								check(LocalWeight >=0 && LocalWeight <= 1.0f);
							}
						}
					}
				}

				OverridePostProcessSettings(Volume->Settings, LocalWeight);
			}

			// further traverse linked list.
			Volume = Volume->NextHigherPriorityVolume;
		}
	}
}

void FSceneView::EndFinalPostprocessSettings()
{
	{
		static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.BloomQuality"));

		int Value = CVar->GetValueOnGameThread();

		if(Value <= 0)
		{
			FinalPostProcessSettings.BloomIntensity = 0.0f;
		}
	}

	if(!Family->EngineShowFlags.Bloom)
	{
		FinalPostProcessSettings.BloomIntensity = 0.0f;
	}

	if(!Family->EngineShowFlags.GlobalIllumination)
	{
		FinalPostProcessSettings.LPVIntensity = 0.0f;
	}

	{
		static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.DepthOfFieldQuality"));

		int Value = CVar->GetValueOnGameThread();

		if(Value <= 0)
		{
			FinalPostProcessSettings.DepthOfFieldScale = 0.0f;
		}
	}

	if(!Family->EngineShowFlags.DepthOfField)
	{
		FinalPostProcessSettings.DepthOfFieldScale = 0;
	}

	if(!Family->EngineShowFlags.Vignette)
	{
		FinalPostProcessSettings.VignetteIntensity = 0;
		FinalPostProcessSettings.VignetteColor = FLinearColor(0.0f, 0.0f, 0.0f);
	}

	if(!Family->EngineShowFlags.Grain)
	{
		FinalPostProcessSettings.GrainIntensity = 0;
		FinalPostProcessSettings.GrainJitter = 0;
	}

	if(!Family->EngineShowFlags.CameraImperfections)
	{
		FinalPostProcessSettings.BloomDirtMaskIntensity = 0;
	}

	if(!Family->EngineShowFlags.AmbientCubemap)
	{
		FinalPostProcessSettings.ContributingCubemaps.Empty();
	}

	if(!Family->EngineShowFlags.LensFlares)
	{
		FinalPostProcessSettings.LensFlareIntensity = 0;
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	{
		static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.ExposureOffset"));

		float Value = CVar->GetValueOnGameThread();
		FinalPostProcessSettings.AutoExposureBias += Value;
	}
#endif

	{
		static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.ScreenPercentage"));

		float Value = CVar->GetValueOnGameThread();

		if(Value >= 0.0)
		{
			if (!GEngine->IsStereoscopic3D())
			{
				FinalPostProcessSettings.ScreenPercentage = FMath::Min(FinalPostProcessSettings.ScreenPercentage, Value);
			}
			else
			{
				FinalPostProcessSettings.ScreenPercentage = Value;
			}
		}
	}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	{
		float Value = CVarSSRMaxRoughness.GetValueOnGameThread();

		if(Value >= 0.0f)
		{
			FinalPostProcessSettings.ScreenSpaceReflectionMaxRoughness = Value;
		}
	}

	{
		float Value = CVarSSRRoughnessScale.GetValueOnGameThread();

		if(Value >= 0.0f)
		{
			FinalPostProcessSettings.ScreenSpaceReflectionRoughnessScale = Value;
		}
	}
#endif

	{
		static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.AmbientOcclusionStaticFraction"));

		float Value = CVar->GetValueOnGameThread();

		if(Value >= 0.0)
		{
			FinalPostProcessSettings.AmbientOcclusionStaticFraction = Value;
		}
	}

	if(!Family->EngineShowFlags.ScreenPercentage)
	{
		FinalPostProcessSettings.ScreenPercentage = 100;
	}

	if(!Family->EngineShowFlags.AmbientOcclusion)
	{
		FinalPostProcessSettings.AmbientOcclusionIntensity = 0;
	}

	{
		static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.AmbientOcclusionRadiusScale"));

		float Scale = FMath::Clamp(CVar->GetValueOnGameThread(), 0.1f, 5.0f);
		
		FinalPostProcessSettings.AmbientOcclusionRadius *= Scale;
	}

	if(!Family->EngineShowFlags.Lighting || !Family->EngineShowFlags.GlobalIllumination)
	{
		FinalPostProcessSettings.IndirectLightingColor = FLinearColor(0,0,0,0);
		FinalPostProcessSettings.IndirectLightingIntensity = 0.0f;
	}

	// Anti-Aliasing
	{
		static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.PostProcessAAQuality")); 
		static auto* MobileHDRCvar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.MobileHDR"));

		int32 Quality = FMath::Clamp(CVar->GetValueOnGameThread(), 0, 6);

		if( !Family->EngineShowFlags.PostProcessing || !Family->EngineShowFlags.AntiAliasing || Quality <= 0
			// Disable antialiasing in GammaLDR mode to avoid jittering.
			|| (GRHIFeatureLevel == ERHIFeatureLevel::ES2 && MobileHDRCvar->GetValueOnGameThread() == 0))
		{
			FinalPostProcessSettings.AntiAliasingMethod = AAM_None;
		}

		if( FinalPostProcessSettings.AntiAliasingMethod == AAM_TemporalAA)
		{
			if( !Family->EngineShowFlags.TemporalAA || !Family->bRealtimeUpdate || Quality < 3 )
			{
				FinalPostProcessSettings.AntiAliasingMethod = AAM_FXAA;
			}
		}

	}

	if (AllowDebugViewmodes())
	{
		ConfigureBufferVisualizationSettings();
	}

	// Access the materials for the hires screenshot system
	static UMaterial* HighResScreenshotMaterial = NULL;
	static UMaterial* HighResScreenshotMaskMaterial = NULL;
	static UMaterial* HighResScreenshotCaptureRegionMaterial = NULL;
	static bool bHighResScreenshotMaterialsConfigured = false;

	if (!bHighResScreenshotMaterialsConfigured)
	{
		HighResScreenshotMaterial = LoadObject<UMaterial>(NULL, TEXT("/Engine/EngineMaterials/HighResScreenshot.HighResScreenshot"));
		HighResScreenshotMaskMaterial = LoadObject<UMaterial>(NULL, TEXT("/Engine/EngineMaterials/HighResScreenshotMask.HighResScreenshotMask"));
		HighResScreenshotCaptureRegionMaterial = LoadObject<UMaterial>(NULL, TEXT("/Engine/EngineMaterials/HighResScreenshotCaptureRegion.HighResScreenshotCaptureRegion"));

		if (HighResScreenshotMaterial)
		{
			HighResScreenshotMaterial->AddToRoot();
		}
		if (HighResScreenshotMaskMaterial)
		{
			HighResScreenshotMaskMaterial->AddToRoot(); 
		}
		if (HighResScreenshotCaptureRegionMaterial)
		{
			HighResScreenshotCaptureRegionMaterial->AddToRoot();
		}

		bHighResScreenshotMaterialsConfigured = true;
	}

	// Pass highres screenshot materials through post process settings
	FinalPostProcessSettings.HighResScreenshotMaterial = HighResScreenshotMaterial;
	FinalPostProcessSettings.HighResScreenshotMaskMaterial = HighResScreenshotMaskMaterial;
	FinalPostProcessSettings.HighResScreenshotCaptureRegionMaterial = NULL;

	FHighResScreenshotConfig& Config = GetHighResScreenshotConfig();

	// If the highres screenshot UI is open and we're not taking a highres screenshot this frame
	if (Config.bDisplayCaptureRegion && !GIsHighResScreenshot)
	{
		// Only enable the capture region effect if the capture region is different from the view rectangle
		if (Config.UnscaledCaptureRegion != ViewRect && Config.UnscaledCaptureRegion.Width() != -1 && Config.UnscaledCaptureRegion.Height() != -1 && State != NULL)
		{
			static const FName ParamName = "RegionRect";
			FLinearColor NormalizedCaptureRegion;

			// Normalize capture region into view rectangle
			NormalizedCaptureRegion.R = (float)Config.UnscaledCaptureRegion.Min.X / (float)ViewRect.Width();
			NormalizedCaptureRegion.G = (float)Config.UnscaledCaptureRegion.Min.Y / (float)ViewRect.Height();
			NormalizedCaptureRegion.B = (float)Config.UnscaledCaptureRegion.Max.X / (float)ViewRect.Width();
			NormalizedCaptureRegion.A = (float)Config.UnscaledCaptureRegion.Max.Y / (float)ViewRect.Height();

			// Get a MID for drawing this frame and push the capture region into the shader parameter
			FinalPostProcessSettings.HighResScreenshotCaptureRegionMaterial = State->GetReusableMID(HighResScreenshotCaptureRegionMaterial);
			FinalPostProcessSettings.HighResScreenshotCaptureRegionMaterial->SetVectorParameterValue(ParamName, NormalizedCaptureRegion);
		}
	}
}

void FSceneView::ConfigureBufferVisualizationSettings()
{
	bool bBufferDumpingRequired = (FScreenshotRequest::IsScreenshotRequested() || GIsHighResScreenshot || GIsDumpingMovie);
	bool bVisualizationRequired = Family->EngineShowFlags.VisualizeBuffer;
	
	if (bVisualizationRequired || bBufferDumpingRequired)
	{
		FinalPostProcessSettings.bBufferVisualizationDumpRequired = bBufferDumpingRequired;
		FinalPostProcessSettings.BufferVisualizationOverviewMaterials.Empty();

		if (bBufferDumpingRequired)
		{
			FinalPostProcessSettings.BufferVisualizationDumpBaseFilename = FPaths::GetBaseFilename(FScreenshotRequest::GetFilename(), false);
		}

		// Get the list of requested buffers from the console
		static IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.BufferVisualizationOverviewTargets"));
		FString SelectedMaterialNames = CVar->GetString();

		FBufferVisualizationData& BufferVisualizationData = GetBufferVisualizationData();

		if (BufferVisualizationData.IsDifferentToCurrentOverviewMaterialNames(SelectedMaterialNames))
		{
			FString Left, Right;

			// Update our record of the list of materials we've been asked to display
			BufferVisualizationData.SetCurrentOverviewMaterialNames(SelectedMaterialNames);
			BufferVisualizationData.GetOverviewMaterials().Empty();

			// Note - This will re-parse the list of names from the console variable every frame. It could be cached and only updated when
			// the variable value changes if this turns out to be a performance issue.
		
			// Extract each material name from the comma separated string
			while (SelectedMaterialNames.Len())
			{
				// Detect last entry in the list
				if (!SelectedMaterialNames.Split(TEXT(","), &Left, &Right))
				{
					Left = SelectedMaterialNames;
					Right = FString();
				}

				// Lookup this material from the list that was parsed out of the global ini file
				Left = Left.Trim();
				UMaterial* Material = BufferVisualizationData.GetMaterial(*Left);

				if (Material == NULL && Left.Len() > 0)
				{
					UE_LOG(LogBufferVisualization, Warning, TEXT("Unknown material '%s'"), *Left);
				}

				// Add this material into the material list in the post processing settings so that the render thread
				// can pick them up and draw them into the on-screen tiles
				BufferVisualizationData.GetOverviewMaterials().Add(Material);
				
				SelectedMaterialNames = Right;
			}
		}

		// Copy current material list into settings material list
		for (TArray<UMaterial*>::TConstIterator It = BufferVisualizationData.GetOverviewMaterials().CreateConstIterator(); It; ++It)
		{
			FinalPostProcessSettings.BufferVisualizationOverviewMaterials.Add(*It);
		}
	}
}

FSceneViewFamily::FSceneViewFamily( const ConstructionValues& CVS )
	:
	FamilySizeX(0),
	FamilySizeY(0),
	RenderTarget(CVS.RenderTarget),
	Scene(CVS.Scene),
	EngineShowFlags(CVS.EngineShowFlags),
	CurrentWorldTime(CVS.CurrentWorldTime),
	DeltaWorldTime(CVS.DeltaWorldTime),
	CurrentRealTime(CVS.CurrentRealTime),
	bRealtimeUpdate(CVS.bRealtimeUpdate),
	bDeferClear(CVS.bDeferClear),
	bResolveScene(CVS.bResolveScene),
	GammaCorrection(CVS.GammaCorrection)
{
	// If we do not pass a valid scene pointer then SetWorldTimes must be called to initialized with valid times.
	ensure(CVS.bTimesSet);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.RenderTimeFrozen")); 
	int32 Value = CVar->GetValueOnAnyThread();

	if(Value)
	{
		CurrentWorldTime = 0;
		CurrentRealTime = 0; 
	}
#endif

#if !WITH_EDITORONLY_DATA
	// Console shader compilers don't set instruction count, 
	// Also various console-specific rendering paths haven't been tested with shader complexity
	check(!EngineShowFlags.ShaderComplexity);
	check(!EngineShowFlags.StationaryLightOverlap);

#else

	// instead of checking IsGameWorld on rendering thread to see if we allow this flag to be disabled 
	// we force it on in the game thread.
	if(IsInGameThread())
	{
		if ( Scene && Scene->GetWorld() && Scene->GetWorld()->IsGameWorld() )
		{
			EngineShowFlags.LOD = 1;
		}
	}

	bDrawBaseInfo = true;
#endif
}

void FSceneViewFamily::ComputeFamilySize()
{
	// Calculate the screen extents of the view family.
	bool bInitializedExtents = false;
	float MaxFamilyX = 0;
	float MaxFamilyY = 0;

	for(int32 ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		const FSceneView* View = Views[ViewIndex];

		float FinalViewMaxX = (float)View->ViewRect.Max.X;
		float FinalViewMaxY = (float)View->ViewRect.Max.Y;

		// Derive the amount of scaling needed for screenpercentage from the scaled / unscaled rect
		const float XScale = FinalViewMaxX / (float)View->UnscaledViewRect.Max.X;
		const float YScale = FinalViewMaxY / (float)View->UnscaledViewRect.Max.Y;

		if(!bInitializedExtents)
		{
			// Note: using the unconstrained view rect to compute family size
			// In the case of constrained views (black bars) this means the scene render targets will fill the whole screen
			// Which is needed for ES2 paths where we render directly to the backbuffer, and the scene depth buffer has to match in size
			MaxFamilyX = View->UnconstrainedViewRect.Max.X * XScale;
			MaxFamilyY = View->UnconstrainedViewRect.Max.Y * YScale;
			bInitializedExtents = true;
		}
		else
		{
			MaxFamilyX = FMath::Max(MaxFamilyX, View->UnconstrainedViewRect.Max.X * XScale);
			MaxFamilyY = FMath::Max(MaxFamilyY, View->UnconstrainedViewRect.Max.Y * YScale);
		}

		// floating point imprecision could cause MaxFamilyX to be less than View->ViewRect.Max.X after integer truncation.
		// since this value controls rendertarget sizes, we don't want to create rendertargets smaller than the view size.
		MaxFamilyX = FMath::Max(MaxFamilyX, FinalViewMaxX);
		MaxFamilyY = FMath::Max(MaxFamilyY, FinalViewMaxY);
	}

	// We render to the actual position of the viewports so with black borders we need the max.
	// We could change it by rendering all to left top but that has implications for splitscreen. 
	FamilySizeX = FMath::Trunc(MaxFamilyX);
	FamilySizeY = FMath::Trunc(MaxFamilyY);	

	check(bInitializedExtents);
}

FSceneViewFamilyContext::~FSceneViewFamilyContext()
{
	// Cleanup the views allocated for this view family.
	for(int32 ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		delete Views[ViewIndex];
	}
}

