// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	TranslucentLighting.cpp: Translucent lighting implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "OneColorShader.h"
#include "LightRendering.h"
#include "SceneFilterRendering.h"
#include "ScreenRendering.h"
#include "AmbientCubemapParameters.h"

class FMaterial;

/** Whether to allow rendering translucency shadow depths. */
bool GUseTranslucencyShadowDepths = true;
 
int32 GUseTranslucentLightingVolumes = 1;
FAutoConsoleVariableRef CVarUseTranslucentLightingVolumes(
	TEXT("r.TranslucentLightingVolume"),
	GUseTranslucentLightingVolumes,
	TEXT("Whether to allow updating the translucent lighting volumes.\n")
	TEXT("0:off, otherwise on, default is 1"),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

int32 GUseIndirectLightingCacheInLightingVolume = 0;
FAutoConsoleVariableRef CUseIndirectLightingCacheInLightingVolume(
	TEXT("r.IndirectLightingCacheInLightingVolume"),
	GUseIndirectLightingCacheInLightingVolume,
	TEXT("Whether to use the indirect lighting cache to inject GI into the translucency lighting volume.  Causes extreme hitching at the moment."),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);


float GTranslucentVolumeMinFOV = 45;
static FAutoConsoleVariableRef CVarTranslucentVolumeMinFOV(
	TEXT("r.TranslucentVolumeMinFOV"),
	GTranslucentVolumeMinFOV,
	TEXT("Minimum FOV for translucent lighting volume.  Prevents popping in lighting when zooming in."),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

float GTranslucentVolumeFOVSnapFactor = 10;
static FAutoConsoleVariableRef CTranslucentVolumeFOVSnapFactor(
	TEXT("r.TranslucentVolumeFOVSnapFactor"),
	GTranslucentVolumeFOVSnapFactor,
	TEXT("FOV will be snapped to a factor of this before computing volume bounds. "),
	ECVF_Cheat | ECVF_RenderThreadSafe
	);

int32 GUseTranslucencyVolumeBlur = 1;
FAutoConsoleVariableRef CVarUseTranslucentLightingVolumeBlur(
	TEXT("r.TranslucencyVolumeBlur"),
	GUseTranslucencyVolumeBlur,
	TEXT("Whether to blur the translucent lighting volumes.\n")
	TEXT("0:off, otherwise on, default is 1"),
	ECVF_Scalability | ECVF_RenderThreadSafe
	);

int32 GTranslucencyLightingVolumeDim = 64;
FAutoConsoleVariableRef CVarTranslucencyLightingVolumeDim(
	TEXT("r.TranslucencyLightingVolumeDim"),
	GTranslucencyLightingVolumeDim,
	TEXT("Dimensions of the volume textures used for translucency lighting.  Larger textures result in higher resolution but lower performance."),
	ECVF_Scalability | ECVF_RenderThreadSafe
	);

static TAutoConsoleVariable<float> CVarTranslucencyLightingVolumeInnerDistance(
	TEXT("r.TranslucencyLightingVolumeInnerDistance"),
	1500.0f,
	TEXT("Distance from the camera that the first volume cascade should end"),
	ECVF_RenderThreadSafe);

static TAutoConsoleVariable<float> CVarTranslucencyLightingVolumeOuterDistance(
	TEXT("r.TranslucencyLightingVolumeOuterDistance"),
	5000.0f,
	TEXT("Distance from the camera that the second volume cascade should end"),
	ECVF_RenderThreadSafe);

void FViewInfo::CalcTranslucencyLightingVolumeBounds(FBox* InOutCascadeBoundsArray, int32 NumCascades) const
{
	for (int32 CascadeIndex = 0; CascadeIndex < NumCascades; CascadeIndex++)
	{
		float InnerDistance = CVarTranslucencyLightingVolumeInnerDistance.GetValueOnRenderThread();
		float OuterDistance = CVarTranslucencyLightingVolumeOuterDistance.GetValueOnRenderThread();

		const float FrustumStartDistance = CascadeIndex == 0 ? 0 : InnerDistance;
		const float FrustumEndDistance = CascadeIndex == 0 ? InnerDistance : OuterDistance;

		float FOV = PI / 4.0f;
		float AspectRatio = 1.0f;

		if (IsPerspectiveProjection())
		{
			// Derive FOV and aspect ratio from the perspective projection matrix
			FOV = FMath::Atan(1.0f / ShadowViewMatrices.ProjMatrix.M[0][0]);
			// Clamp to prevent shimmering when zooming in
			FOV = FMath::Max(FOV, GTranslucentVolumeMinFOV * (float)PI / 180.0f);
			const float RoundFactorRadians = GTranslucentVolumeFOVSnapFactor * (float)PI / 180.0f;
			// Round up to a fixed factor
			// This causes the volume lighting to make discreet jumps as the FOV animates, instead of slowly crawling over a long period
			FOV = FOV + RoundFactorRadians - FMath::Fmod(FOV, RoundFactorRadians);
			AspectRatio = ShadowViewMatrices.ProjMatrix.M[1][1] / ShadowViewMatrices.ProjMatrix.M[0][0];
		}

		const float StartHorizontalLength = FrustumStartDistance * FMath::Tan(FOV);
		const FVector StartCameraRightOffset = ShadowViewMatrices.ViewMatrix.GetColumn(0) * StartHorizontalLength;
		const float StartVerticalLength = StartHorizontalLength / AspectRatio;
		const FVector StartCameraUpOffset = ShadowViewMatrices.ViewMatrix.GetColumn(1) * StartVerticalLength;

		const float EndHorizontalLength = FrustumEndDistance * FMath::Tan(FOV);
		const FVector EndCameraRightOffset = ShadowViewMatrices.ViewMatrix.GetColumn(0) * EndHorizontalLength;
		const float EndVerticalLength = EndHorizontalLength / AspectRatio;
		const FVector EndCameraUpOffset = ShadowViewMatrices.ViewMatrix.GetColumn(1) * EndVerticalLength;

		FVector SplitVertices[8];
		SplitVertices[0] = ShadowViewMatrices.ViewOrigin + GetViewDirection() * FrustumStartDistance + StartCameraRightOffset + StartCameraUpOffset;
		SplitVertices[1] = ShadowViewMatrices.ViewOrigin + GetViewDirection() * FrustumStartDistance + StartCameraRightOffset - StartCameraUpOffset;
		SplitVertices[2] = ShadowViewMatrices.ViewOrigin + GetViewDirection() * FrustumStartDistance - StartCameraRightOffset + StartCameraUpOffset;
		SplitVertices[3] = ShadowViewMatrices.ViewOrigin + GetViewDirection() * FrustumStartDistance - StartCameraRightOffset - StartCameraUpOffset;

		SplitVertices[4] = ShadowViewMatrices.ViewOrigin + GetViewDirection() * FrustumEndDistance + EndCameraRightOffset + EndCameraUpOffset;
		SplitVertices[5] = ShadowViewMatrices.ViewOrigin + GetViewDirection() * FrustumEndDistance + EndCameraRightOffset - EndCameraUpOffset;
		SplitVertices[6] = ShadowViewMatrices.ViewOrigin + GetViewDirection() * FrustumEndDistance - EndCameraRightOffset + EndCameraUpOffset;
		SplitVertices[7] = ShadowViewMatrices.ViewOrigin + GetViewDirection() * FrustumEndDistance - EndCameraRightOffset - EndCameraUpOffset;

		FVector Center(0,0,0);
		// Weight the far vertices more so that the bounding sphere will be further from the camera
		// This minimizes wasted shadowmap space behind the viewer
		const float FarVertexWeightScale = 10.0f;
		for (int32 VertexIndex = 0; VertexIndex < 8; VertexIndex++)
		{
			const float Weight = VertexIndex > 3 ? 1 / (4.0f + 4.0f / FarVertexWeightScale) : 1 / (4.0f + 4.0f * FarVertexWeightScale);
			Center += SplitVertices[VertexIndex] * Weight;
		}

		float RadiusSquared = 0;
		for (int32 VertexIndex = 0; VertexIndex < 8; VertexIndex++)
		{
			RadiusSquared = FMath::Max(RadiusSquared, (Center - SplitVertices[VertexIndex]).SizeSquared());
		}

		FSphere SphereBounds(Center, FMath::Sqrt(RadiusSquared));

		// Snap the center to a multiple of the volume dimension for stability
		SphereBounds.Center.X = SphereBounds.Center.X - FMath::Fmod(SphereBounds.Center.X, SphereBounds.W * 2 * TranslucentVolumeGISratchDownsampleFactor / GTranslucencyLightingVolumeDim);
		SphereBounds.Center.Y = SphereBounds.Center.Y - FMath::Fmod(SphereBounds.Center.Y, SphereBounds.W * 2 * TranslucentVolumeGISratchDownsampleFactor / GTranslucencyLightingVolumeDim);
		SphereBounds.Center.Z = SphereBounds.Center.Z - FMath::Fmod(SphereBounds.Center.Z, SphereBounds.W * 2 * TranslucentVolumeGISratchDownsampleFactor / GTranslucencyLightingVolumeDim);

		InOutCascadeBoundsArray[CascadeIndex] = FBox(SphereBounds.Center - SphereBounds.W, SphereBounds.Center + SphereBounds.W);
	}
}

/**
 * Vertex shader used to render shadow maps for translucency.
 */
class FTranslucencyShadowDepthVS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FTranslucencyShadowDepthVS,MeshMaterial);
public:

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return IsTranslucentBlendMode(Material->GetBlendMode()) && IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	FTranslucencyShadowDepthVS() {}
	FTranslucencyShadowDepthVS(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer) :
		FMeshMaterialShader(Initializer)
	{
		ShadowParameters.Bind(Initializer.ParameterMap);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FMeshMaterialShader::Serialize(Ar);
		Ar << ShadowParameters;
		return bShaderHasOutdatedParameters;
	}

	void SetParameters(
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FSceneView& View,
		const FProjectedShadowInfo* ShadowInfo
		)
	{
		FMeshMaterialShader::SetParameters(GetVertexShader(), MaterialRenderProxy, *MaterialRenderProxy->GetMaterial(GRHIFeatureLevel), View, ESceneRenderTargetsMode::DontSet);
		ShadowParameters.SetVertexShader(this, View, ShadowInfo, MaterialRenderProxy);
	}

	void SetMesh(const FVertexFactory* VertexFactory,const FSceneView& View,const FPrimitiveSceneProxy* Proxy,const FMeshBatchElement& BatchElement)
	{
		FMeshMaterialShader::SetMesh(GetVertexShader(),VertexFactory,View,Proxy,BatchElement);
	}

private:
	FShadowDepthShaderParameters ShadowParameters;
};

enum ETranslucencyShadowDepthShaderMode
{
	TranslucencyShadowDepth_PerspectiveCorrect,
	TranslucencyShadowDepth_Standard,
};

template <ETranslucencyShadowDepthShaderMode ShaderMode>
class TTranslucencyShadowDepthVS : public FTranslucencyShadowDepthVS
{
	DECLARE_SHADER_TYPE(TTranslucencyShadowDepthVS,MeshMaterial);
public:

	TTranslucencyShadowDepthVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FTranslucencyShadowDepthVS(Initializer)
	{
	}

	TTranslucencyShadowDepthVS() {}

	static void ModifyCompilationEnvironment( EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment )
	{
		FTranslucencyShadowDepthVS::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("PERSPECTIVE_CORRECT_DEPTH"), (uint32)(ShaderMode == TranslucencyShadowDepth_PerspectiveCorrect ? 1 : 0));
	}
};

IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TTranslucencyShadowDepthVS<TranslucencyShadowDepth_PerspectiveCorrect>,TEXT("TranslucentShadowDepthShaders"),TEXT("MainVS"),SF_Vertex);
IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TTranslucencyShadowDepthVS<TranslucencyShadowDepth_Standard>,TEXT("TranslucentShadowDepthShaders"),TEXT("MainVS"),SF_Vertex);

/**
 * Pixel shader used for accumulating translucency layer densities
 */
class FTranslucencyShadowDepthPS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FTranslucencyShadowDepthPS,MeshMaterial);
public:

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return IsTranslucentBlendMode(Material->GetBlendMode()) && IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	FTranslucencyShadowDepthPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FMeshMaterialShader(Initializer)
	{
		InvMaxSubjectDepth.Bind(Initializer.ParameterMap,TEXT("InvMaxSubjectDepth"));
		TranslucentShadowStartOffset.Bind(Initializer.ParameterMap,TEXT("TranslucentShadowStartOffset"));
		TranslucencyProjectionParameters.Bind(Initializer.ParameterMap);
	}

	FTranslucencyShadowDepthPS() {}

	void SetParameters(
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FSceneView& View,
		const FProjectedShadowInfo* ShadowInfo
		)
	{
		//@todo - scene depth can be bound by the material for use in depth fades
		// This is incorrect when rendering a shadowmap as it's not from the camera's POV
		// Set the scene depth texture to something safe when rendering shadow depths
		FMeshMaterialShader::SetParameters(GetPixelShader(),MaterialRenderProxy,*MaterialRenderProxy->GetMaterial(GRHIFeatureLevel),View,ESceneRenderTargetsMode::NonSceneAlignedPass);

		SetShaderValue(GetPixelShader(),InvMaxSubjectDepth,1.0f / ShadowInfo->MaxSubjectDepth);

		const float LocalToWorldScale = ShadowInfo->ParentSceneInfo->Proxy->GetLocalToWorld().GetScaleVector().GetMax();
		const float TranslucentShadowStartOffsetValue = MaterialRenderProxy->GetMaterial(GRHIFeatureLevel)->GetTranslucentShadowStartOffset() * LocalToWorldScale;
		SetShaderValue(GetPixelShader(),TranslucentShadowStartOffset, TranslucentShadowStartOffsetValue / (ShadowInfo->MaxSubjectZ - ShadowInfo->MinSubjectZ));
		TranslucencyProjectionParameters.Set(this);
	}

	void SetMesh(const FVertexFactory* VertexFactory,const FSceneView& View,const FPrimitiveSceneProxy* Proxy,const FMeshBatchElement& BatchElement)
	{
		FMeshMaterialShader::SetMesh(GetPixelShader(),VertexFactory,View,Proxy,BatchElement);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FMeshMaterialShader::Serialize(Ar);
		Ar << InvMaxSubjectDepth;
		Ar << TranslucentShadowStartOffset;
		Ar << TranslucencyProjectionParameters;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderParameter InvMaxSubjectDepth;
	FShaderParameter TranslucentShadowStartOffset;
	FTranslucencyShadowProjectionShaderParameters TranslucencyProjectionParameters;
};

template <ETranslucencyShadowDepthShaderMode ShaderMode>
class TTranslucencyShadowDepthPS : public FTranslucencyShadowDepthPS
{
	DECLARE_SHADER_TYPE(TTranslucencyShadowDepthPS,MeshMaterial);
public:

	TTranslucencyShadowDepthPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FTranslucencyShadowDepthPS(Initializer)
	{
	}

	TTranslucencyShadowDepthPS() {}

	static void ModifyCompilationEnvironment( EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment )
	{
		FTranslucencyShadowDepthPS::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("PERSPECTIVE_CORRECT_DEPTH"), (uint32)(ShaderMode == TranslucencyShadowDepth_PerspectiveCorrect ? 1 : 0));
	}
};

IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TTranslucencyShadowDepthPS<TranslucencyShadowDepth_PerspectiveCorrect>,TEXT("TranslucentShadowDepthShaders"),TEXT("MainOpacityPS"),SF_Pixel);
IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TTranslucencyShadowDepthPS<TranslucencyShadowDepth_Standard>,TEXT("TranslucentShadowDepthShaders"),TEXT("MainOpacityPS"),SF_Pixel);

/**
 * Drawing policy used to create Fourier opacity maps
 */
class FTranslucencyShadowDepthDrawingPolicy : public FMeshDrawingPolicy
{
public:

	FTranslucencyShadowDepthDrawingPolicy(
		const FVertexFactory* InVertexFactory,
		const FMaterialRenderProxy* InMaterialRenderProxy,
		const FMaterial& InMaterialResource,
		bool bInDirectionalLight
		):
		FMeshDrawingPolicy(InVertexFactory,InMaterialRenderProxy,InMaterialResource,false,false)
	{
		const bool bUsePerspectiveCorrectShadowDepths = !bInDirectionalLight;

		if (bUsePerspectiveCorrectShadowDepths)
		{
			VertexShader = InMaterialResource.GetShader<TTranslucencyShadowDepthVS<TranslucencyShadowDepth_PerspectiveCorrect> >(InVertexFactory->GetType());
			PixelShader = InMaterialResource.GetShader<TTranslucencyShadowDepthPS<TranslucencyShadowDepth_PerspectiveCorrect> >(InVertexFactory->GetType());
		}
		else
		{
			VertexShader = InMaterialResource.GetShader<TTranslucencyShadowDepthVS<TranslucencyShadowDepth_Standard> >(InVertexFactory->GetType());
			PixelShader = InMaterialResource.GetShader<TTranslucencyShadowDepthPS<TranslucencyShadowDepth_Standard> >(InVertexFactory->GetType());
		}
	}

	void DrawShared(const FSceneView* View,FBoundShaderStateRHIParamRef BoundShaderState) const
	{
		// Set the shared mesh resources.
		FMeshDrawingPolicy::DrawShared(View);

		// Set the actual shader & vertex declaration state
		RHISetBoundShaderState(BoundShaderState);

		VertexShader->SetParameters(MaterialRenderProxy, *View, PolicyShadowInfo);
		PixelShader->SetParameters(MaterialRenderProxy, *View, PolicyShadowInfo);
	}

	/** 
	* Create bound shader state using the vertex decl from the mesh draw policy
	* as well as the shaders needed to draw the mesh
	* @return new bound shader state object
	*/
	FBoundShaderStateRHIRef CreateBoundShaderState()
	{
		return RHICreateBoundShaderState(
			FMeshDrawingPolicy::GetVertexDeclaration(), 
			VertexShader->GetVertexShader(),
			NULL, 
			NULL,
			PixelShader->GetPixelShader(),
			NULL);
	}

	void SetMeshRenderState(
		const FSceneView& View,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const FMeshBatch& Mesh,
		int32 BatchElementIndex,
		bool bBackFace,
		const ElementDataType& ElementData
		) const
	{
		const FMeshBatchElement& BatchElement = Mesh.Elements[BatchElementIndex];
		VertexShader->SetMesh(VertexFactory,View,PrimitiveSceneProxy,BatchElement);
		PixelShader->SetMesh(VertexFactory,View,PrimitiveSceneProxy,BatchElement);

		FMeshDrawingPolicy::SetMeshRenderState(View,PrimitiveSceneProxy,Mesh,BatchElementIndex,bBackFace,ElementData);
	}

	static const FProjectedShadowInfo* PolicyShadowInfo;

private:
	FTranslucencyShadowDepthVS* VertexShader;
	FTranslucencyShadowDepthPS* PixelShader;
};

const FProjectedShadowInfo* FTranslucencyShadowDepthDrawingPolicy::PolicyShadowInfo = NULL;

class FTranslucencyShadowDepthDrawingPolicyFactory
{
public:

	enum { bAllowSimpleElements = false };
	struct ContextType 
	{
		ContextType(bool bInDirectionalLight) :
			bDirectionalLight(bInDirectionalLight)
		{}

		bool bDirectionalLight;
	};

	static bool DrawDynamicMesh(
		const FSceneView& View,
		ContextType DrawingContext,
		const FMeshBatch& Mesh,
		bool bBackFace,
		bool bPreFog,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		FHitProxyId HitProxyId
		)
	{
		bool bDirty = false;

		if (Mesh.CastShadow)
		{
			const FMaterialRenderProxy* MaterialRenderProxy = Mesh.MaterialRenderProxy;
			const EBlendMode BlendMode = MaterialRenderProxy->GetMaterial(GRHIFeatureLevel)->GetBlendMode();

			// Only render translucent meshes into the Fourier opacity maps
			if (IsTranslucentBlendMode(BlendMode))
			{
				FTranslucencyShadowDepthDrawingPolicy DrawingPolicy(Mesh.VertexFactory, MaterialRenderProxy, *MaterialRenderProxy->GetMaterial(GRHIFeatureLevel), DrawingContext.bDirectionalLight);
				DrawingPolicy.DrawShared(&View, DrawingPolicy.CreateBoundShaderState());

				for (int32 BatchElementIndex = 0; BatchElementIndex < Mesh.Elements.Num(); BatchElementIndex++)
				{
					DrawingPolicy.SetMeshRenderState(View,PrimitiveSceneProxy,Mesh,BatchElementIndex,bBackFace,FMeshDrawingPolicy::ElementDataType());
					DrawingPolicy.DrawMesh(Mesh,BatchElementIndex);
				}
				bDirty = true;
			}
		}
	
		return bDirty;
	}

	static bool DrawStaticMesh(
		const FViewInfo& View,
		ContextType DrawingContext,
		const FStaticMesh& StaticMesh,
		bool bPreFog,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		FHitProxyId HitProxyId
		)
	{
		bool bDirty = false;

		bDirty |= DrawDynamicMesh(
			View,
			DrawingContext,
			StaticMesh,
			false,
			bPreFog,
			PrimitiveSceneProxy,
			HitProxyId
			);

		return bDirty;
	}

	static bool IsMaterialIgnored(const FMaterialRenderProxy* MaterialRenderProxy) 
	{ 
		return !IsTranslucentBlendMode(MaterialRenderProxy->GetMaterial(GRHIFeatureLevel)->GetBlendMode());
	}
};

/** Renders shadow maps for translucent primitives. */
void FProjectedShadowInfo::RenderTranslucencyDepths(FDeferredShadingSceneRenderer* SceneRenderer)
{
	checkSlow(!bWholeSceneShadow);
	SCOPE_CYCLE_COUNTER(STAT_RenderWholeSceneShadowDepthsTime);

	// Choose an arbitrary view where this shadow's subject is relevant.
	FViewInfo* FoundView = NULL;
	for(int32 ViewIndex = 0;ViewIndex < SceneRenderer->Views.Num();ViewIndex++)
	{
		FViewInfo* CheckView = &SceneRenderer->Views[ViewIndex];
		const FVisibleLightViewInfo& VisibleLightViewInfo = CheckView->VisibleLightInfos[LightSceneInfo->Id];
		FPrimitiveViewRelevance ViewRel = VisibleLightViewInfo.ProjectedShadowViewRelevanceMap[ShadowId];
		if (ViewRel.bShadowRelevance)
		{
			FoundView = CheckView;
			break;
		}
	}
	check(FoundView);
	
	// Backup properties of the view that we will override
	TUniformBufferRef<FViewUniformShaderParameters> OriginalUniformBuffer = FoundView->UniformBuffer;
	FMatrix OriginalViewMatrix = FoundView->ViewMatrices.ViewMatrix;

	// Override the view matrix so that billboarding primitives will be aligned to the light
	FoundView->ViewMatrices.ViewMatrix = ShadowViewMatrix;
	FBox VolumeBounds[TVC_MAX];
	FoundView->UniformBuffer = FoundView->CreateUniformBuffer(
		ShadowViewMatrix, 
		VolumeBounds,
		TVC_MAX);
		
	// Prevent materials from getting overridden during shadow casting and in viewmodes like lighting only
	// Lighting only should only affect the material used with direct lighting, not the indirect lighting
	FoundView->bForceShowMaterials = true;

	FTranslucencyShadowDepthDrawingPolicy::PolicyShadowInfo = this;

	{
#if WANTS_DRAW_MESH_EVENTS
		FString EventName;
		GetShadowTypeNameForDrawEvent(EventName);
		SCOPED_DRAW_EVENTF(EventShadowDepthActor, DEC_SCENE_ITEMS, *EventName);
#endif

		FTextureRHIParamRef RenderTargets[2] =
		{
			GSceneRenderTargets.TranslucencyShadowTransmission[0]->GetRenderTargetItem().TargetableTexture,
			GSceneRenderTargets.TranslucencyShadowTransmission[1]->GetRenderTargetItem().TargetableTexture
		};
		RHISetRenderTargets(ARRAY_COUNT(RenderTargets), RenderTargets, FTextureRHIParamRef(), 0, NULL);

		// Clear the shadow and its border
		RHISetViewport(
			X,
			Y,
			0.0f,
			(X + SHADOW_BORDER*2 + ResolutionX),
			(Y + SHADOW_BORDER*2 + ResolutionY),
			1.0f
			);

		FLinearColor ClearColors[2] = {FLinearColor(0,0,0,0), FLinearColor(0,0,0,0)};
		DrawClearQuadMRT(true, ARRAY_COUNT(ClearColors), ClearColors, false, 1.0f, false, 0);

		// Set the viewport for the shadow.
		RHISetViewport(
			(X + SHADOW_BORDER),
			(Y + SHADOW_BORDER),
			0.0f,
			(X + SHADOW_BORDER + ResolutionX),
			(Y + SHADOW_BORDER + ResolutionY),
			1.0f
			);

		RHISetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
		RHISetBlendState(TStaticBlendState<
			CW_RGBA, BO_Add, BF_One, BF_One, BO_Add, BF_One, BF_One,
			CW_RGBA, BO_Add, BF_One, BF_One, BO_Add, BF_One, BF_One>::GetRHI());

		TDynamicPrimitiveDrawer<FTranslucencyShadowDepthDrawingPolicyFactory> OpacityDrawer(FoundView, FTranslucencyShadowDepthDrawingPolicyFactory::ContextType(bDirectionalLight), true);

		for (int32 PrimitiveIndex = 0; PrimitiveIndex < SubjectTranslucentPrimitives.Num(); PrimitiveIndex++)
		{
			const FPrimitiveSceneInfo* PrimitiveSceneInfo = SubjectTranslucentPrimitives[PrimitiveIndex];
			int32 PrimitiveId = PrimitiveSceneInfo->GetIndex();
			FPrimitiveViewRelevance ViewRelevance = FoundView->PrimitiveViewRelevanceMap[PrimitiveId];

			if (!ViewRelevance.bInitializedThisFrame)
			{
				// Compute the subject primitive's view relevance since it wasn't cached
				ViewRelevance = PrimitiveSceneInfo->Proxy->GetViewRelevance(FoundView);
			}

			if(ViewRelevance.bDrawRelevance)
			{
				if (ViewRelevance.bDynamicRelevance)
				{
					OpacityDrawer.SetPrimitive(PrimitiveSceneInfo->Proxy);
					PrimitiveSceneInfo->Proxy->DrawDynamicElements(&OpacityDrawer, FoundView);
				}

				if (ViewRelevance.bStaticRelevance)
				{
					for (int32 MeshIndex = 0; MeshIndex < PrimitiveSceneInfo->StaticMeshes.Num(); MeshIndex++)
					{
						FTranslucencyShadowDepthDrawingPolicyFactory::DrawStaticMesh(
							*FoundView,
							FTranslucencyShadowDepthDrawingPolicyFactory::ContextType(bDirectionalLight),
							PrimitiveSceneInfo->StaticMeshes[MeshIndex],
							true,
							PrimitiveSceneInfo->Proxy,
							FHitProxyId());
					}
				}
			}
		}
	}

	// Restore overridden properties
	FTranslucencyShadowDepthDrawingPolicy::PolicyShadowInfo = NULL;

	FoundView->bForceShowMaterials = false;
	FoundView->UniformBuffer = OriginalUniformBuffer;
	FoundView->ViewMatrices.ViewMatrix = OriginalViewMatrix;
}

IMPLEMENT_SHADER_TYPE(,FWriteToSliceGS,TEXT("TranslucentLightingShaders"),TEXT("WriteToSliceMainGS"),SF_Geometry);
IMPLEMENT_SHADER_TYPE(,FWriteToSliceVS,TEXT("TranslucentLightingShaders"),TEXT("WriteToSliceMainVS"),SF_Vertex);

/** Pixel shader used to filter a single volume lighting cascade. */
class FFilterTranslucentVolumePS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FFilterTranslucentVolumePS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform) 
	{ 
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4); 
	}

	FFilterTranslucentVolumePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
		TexelSize.Bind(Initializer.ParameterMap, TEXT("TexelSize"));
		TranslucencyLightingVolumeAmbient.Bind(Initializer.ParameterMap, TEXT("TranslucencyLightingVolumeAmbient"));
		TranslucencyLightingVolumeAmbientSampler.Bind(Initializer.ParameterMap, TEXT("TranslucencyLightingVolumeAmbientSampler"));
		TranslucencyLightingVolumeDirectional.Bind(Initializer.ParameterMap, TEXT("TranslucencyLightingVolumeDirectional"));
		TranslucencyLightingVolumeDirectionalSampler.Bind(Initializer.ParameterMap, TEXT("TranslucencyLightingVolumeDirectionalSampler"));
	}
	FFilterTranslucentVolumePS() {}

	void SetParameters(const FViewInfo& View, int32 VolumeCascadeIndex)
	{
		FGlobalShader::SetParameters(GetPixelShader(), View);

		SetShaderValue(GetPixelShader(), TexelSize, 1.0f / GTranslucencyLightingVolumeDim);
		SetTextureParameter(
			GetPixelShader(), 
			TranslucencyLightingVolumeAmbient, 
			TranslucencyLightingVolumeAmbientSampler, 
			TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), 
			GSceneRenderTargets.TranslucencyLightingVolumeAmbient[VolumeCascadeIndex]->GetRenderTargetItem().ShaderResourceTexture);

		SetTextureParameter(
			GetPixelShader(), 
			TranslucencyLightingVolumeDirectional, 
			TranslucencyLightingVolumeDirectionalSampler, 
			TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI(), 
			GSceneRenderTargets.TranslucencyLightingVolumeDirectional[VolumeCascadeIndex]->GetRenderTargetItem().ShaderResourceTexture);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << TexelSize;
		Ar << TranslucencyLightingVolumeAmbient;
		Ar << TranslucencyLightingVolumeAmbientSampler;
		Ar << TranslucencyLightingVolumeDirectional;
		Ar << TranslucencyLightingVolumeDirectionalSampler;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderParameter TexelSize;
	FShaderResourceParameter TranslucencyLightingVolumeAmbient;
	FShaderResourceParameter TranslucencyLightingVolumeAmbientSampler;
	FShaderResourceParameter TranslucencyLightingVolumeDirectional;
	FShaderResourceParameter TranslucencyLightingVolumeDirectionalSampler;
};

IMPLEMENT_SHADER_TYPE(,FFilterTranslucentVolumePS,TEXT("TranslucentLightingShaders"),TEXT("FilterMainPS"),SF_Pixel);

/** Shader parameters needed to inject direct lighting into a volume. */
class FTranslucentInjectParameters
{
public:

	void Bind(const FShaderParameterMap& ParameterMap)
	{
		WorldToShadowMatrix.Bind(ParameterMap,TEXT("WorldToShadowMatrix"));
		ShadowmapMinMax.Bind(ParameterMap,TEXT("ShadowmapMinMax"));
		VolumeCascadeIndex.Bind(ParameterMap,TEXT("VolumeCascadeIndex"));
	}

	template<typename ShaderRHIParamRef>
	void Set(
		const ShaderRHIParamRef ShaderRHI, 
		FShader* Shader, 
		const FViewInfo& View, 
		const FLightSceneInfo* LightSceneInfo, 
		const FProjectedShadowInfo* ShadowMap, 
		int32 VolumeCascadeIndexValue, 
		bool bShadowed) const
	{
		SetDeferredLightParameters(ShaderRHI, Shader->GetUniformBufferParameter<FDeferredLightUniformStruct>(), LightSceneInfo, View);

		if (bShadowed)
		{
			FVector4 ShadowmapMinMaxValue;
			FMatrix WorldToShadowMatrixValue = ShadowMap->GetWorldToShadowMatrix(ShadowmapMinMaxValue);

			SetShaderValue(ShaderRHI, WorldToShadowMatrix, WorldToShadowMatrixValue);
			SetShaderValue(ShaderRHI, ShadowmapMinMax, ShadowmapMinMaxValue);
		}

		SetShaderValue(ShaderRHI, VolumeCascadeIndex, VolumeCascadeIndexValue);
	}

	/** Serializer. */ 
	friend FArchive& operator<<(FArchive& Ar,FTranslucentInjectParameters& P)
	{
		Ar << P.WorldToShadowMatrix;
		Ar << P.ShadowmapMinMax;
		Ar << P.VolumeCascadeIndex;
		return Ar;
	}

private:

	FShaderParameter WorldToShadowMatrix;
	FShaderParameter ShadowmapMinMax;
	FShaderParameter VolumeCascadeIndex;
};

/** Pixel shader used to accumulate per-object translucent shadows into a volume texture. */
class FTranslucentObjectShadowingPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FTranslucentObjectShadowingPS,Global);
public:

	static void ModifyCompilationEnvironment( EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment )
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("INJECTION_PIXEL_SHADER"), 1);
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	FTranslucentObjectShadowingPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
		TranslucencyProjectionParameters.Bind(Initializer.ParameterMap);
		TranslucentInjectParameters.Bind(Initializer.ParameterMap);
	}
	FTranslucentObjectShadowingPS() {}

	void SetParameters(const FViewInfo& View, const FLightSceneInfo* LightSceneInfo, const FProjectedShadowInfo* ShadowMap, int32 VolumeCascadeIndexValue)
	{
		FGlobalShader::SetParameters(GetPixelShader(), View);
		TranslucencyProjectionParameters.Set(this);
		TranslucentInjectParameters.Set(GetPixelShader(), this, View, LightSceneInfo, ShadowMap, VolumeCascadeIndexValue, true);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << TranslucencyProjectionParameters;
		Ar << TranslucentInjectParameters;
		return bShaderHasOutdatedParameters;
	}

private:
	FTranslucencyShadowProjectionShaderParameters TranslucencyProjectionParameters;
	FTranslucentInjectParameters TranslucentInjectParameters;
};

IMPLEMENT_SHADER_TYPE(,FTranslucentObjectShadowingPS,TEXT("TranslucentLightingShaders"),TEXT("PerObjectShadowingMainPS"),SF_Pixel);

/** Shader that adds direct lighting contribution from the given light to the current volume lighting cascade. */
template<ELightComponentType InjectionType, bool bShadowed, bool bApplyLightFunction, bool bInverseSquared>
class TTranslucentLightingInjectPS : public FMaterialShader
{
	DECLARE_SHADER_TYPE(TTranslucentLightingInjectPS,Material);
public:

	static void ModifyCompilationEnvironment( EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment )
	{
		FMaterialShader::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("RADIAL_ATTENUATION"), (uint32)(InjectionType != LightType_Directional));
		OutEnvironment.SetDefine(TEXT("INJECTION_PIXEL_SHADER"), 1);
		OutEnvironment.SetDefine(TEXT("DYNAMICALLY_SHADOWED"), (uint32)bShadowed);
		OutEnvironment.SetDefine(TEXT("APPLY_LIGHT_FUNCTION"), (uint32)bApplyLightFunction);
		OutEnvironment.SetDefine(TEXT("INVERSE_SQUARED_FALLOFF"), (uint32)bInverseSquared);
	}

	/**
	  * Makes sure only shaders for materials that are explicitly flagged
	  * as 'UsedAsLightFunction' in the Material Editor gets compiled into
	  * the shader cache.
	  */
	static bool ShouldCache(EShaderPlatform Platform, const FMaterial* Material)
	{
		return (Material->IsLightFunction() || Material->IsSpecialEngineMaterial()) && IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	TTranslucentLightingInjectPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FMaterialShader(Initializer)
	{
		DepthBiasParameters.Bind(Initializer.ParameterMap, TEXT("DepthBiasParameters"));
		CascadeBounds.Bind(Initializer.ParameterMap, TEXT("CascadeBounds"));
		InnerCascadeBounds.Bind(Initializer.ParameterMap, TEXT("InnerCascadeBounds"));
		ClippingPlanes.Bind(Initializer.ParameterMap, TEXT("ClippingPlanes"));
		ShadowInjectParams.Bind(Initializer.ParameterMap, TEXT("ShadowInjectParams"));
		SpotlightMask.Bind(Initializer.ParameterMap, TEXT("SpotlightMask"));
		ShadowDepthTexture.Bind(Initializer.ParameterMap, TEXT("ShadowDepthTexture"));
		ShadowDepthTextureSampler.Bind(Initializer.ParameterMap, TEXT("ShadowDepthTextureSampler"));
		OnePassShadowParameters.Bind(Initializer.ParameterMap);
		LightFunctionParameters.Bind(Initializer.ParameterMap);
		TranslucentInjectParameters.Bind(Initializer.ParameterMap);
		LightFunctionWorldToLight.Bind(Initializer.ParameterMap, TEXT("LightFunctionWorldToLight"));
	}
	TTranslucentLightingInjectPS() {}

	// @param InnerSplitIndex which CSM shadow map level, INDEX_NONE if no directional light
	// @param VolumeCascadeIndexValue which volume we render to
	void SetParameters(
		const FViewInfo& View, 
		const FLightSceneInfo* LightSceneInfo, 
		const FMaterialRenderProxy* MaterialProxy, 
		const FProjectedShadowInfo* ShadowMap, 
		int32 InnerSplitIndex, 
		int32 VolumeCascadeIndexValue)
	{
		check(ShadowMap || !bShadowed);
		
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FMaterialShader::SetParameters(ShaderRHI, MaterialProxy, *MaterialProxy->GetMaterial(GRHIFeatureLevel), View, false, ESceneRenderTargetsMode::SetTextures);
		
		FSphere ShadowBounds = ShadowMap ? ShadowMap->ShadowBounds : FSphere(FVector(0,0,0), HALF_WORLD_MAX);
		SetShaderValue(ShaderRHI, CascadeBounds, ShadowBounds);

		FSphere InnerBounds(0);
		// default to ignore the plane
		FVector4 Planes[2] = { FVector4(0, 0, 0, -1), FVector4(0, 0, 0, -1) };
		// .zw:DistanceFadeMAD to use MAD for efficiency in the shader, default to ignore the plane
		FVector4 ShadowInjectParamValue(1, 1, 0, 0);

		if (InnerSplitIndex >= 0)
		{
			FShadowCascadeSettings ShadowCascadeSettings;

			// todo: optimize, not all computed data is needed
			InnerBounds = LightSceneInfo->Proxy->GetShadowSplitBounds(View, InnerSplitIndex, &ShadowCascadeSettings);

			// near cascade plane
			{
				ShadowInjectParamValue.X = ShadowCascadeSettings.SplitNearFadeRegion == 0 ? 1.0f : 1.0f / ShadowCascadeSettings.SplitNearFadeRegion;
				Planes[0] = FVector4((FVector)(ShadowCascadeSettings.NearFrustumPlane),  -ShadowCascadeSettings.NearFrustumPlane.W);
			}

			int32 CascadeCount = LightSceneInfo->Proxy->GetNumViewDependentWholeSceneShadows(View);

			// far cascade plane
			if(InnerSplitIndex != CascadeCount - 1)
			{
				ShadowInjectParamValue.Y = 1.0f / ShadowCascadeSettings.SplitFarFadeRegion;
				Planes[1] = FVector4((FVector)(ShadowCascadeSettings.FarFrustumPlane), -ShadowCascadeSettings.FarFrustumPlane.W);
			}

			const FVector2D FadeParams = LightSceneInfo->Proxy->GetDirectionalLightDistanceFadeParameters();

			// setup constants for the MAD in shader
			ShadowInjectParamValue.Z = FadeParams.Y;
			ShadowInjectParamValue.W = -FadeParams.X * FadeParams.Y;
		}

		SetShaderValue(ShaderRHI, ShadowInjectParams, ShadowInjectParamValue);
		SetShaderValue(ShaderRHI, InnerCascadeBounds, InnerBounds);

		SetShaderValueArray(ShaderRHI, ClippingPlanes, Planes, ARRAY_COUNT(Planes));
	

		//@todo - needs to be a permutation to reduce shadow filtering work
		SetShaderValue(ShaderRHI, SpotlightMask, (LightSceneInfo->Proxy->GetLightType() == LightType_Spot ? 1.0f : 0.0f));

		if (bShadowed)
		{
			SetShaderValue(ShaderRHI, DepthBiasParameters, FVector2D(ShadowMap->GetShaderDepthBias(), 1.0f / (ShadowMap->MaxSubjectZ - ShadowMap->MinSubjectZ)));

			SetTextureParameter(
				ShaderRHI,
				ShadowDepthTexture,
				ShadowDepthTextureSampler,
				TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
				GSceneRenderTargets.GetShadowDepthZTexture()
				);
		}

		if (bShadowed && InjectionType == LightType_Point)
		{
			OnePassShadowParameters.Set(ShaderRHI, ShadowMap);
		}

		LightFunctionParameters.Set(ShaderRHI, LightSceneInfo, 1);
		TranslucentInjectParameters.Set(ShaderRHI, this, View, LightSceneInfo, ShadowMap, VolumeCascadeIndexValue, bShadowed);

		if (LightFunctionWorldToLight.IsBound())
		{
			const FVector Scale = LightSceneInfo->Proxy->GetLightFunctionScale();
			// Switch x and z so that z of the user specified scale affects the distance along the light direction
			const FVector InverseScale = FVector( 1.f / Scale.Z, 1.f / Scale.Y, 1.f / Scale.X );
			const FMatrix WorldToLight = LightSceneInfo->Proxy->GetWorldToLight() * FScaleMatrix(FVector(InverseScale));	

			SetShaderValue(ShaderRHI, LightFunctionWorldToLight, WorldToLight);
		}
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FMaterialShader::Serialize(Ar);
		Ar << DepthBiasParameters;
		Ar << CascadeBounds;
		Ar << InnerCascadeBounds;
		Ar << ClippingPlanes;
		Ar << ShadowInjectParams;
		Ar << SpotlightMask;
		Ar << ShadowDepthTexture;
		Ar << ShadowDepthTextureSampler;
		Ar << OnePassShadowParameters;
		Ar << LightFunctionParameters;
		Ar << TranslucentInjectParameters;
		Ar << LightFunctionWorldToLight;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderParameter DepthBiasParameters;
	FShaderParameter CascadeBounds;
	FShaderParameter InnerCascadeBounds;
	FShaderParameter ClippingPlanes;
	FShaderParameter ShadowInjectParams;
	FShaderParameter SpotlightMask;
	FShaderResourceParameter ShadowDepthTexture;
	FShaderResourceParameter ShadowDepthTextureSampler;
	FOnePassPointShadowProjectionShaderParameters OnePassShadowParameters;
	FLightFunctionSharedParameters LightFunctionParameters;
	FTranslucentInjectParameters TranslucentInjectParameters;
	FShaderParameter LightFunctionWorldToLight;
};

#define IMPLEMENT_INJECTION_PIXELSHADER_TYPE(LightType,bShadowed,bApplyLightFunction,bInverseSquared) \
	typedef TTranslucentLightingInjectPS<LightType,bShadowed,bApplyLightFunction,bInverseSquared> TTranslucentLightingInjectPS##LightType##bShadowed##bApplyLightFunction##bInverseSquared; \
	IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TTranslucentLightingInjectPS##LightType##bShadowed##bApplyLightFunction##bInverseSquared,TEXT("TranslucentLightInjectionShaders"),TEXT("InjectMainPS"),SF_Pixel);

/** Versions with a light function. */
IMPLEMENT_INJECTION_PIXELSHADER_TYPE(LightType_Directional,true,true,false); 
IMPLEMENT_INJECTION_PIXELSHADER_TYPE(LightType_Directional,false,true,false); 
IMPLEMENT_INJECTION_PIXELSHADER_TYPE(LightType_Point,true,true,true); 
IMPLEMENT_INJECTION_PIXELSHADER_TYPE(LightType_Point,false,true,true); 
IMPLEMENT_INJECTION_PIXELSHADER_TYPE(LightType_Point,true,true,false); 
IMPLEMENT_INJECTION_PIXELSHADER_TYPE(LightType_Point,false,true,false); 

/** Versions without a light function. */
IMPLEMENT_INJECTION_PIXELSHADER_TYPE(LightType_Directional,true,false,false); 
IMPLEMENT_INJECTION_PIXELSHADER_TYPE(LightType_Directional,false,false,false); 
IMPLEMENT_INJECTION_PIXELSHADER_TYPE(LightType_Point,true,false,true); 
IMPLEMENT_INJECTION_PIXELSHADER_TYPE(LightType_Point,false,false,true); 
IMPLEMENT_INJECTION_PIXELSHADER_TYPE(LightType_Point,true,false,false); 
IMPLEMENT_INJECTION_PIXELSHADER_TYPE(LightType_Point,false,false,false); 

/** Vertex buffer used for rendering into a volume texture. */
class FVolumeRasterizeVertexBuffer : public FVertexBuffer
{
public:

	virtual void InitRHI()
	{
		// Used as a non-indexed triangle strip, so 4 vertices per quad
		const uint32 Size = 4 * sizeof(FScreenVertex);
		VertexBufferRHI = RHICreateVertexBuffer(Size, NULL, BUF_Static);
		void* Buffer = RHILockVertexBuffer(VertexBufferRHI, 0, Size, RLM_WriteOnly);
		FScreenVertex* DestVertex = (FScreenVertex*)Buffer;

		// Setup a full - render target quad
		// A viewport and UVScaleBias will be used to implement rendering to a sub region
		DestVertex[0].Position = FVector2D(1, -GProjectionSignY);
		DestVertex[0].UV = FVector2D(1, 1);
		DestVertex[1].Position = FVector2D(1, GProjectionSignY);
		DestVertex[1].UV = FVector2D(1, 0);
		DestVertex[2].Position = FVector2D(-1, -GProjectionSignY);
		DestVertex[2].UV = FVector2D(0, 1);
		DestVertex[3].Position = FVector2D(-1, GProjectionSignY);
		DestVertex[3].UV = FVector2D(0, 0);

		RHIUnlockVertexBuffer(VertexBufferRHI);      
	}
};

TGlobalResource<FVolumeRasterizeVertexBuffer> GVolumeRasterizeVertexBuffer;

/** Draws a quad per volume texture slice to the subregion of the volume texture specified. */
void RasterizeToVolumeTexture(FVolumeBounds VolumeBounds)
{
	// Setup the viewport to only render to the given bounds
	RHISetViewport(VolumeBounds.MinX, VolumeBounds.MinY, 0, VolumeBounds.MaxX, VolumeBounds.MaxY, 0);
	RHISetStreamSource(0, GVolumeRasterizeVertexBuffer.VertexBufferRHI, sizeof(FScreenVertex), 0);
	const int32 NumInstances = VolumeBounds.MaxZ - VolumeBounds.MinZ;
	// Render a quad per slice affected by the given bounds
	RHIDrawPrimitive(PT_TriangleStrip, 0, 2, NumInstances);
}

/** Helper function that clears the given volume texture render targets. */
template<int32 NumRenderTargets>
void ClearVolumeTextures(const FTextureRHIParamRef* RenderTargets, const FLinearColor* ClearColors)
{
	RHISetRenderTargets(NumRenderTargets, RenderTargets, FTextureRHIRef(), 0, NULL);

	static FGlobalBoundShaderState VolumeClearBoundShaderState;

	// Currently using a manual clear, which is ~10x faster than a hardware clear of the volume textures on AMD PC GPU's
	if (false)
	{
		RHIClearMRT(true, NumRenderTargets, ClearColors, false, 0, false, 0, FIntRect());
	}
	else
	{
		RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
		RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());
		RHISetBlendState(TStaticBlendState<>::GetRHI());

		const FVolumeBounds VolumeBounds(GTranslucencyLightingVolumeDim);
		TShaderMapRef<FWriteToSliceVS> VertexShader(GetGlobalShaderMap());
		TShaderMapRef<FWriteToSliceGS> GeometryShader(GetGlobalShaderMap());
		TShaderMapRef<TOneColorPixelShaderMRT<NumRenderTargets> > PixelShader(GetGlobalShaderMap());
		
		SetGlobalBoundShaderState(VolumeClearBoundShaderState, GScreenVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader, *GeometryShader);

		VertexShader->SetParameters(VolumeBounds, GTranslucencyLightingVolumeDim);
		GeometryShader->SetParameters(VolumeBounds);

		FLinearColor ShaderClearColors[MaxSimultaneousRenderTargets];
		FMemory::MemZero(ShaderClearColors);

		for (int32 i = 0; i < NumRenderTargets; i++)
		{
			ShaderClearColors[i] = ClearColors[i];
		}

		SetShaderValueArray(PixelShader->GetPixelShader(), PixelShader->ColorParameter, ShaderClearColors, NumRenderTargets);

		RasterizeToVolumeTexture(VolumeBounds);
	}
}

void FDeferredShadingSceneRenderer::ClearTranslucentVolumeLighting()
{
	if (GUseTranslucentLightingVolumes)
	{
		SCOPED_DRAW_EVENT(ClearTranslucentVolumeLighting, DEC_SCENE_ITEMS);

		// Clear all volume textures in the same draw with MRT, which is faster than individually

		checkAtCompileTime(TVC_MAX == 2, OnlyExpectingTwoTranslucencyLightingCascades);
		FTextureRHIParamRef RenderTargets[4];
		RenderTargets[0] = GSceneRenderTargets.TranslucencyLightingVolumeAmbient[0]->GetRenderTargetItem().TargetableTexture;
		RenderTargets[1] = GSceneRenderTargets.TranslucencyLightingVolumeDirectional[0]->GetRenderTargetItem().TargetableTexture;
		RenderTargets[2] = GSceneRenderTargets.TranslucencyLightingVolumeAmbient[1]->GetRenderTargetItem().TargetableTexture;
		RenderTargets[3] = GSceneRenderTargets.TranslucencyLightingVolumeDirectional[1]->GetRenderTargetItem().TargetableTexture;

		FLinearColor ClearColors[4];
		ClearColors[0] = FLinearColor(0, 0, 0, 0);
		ClearColors[1] = FLinearColor(0, 0, 0, 0);
		ClearColors[2] = FLinearColor(0, 0, 0, 0);
		ClearColors[3] = FLinearColor(0, 0, 0, 0);

		ClearVolumeTextures<ARRAY_COUNT(RenderTargets)>(RenderTargets, ClearColors);
	}
}


/** Encapsulates a pixel shader that is adding ambient cubemap to the volume. */
class FInjectAmbientCubemapPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FInjectAmbientCubemapPS, Global);

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	/** Default constructor. */
	FInjectAmbientCubemapPS() {}

public:
	FCubemapShaderParameters CubemapShaderParameters;

	/** Initialization constructor. */
	FInjectAmbientCubemapPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		CubemapShaderParameters.Bind(Initializer.ParameterMap);
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << CubemapShaderParameters;
		return bShaderHasOutdatedParameters;
	}

	void SetParameters(const FViewInfo& View, const FFinalPostProcessSettings::FCubemapEntry& CubemapEntry)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(ShaderRHI, View);

		CubemapShaderParameters.SetParameters(ShaderRHI, CubemapEntry);
	}
};

IMPLEMENT_SHADER_TYPE(,FInjectAmbientCubemapPS,TEXT("TranslucentLightingShaders"),TEXT("InjectAmbientCubemapMainPS"),SF_Pixel);

void FDeferredShadingSceneRenderer::InjectAmbientCubemapTranslucentVolumeLighting()
{
	//@todo - support multiple views
	const FViewInfo& View = Views[0];

	if (GUseTranslucentLightingVolumes && View.FinalPostProcessSettings.ContributingCubemaps.Num() && !IsSimpleDynamicLightingEnabled())
	{
		SCOPED_DRAW_EVENT(InjectAmbientCubemapTranslucentVolumeLighting, DEC_SCENE_ITEMS);

		RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
		RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());
		RHISetBlendState(TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_One, BO_Add, BF_One, BF_One>::GetRHI());

		const FVolumeBounds VolumeBounds(GTranslucencyLightingVolumeDim);
		
		for (int32 VolumeCascadeIndex = 0; VolumeCascadeIndex < TVC_MAX; VolumeCascadeIndex++)
		{
			// we don't update the directional volume (could be a HQ option)
			RHISetRenderTarget(GSceneRenderTargets.TranslucencyLightingVolumeAmbient[VolumeCascadeIndex]->GetRenderTargetItem().TargetableTexture, FTextureRHIRef());

			TShaderMapRef<FWriteToSliceVS> VertexShader(GetGlobalShaderMap());
			TShaderMapRef<FWriteToSliceGS> GeometryShader(GetGlobalShaderMap());
			TShaderMapRef<FInjectAmbientCubemapPS> PixelShader(GetGlobalShaderMap());

			static FGlobalBoundShaderState BoundShaderState;
						
			SetGlobalBoundShaderState(BoundShaderState, GScreenVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader, *GeometryShader);

			VertexShader->SetParameters(VolumeBounds, GTranslucencyLightingVolumeDim);
			GeometryShader->SetParameters(VolumeBounds);

			uint32 Count = View.FinalPostProcessSettings.ContributingCubemaps.Num();
			for(uint32 i = 0; i < Count; ++i)
			{
				const FFinalPostProcessSettings::FCubemapEntry& CubemapEntry = View.FinalPostProcessSettings.ContributingCubemaps[i];

				PixelShader->SetParameters(View, CubemapEntry);

				RasterizeToVolumeTexture(VolumeBounds);
			}
			
			RHICopyToResolveTarget(GSceneRenderTargets.TranslucencyLightingVolumeAmbient[VolumeCascadeIndex]->GetRenderTargetItem().TargetableTexture,
				GSceneRenderTargets.TranslucencyLightingVolumeAmbient[VolumeCascadeIndex]->GetRenderTargetItem().ShaderResourceTexture, true, FResolveParams());
		}
	}
}


/** Pixel shader used to filter a single volume lighting cascade. */
class FCompositeGIForTranslucencyPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FCompositeGIForTranslucencyPS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform) 
	{ 
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4); 
	}

	FCompositeGIForTranslucencyPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
		IndirectLightingCacheTexture.Bind(Initializer.ParameterMap, TEXT("IndirectLightingCacheTexture"));
		IndirectLightingCacheTexture1.Bind(Initializer.ParameterMap, TEXT("IndirectLightingCacheTexture1"));
		IndirectLightingCacheTexture2.Bind(Initializer.ParameterMap, TEXT("IndirectLightingCacheTexture2"));
		IndirectLightingCacheTextureSampler.Bind(Initializer.ParameterMap, TEXT("IndirectLightingCacheTextureSampler"));
		IndirectLightingCacheTexture1Sampler.Bind(Initializer.ParameterMap, TEXT("IndirectLightingCacheTexture1Sampler"));
		IndirectLightingCacheTexture2Sampler.Bind(Initializer.ParameterMap, TEXT("IndirectLightingCacheTexture2Sampler"));
		IndirectlightingCacheAdd.Bind(Initializer.ParameterMap, TEXT("IndirectlightingCacheAdd"));
		IndirectlightingCacheScale.Bind(Initializer.ParameterMap, TEXT("IndirectlightingCacheScale"));
		VolumeCascadeIndex.Bind(Initializer.ParameterMap,TEXT("VolumeCascadeIndex"));
	}
	FCompositeGIForTranslucencyPS() {}

	void SetParameters(const FViewInfo& View, const FSceneViewState* ViewState, int32 VolumeCascadeIndexValue)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(ShaderRHI, View);

		SetShaderValue(ShaderRHI, VolumeCascadeIndex, VolumeCascadeIndexValue);

		const FIndirectLightingCacheAllocation* LightingAllocation = ViewState->TranslucencyLightingCacheAllocations[VolumeCascadeIndexValue];

		if (LightingAllocation && LightingAllocation->IsValid())
		{
			SetShaderValue(ShaderRHI, IndirectlightingCacheAdd, LightingAllocation->Add);
			SetShaderValue(ShaderRHI, IndirectlightingCacheScale, LightingAllocation->Scale);

			FScene* Scene = (FScene*)View.Family->Scene;
			FIndirectLightingCache& LightingCache = Scene->IndirectLightingCache;

			if (LightingCache.IsInitialized())
			{
				SetTextureParameter(
					ShaderRHI,
					IndirectLightingCacheTexture,
					IndirectLightingCacheTextureSampler,
					TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
					LightingCache.GetTexture0().ShaderResourceTexture
					);

				SetTextureParameter(
					ShaderRHI,
					IndirectLightingCacheTexture1,
					IndirectLightingCacheTexture1Sampler,
					TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
					LightingCache.GetTexture1().ShaderResourceTexture
					);

				SetTextureParameter(
					ShaderRHI,
					IndirectLightingCacheTexture2,
					IndirectLightingCacheTexture2Sampler,
					TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
					LightingCache.GetTexture2().ShaderResourceTexture
					);
			}
		}
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << IndirectLightingCacheTexture;
		Ar << IndirectLightingCacheTexture1;
		Ar << IndirectLightingCacheTexture2;
		Ar << IndirectLightingCacheTextureSampler;
		Ar << IndirectLightingCacheTexture1Sampler;
		Ar << IndirectLightingCacheTexture2Sampler;
		Ar << IndirectlightingCacheAdd;
		Ar << IndirectlightingCacheScale;
		Ar << VolumeCascadeIndex;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderResourceParameter IndirectLightingCacheTexture;
	FShaderResourceParameter IndirectLightingCacheTexture1;
	FShaderResourceParameter IndirectLightingCacheTexture2;
	FShaderResourceParameter IndirectLightingCacheTextureSampler;
	FShaderResourceParameter IndirectLightingCacheTexture1Sampler;
	FShaderResourceParameter IndirectLightingCacheTexture2Sampler;
	FShaderParameter IndirectlightingCacheAdd;
	FShaderParameter IndirectlightingCacheScale;
	FShaderParameter VolumeCascadeIndex;
};

IMPLEMENT_SHADER_TYPE(,FCompositeGIForTranslucencyPS,TEXT("TranslucentLightingShaders"),TEXT("CompositeGIMainPS"),SF_Pixel);

void FDeferredShadingSceneRenderer::CompositeIndirectTranslucentVolumeLighting()
{
	bool bAnyViewAllowsIndirectLightingCache = false;

	for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
	{
		bAnyViewAllowsIndirectLightingCache |= Views[ViewIndex].Family->EngineShowFlags.IndirectLightingCache;
	}

	if (GUseTranslucentLightingVolumes 
		&& GUseIndirectLightingCacheInLightingVolume
		&& bAnyViewAllowsIndirectLightingCache)
	{
		SCOPED_DRAW_EVENT(CompositeIndirectTranslucentVolumeLighting, DEC_SCENE_ITEMS);

		RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
		RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());
		RHISetBlendState(TStaticBlendState<CW_RGBA, BO_Add, BF_One, BF_One, BO_Add, BF_One, BF_One>::GetRHI());

		const FVolumeBounds VolumeBounds(GTranslucencyLightingVolumeDim);

		for (int32 VolumeCascadeIndex = 0; VolumeCascadeIndex < TVC_MAX; VolumeCascadeIndex++)
		{
			FTextureRHIParamRef RenderTargets[2];
			RenderTargets[0] = GSceneRenderTargets.TranslucencyLightingVolumeAmbient[VolumeCascadeIndex]->GetRenderTargetItem().TargetableTexture;
			RenderTargets[1] = GSceneRenderTargets.TranslucencyLightingVolumeDirectional[VolumeCascadeIndex]->GetRenderTargetItem().TargetableTexture;

			RHISetRenderTargets(ARRAY_COUNT(RenderTargets), RenderTargets, FTextureRHIRef(), 0, NULL);


			//@todo - support multiple views
			const FViewInfo& View = Views[0];
			const FSceneViewState* ViewState = (const FSceneViewState*)View.State;

			if (ViewState 
				&& View.Family->EngineShowFlags.IndirectLightingCache
				&& View.Family->EngineShowFlags.GlobalIllumination)
			{
				TShaderMapRef<FWriteToSliceVS> VertexShader(GetGlobalShaderMap());
				TShaderMapRef<FWriteToSliceGS> GeometryShader(GetGlobalShaderMap());
				TShaderMapRef<FCompositeGIForTranslucencyPS> PixelShader(GetGlobalShaderMap());

				static FGlobalBoundShaderState BoundShaderState;
				SetGlobalBoundShaderState(BoundShaderState, GScreenVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader, *GeometryShader);

				PixelShader->SetParameters(View, ViewState, VolumeCascadeIndex);
				VertexShader->SetParameters(VolumeBounds, GTranslucencyLightingVolumeDim);
				GeometryShader->SetParameters(VolumeBounds);

				RasterizeToVolumeTexture(VolumeBounds);

				RHICopyToResolveTarget(GSceneRenderTargets.TranslucencyLightingVolumeAmbient[VolumeCascadeIndex]->GetRenderTargetItem().TargetableTexture,
					GSceneRenderTargets.TranslucencyLightingVolumeAmbient[VolumeCascadeIndex]->GetRenderTargetItem().ShaderResourceTexture, true, FResolveParams());
				RHICopyToResolveTarget(GSceneRenderTargets.TranslucencyLightingVolumeDirectional[VolumeCascadeIndex]->GetRenderTargetItem().TargetableTexture,
					GSceneRenderTargets.TranslucencyLightingVolumeDirectional[VolumeCascadeIndex]->GetRenderTargetItem().ShaderResourceTexture, true, FResolveParams());
			}
		}
	}
}

void FDeferredShadingSceneRenderer::ClearTranslucentVolumePerObjectShadowing()
{
	if (GUseTranslucentLightingVolumes)
	{
		SCOPED_DRAW_EVENT(ClearTranslucentVolumePerLightShadowing, DEC_SCENE_ITEMS);

		checkAtCompileTime(TVC_MAX == 2, OnlyExpectingTwoTranslucencyLightingCascades);
		FTextureRHIParamRef RenderTargets[2];
		RenderTargets[0] = GSceneRenderTargets.GetTranslucencyVolumeAmbient(TVC_Inner)->GetRenderTargetItem().TargetableTexture;
		RenderTargets[1] = GSceneRenderTargets.GetTranslucencyVolumeDirectional(TVC_Inner)->GetRenderTargetItem().TargetableTexture;

		FLinearColor ClearColors[2];
		ClearColors[0] = FLinearColor(1, 1, 1, 1);
		ClearColors[1] = FLinearColor(1, 1, 1, 1);

		ClearVolumeTextures<ARRAY_COUNT(RenderTargets)>(RenderTargets, ClearColors);
	}
}

/** Calculates volume texture bounds for the given light in the given translucent lighting volume cascade. */
FVolumeBounds CalculateLightVolumeBounds(const FSphere& LightBounds, const FViewInfo& View, int32 VolumeCascadeIndex, bool bDirectionalLight)
{
	FVolumeBounds VolumeBounds;

	if (bDirectionalLight)
	{
		VolumeBounds = FVolumeBounds(GTranslucencyLightingVolumeDim);
	}
	else
	{
		// Determine extents in the volume texture
		const FVector MinPosition = (LightBounds.Center - LightBounds.W - View.TranslucencyLightingVolumeMin[VolumeCascadeIndex]) / View.TranslucencyVolumeVoxelSize[VolumeCascadeIndex];
		const FVector MaxPosition = (LightBounds.Center + LightBounds.W - View.TranslucencyLightingVolumeMin[VolumeCascadeIndex]) / View.TranslucencyVolumeVoxelSize[VolumeCascadeIndex];

		VolumeBounds.MinX = FMath::Max(FMath::Trunc(MinPosition.X), 0);
		VolumeBounds.MinY = FMath::Max(FMath::Trunc(MinPosition.Y), 0);
		VolumeBounds.MinZ = FMath::Max(FMath::Trunc(MinPosition.Z), 0);

		VolumeBounds.MaxX = FMath::Min(FMath::Trunc(MaxPosition.X) + 1, GTranslucencyLightingVolumeDim);
		VolumeBounds.MaxY = FMath::Min(FMath::Trunc(MaxPosition.Y) + 1, GTranslucencyLightingVolumeDim);
		VolumeBounds.MaxZ = FMath::Min(FMath::Trunc(MaxPosition.Z) + 1, GTranslucencyLightingVolumeDim);
	}

	return VolumeBounds;
}

FGlobalBoundShaderState ObjectShadowingBoundShaderState;

void FDeferredShadingSceneRenderer::AccumulateTranslucentVolumeObjectShadowing(const FProjectedShadowInfo* InProjectedShadowInfo, bool bClearVolume)
{
	const FLightSceneInfo* LightSceneInfo = InProjectedShadowInfo->LightSceneInfo;

	if (bClearVolume)
	{
		ClearTranslucentVolumePerObjectShadowing();
	}

	if (GUseTranslucentLightingVolumes)
	{
		SCOPED_DRAW_EVENT(AccumulateTranslucentVolumeShadowing, DEC_SCENE_ITEMS);

		RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
		RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());

		// Modulate the contribution of multiple object shadows in rgb
		RHISetBlendState(TStaticBlendState<CW_RGB, BO_Add, BF_DestColor, BF_Zero>::GetRHI());

		// Inject into each volume cascade
		for (int32 VolumeCascadeIndex = 0; VolumeCascadeIndex < TVC_MAX; VolumeCascadeIndex++)
		{
			//@todo - support multiple views
			const FViewInfo& View = Views[0];
			const bool bDirectionalLight = LightSceneInfo->Proxy->GetLightType() == LightType_Directional;
			const FVolumeBounds VolumeBounds = CalculateLightVolumeBounds(LightSceneInfo->Proxy->GetBoundingSphere(), View, VolumeCascadeIndex, bDirectionalLight);

			if (VolumeBounds.IsValid())
			{
				FTextureRHIParamRef RenderTarget;

				if (VolumeCascadeIndex == 0)
				{
					RenderTarget = GSceneRenderTargets.GetTranslucencyVolumeAmbient(TVC_Inner)->GetRenderTargetItem().TargetableTexture;
				}
				else
				{
					RenderTarget = GSceneRenderTargets.GetTranslucencyVolumeDirectional(TVC_Inner)->GetRenderTargetItem().TargetableTexture;
				}

				RHISetRenderTarget(RenderTarget, FTextureRHIRef());

				TShaderMapRef<FWriteToSliceVS> VertexShader(GetGlobalShaderMap());
				TShaderMapRef<FWriteToSliceGS> GeometryShader(GetGlobalShaderMap());
				TShaderMapRef<FTranslucentObjectShadowingPS> PixelShader(GetGlobalShaderMap());

				SetGlobalBoundShaderState(ObjectShadowingBoundShaderState, GScreenVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader, *GeometryShader);
				VertexShader->SetParameters(VolumeBounds, GTranslucencyLightingVolumeDim);
				GeometryShader->SetParameters(VolumeBounds);
				PixelShader->SetParameters(View, LightSceneInfo, InProjectedShadowInfo, VolumeCascadeIndex);
				
				RasterizeToVolumeTexture(VolumeBounds);

				RHICopyToResolveTarget(GSceneRenderTargets.GetTranslucencyVolumeAmbient((ETranslucencyVolumeCascade)VolumeCascadeIndex)->GetRenderTargetItem().TargetableTexture,
					GSceneRenderTargets.GetTranslucencyVolumeAmbient((ETranslucencyVolumeCascade)VolumeCascadeIndex)->GetRenderTargetItem().ShaderResourceTexture, true, FResolveParams());
			}
		}
	}
}

/**
 * Helper function for finding and setting the right version of TTranslucentLightingInjectPS given template parameters.
 * @param MaterialProxy must not be 0
 * @param InnerSplitIndex todo: get from ShadowMap, INDEX_NONE if no directional light
 */
template<ELightComponentType InjectionType, bool bShadowed>
void SetInjectionShader(
	const FViewInfo& View, 
	const FMaterialRenderProxy* MaterialProxy,
	const FLightSceneInfo* LightSceneInfo, 
	const FProjectedShadowInfo* ShadowMap, 
	int32 InnerSplitIndex, 
	int32 VolumeCascadeIndexValue,
	FWriteToSliceVS* VertexShader,
	FWriteToSliceGS* GeometryShader,
	bool bApplyLightFunction,
	bool bInverseSquared)
{
	check(ShadowMap || !bShadowed);

	const FMaterialShaderMap* MaterialShaderMap = MaterialProxy->GetMaterial(GRHIFeatureLevel)->GetRenderingThreadShaderMap();
	FMaterialShader* PixelShader = NULL;

	const bool Directional = InjectionType == LightType_Directional;

	if (bApplyLightFunction)
	{
		if( bInverseSquared )
		{
			auto InjectionPixelShader = MaterialShaderMap->GetShader< TTranslucentLightingInjectPS<InjectionType, bShadowed, true, true && !Directional> >();

			check(InjectionPixelShader);
			PixelShader = InjectionPixelShader;
		}
		else
		{
			auto InjectionPixelShader = MaterialShaderMap->GetShader< TTranslucentLightingInjectPS<InjectionType, bShadowed, true, false> >();

			check(InjectionPixelShader);
			PixelShader = InjectionPixelShader;
		}
	}
	else
	{
		if( bInverseSquared )
		{
			auto InjectionPixelShader = MaterialShaderMap->GetShader< TTranslucentLightingInjectPS<InjectionType, bShadowed, false, true && !Directional> >();

			check(InjectionPixelShader);
			PixelShader = InjectionPixelShader;
		}
		else
		{
			auto InjectionPixelShader = MaterialShaderMap->GetShader< TTranslucentLightingInjectPS<InjectionType, bShadowed, false, false> >();

			check(InjectionPixelShader);
			PixelShader = InjectionPixelShader;
		}
	}
	
	FBoundShaderStateRHIRef& BoundShaderState = LightSceneInfo->TranslucentInjectBoundShaderState[InjectionType][bShadowed][bApplyLightFunction][bInverseSquared];
	const FMaterialShaderMap*& CachedShaderMap = LightSceneInfo->TranslucentInjectCachedShaderMaps[InjectionType][bShadowed][bApplyLightFunction][bInverseSquared];

	// Recreate the bound shader state if the shader map has changed since the last cache
	// This can happen due to async shader compiling
	if (!IsValidRef(BoundShaderState) || CachedShaderMap != MaterialShaderMap)
	{
		CachedShaderMap = MaterialShaderMap;
		BoundShaderState = RHICreateBoundShaderState(GScreenVertexDeclaration.VertexDeclarationRHI, VertexShader->GetVertexShader(), FHullShaderRHIRef(), FDomainShaderRHIRef(), PixelShader->GetPixelShader(), GeometryShader->GetGeometryShader());
	}

	RHISetBoundShaderState(BoundShaderState);

	// Now shader is set, bind parameters
	if (bApplyLightFunction)
	{
		if( bInverseSquared )
		{
			auto InjectionPixelShader = MaterialShaderMap->GetShader< TTranslucentLightingInjectPS<InjectionType, bShadowed, true, true && !Directional> >();
			check(InjectionPixelShader);
			InjectionPixelShader->SetParameters(View, LightSceneInfo, MaterialProxy, ShadowMap, InnerSplitIndex, VolumeCascadeIndexValue);
		}
		else
		{
			auto InjectionPixelShader = MaterialShaderMap->GetShader< TTranslucentLightingInjectPS<InjectionType, bShadowed, true, false> >();
			check(InjectionPixelShader);
			InjectionPixelShader->SetParameters(View, LightSceneInfo, MaterialProxy, ShadowMap, InnerSplitIndex, VolumeCascadeIndexValue);
		}
	}
	else
	{
		if( bInverseSquared )
		{
			auto InjectionPixelShader = MaterialShaderMap->GetShader< TTranslucentLightingInjectPS<InjectionType, bShadowed, false, true && !Directional> >();
			check(InjectionPixelShader);
			InjectionPixelShader->SetParameters(View, LightSceneInfo, MaterialProxy, ShadowMap, InnerSplitIndex, VolumeCascadeIndexValue);
		}
		else
		{
			auto InjectionPixelShader = MaterialShaderMap->GetShader< TTranslucentLightingInjectPS<InjectionType, bShadowed, false, false> >();
			check(InjectionPixelShader);
			InjectionPixelShader->SetParameters(View, LightSceneInfo, MaterialProxy, ShadowMap, InnerSplitIndex, VolumeCascadeIndexValue);
		}
	}
}

/** 
 * Information about a light to be injected.
 * Cached in this struct to avoid recomputing multiple times (multiple cascades).
 */
struct FTranslucentLightInjectionData
{
	// must not be 0
	const FLightSceneInfo* LightSceneInfo;
	// can be 0
	const FProjectedShadowInfo* ProjectedShadowInfo;
	//
	bool bApplyLightFunction;
	// must not be 0
	const FMaterialRenderProxy* LightFunctionMaterialProxy;
};

/**
 * Adds a light to LightInjectionData if it should be injected into the translucent volume, and caches relevant information in a FTranslucentLightInjectionData.
 * @param InProjectedShadowInfo is 0 for unshadowed lights
 */
static void AddLightForInjection(
	FDeferredShadingSceneRenderer& SceneRenderer,
	const FLightSceneInfo& LightSceneInfo, 
	const FProjectedShadowInfo* InProjectedShadowInfo,
	TArray<FTranslucentLightInjectionData, SceneRenderingAllocator>& LightInjectionData)
{
	if (LightSceneInfo.Proxy->AffectsTranslucentLighting())
	{
		const FVisibleLightInfo& VisibleLightInfo = SceneRenderer.VisibleLightInfos[LightSceneInfo.Id];

		const bool bApplyLightFunction = (SceneRenderer.ViewFamily.EngineShowFlags.LightFunctions &&
			LightSceneInfo.Proxy->GetLightFunctionMaterial() && 
			LightSceneInfo.Proxy->GetLightFunctionMaterial()->GetMaterial(GRHIFeatureLevel)->IsLightFunction());

		const FMaterialRenderProxy* MaterialProxy = bApplyLightFunction ? 
			LightSceneInfo.Proxy->GetLightFunctionMaterial() : 
			UMaterial::GetDefaultMaterial(MD_LightFunction)->GetRenderProxy(false);

		// Skip rendering if the DefaultLightFunctionMaterial isn't compiled yet
		if (MaterialProxy->GetMaterial(GRHIFeatureLevel)->IsLightFunction())
		{
			FTranslucentLightInjectionData InjectionData;
			InjectionData.LightSceneInfo = &LightSceneInfo;
			InjectionData.ProjectedShadowInfo = InProjectedShadowInfo;
			InjectionData.bApplyLightFunction = bApplyLightFunction;
			InjectionData.LightFunctionMaterialProxy = MaterialProxy;
			LightInjectionData.Add(InjectionData);
		}
	}
}

/** Injects all the lights in LightInjectionData into the translucent lighting volume textures. */
static void InjectTranslucentLightArray(const FViewInfo& View, const TArray<FTranslucentLightInjectionData, SceneRenderingAllocator>& LightInjectionData)
{
	INC_DWORD_STAT_BY(STAT_NumLightsInjectedIntoTranslucency, LightInjectionData.Num());

	// Inject into each volume cascade
	// Operate on one cascade at a time to reduce render target switches
	for (int32 VolumeCascadeIndex = 0; VolumeCascadeIndex < TVC_MAX; VolumeCascadeIndex++)
	{
		const IPooledRenderTarget* RT0 = GSceneRenderTargets.TranslucencyLightingVolumeAmbient[VolumeCascadeIndex];
		const IPooledRenderTarget* RT1 = GSceneRenderTargets.TranslucencyLightingVolumeDirectional[VolumeCascadeIndex];

		GRenderTargetPool.VisualizeTexture.SetCheckPoint(RT0);
		GRenderTargetPool.VisualizeTexture.SetCheckPoint(RT1);

		FTextureRHIParamRef RenderTargets[2];
		RenderTargets[0] = RT0->GetRenderTargetItem().TargetableTexture;
		RenderTargets[1] = RT1->GetRenderTargetItem().TargetableTexture;

		RHISetRenderTargets(ARRAY_COUNT(RenderTargets), RenderTargets, FTextureRHIRef(), 0, NULL);

		RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
		RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());

		for (int32 LightIndex = 0; LightIndex < LightInjectionData.Num(); LightIndex++)
		{
			const FTranslucentLightInjectionData& InjectionData = LightInjectionData[LightIndex];
			const FLightSceneInfo* const LightSceneInfo = InjectionData.LightSceneInfo;
			const bool bInverseSquared = LightSceneInfo->Proxy->IsInverseSquared();
			const bool bDirectionalLight = LightSceneInfo->Proxy->GetLightType() == LightType_Directional;
			const FVolumeBounds VolumeBounds = CalculateLightVolumeBounds(LightSceneInfo->Proxy->GetBoundingSphere(), View, VolumeCascadeIndex, bDirectionalLight);

			if (VolumeBounds.IsValid())
			{
				TShaderMapRef<FWriteToSliceVS> VertexShader(GetGlobalShaderMap());
				TShaderMapRef<FWriteToSliceGS> GeometryShader(GetGlobalShaderMap());

				if (bDirectionalLight)
				{
					// Accumulate the contribution of multiple lights
					// Directional lights write their shadowing into alpha of the ambient texture
					RHISetBlendState(TStaticBlendState<
						CW_RGBA, BO_Add, BF_One, BF_One, BO_Add, BF_One, BF_One,
						CW_RGB, BO_Add, BF_One, BF_One, BO_Add, BF_One, BF_One>::GetRHI());
					
					if (InjectionData.ProjectedShadowInfo)
					{
						// shadows, restricting light contribution to the cascade bounds (except last cascade far to get light functions and no shadows there)
						SetInjectionShader<LightType_Directional,true>(View, InjectionData.LightFunctionMaterialProxy, LightSceneInfo, InjectionData.ProjectedShadowInfo, InjectionData.ProjectedShadowInfo->SplitIndex, VolumeCascadeIndex, *VertexShader, *GeometryShader, InjectionData.bApplyLightFunction, false);
					}
					else
					{
						// no shadows
						SetInjectionShader<LightType_Directional,false>(View, InjectionData.LightFunctionMaterialProxy, LightSceneInfo, InjectionData.ProjectedShadowInfo, -1, VolumeCascadeIndex, *VertexShader, *GeometryShader, InjectionData.bApplyLightFunction, false);
					}
				}
				else
				{
					// Accumulate the contribution of multiple lights
					RHISetBlendState(TStaticBlendState<
						CW_RGB, BO_Add, BF_One, BF_One, BO_Add, BF_Zero, BF_One,
						CW_RGB, BO_Add, BF_One, BF_One, BO_Add, BF_Zero, BF_One>::GetRHI());

					if (InjectionData.ProjectedShadowInfo)
					{
						SetInjectionShader<LightType_Point,true>(View, InjectionData.LightFunctionMaterialProxy, LightSceneInfo, InjectionData.ProjectedShadowInfo, -1, VolumeCascadeIndex, *VertexShader, *GeometryShader, InjectionData.bApplyLightFunction, bInverseSquared);
					}
					else
					{
						SetInjectionShader<LightType_Point,false>(View, InjectionData.LightFunctionMaterialProxy, LightSceneInfo, InjectionData.ProjectedShadowInfo, -1, VolumeCascadeIndex, *VertexShader, *GeometryShader, InjectionData.bApplyLightFunction, bInverseSquared);
					}
				}

				VertexShader->SetParameters(VolumeBounds, GTranslucencyLightingVolumeDim);
				GeometryShader->SetParameters(VolumeBounds);
				RasterizeToVolumeTexture(VolumeBounds);
			}
		}

		RHICopyToResolveTarget(RT0->GetRenderTargetItem().TargetableTexture, RT0->GetRenderTargetItem().ShaderResourceTexture, true, FResolveParams());
		RHICopyToResolveTarget(RT1->GetRenderTargetItem().TargetableTexture, RT1->GetRenderTargetItem().ShaderResourceTexture, true, FResolveParams());
	}
}

void FDeferredShadingSceneRenderer::InjectTranslucentVolumeLighting(const FLightSceneInfo& LightSceneInfo, const FProjectedShadowInfo* InProjectedShadowInfo)
{
	if (GUseTranslucentLightingVolumes)
	{
		SCOPE_CYCLE_COUNTER(STAT_TranslucentInjectTime);

		//@todo - support multiple views
		const FViewInfo& View = Views[0];

		TArray<FTranslucentLightInjectionData, SceneRenderingAllocator> LightInjectionData;

		AddLightForInjection(*this, LightSceneInfo, InProjectedShadowInfo, LightInjectionData);

		// shadowed or unshadowed (InProjectedShadowInfo==0)
		InjectTranslucentLightArray(View, LightInjectionData);
	}
}

void FDeferredShadingSceneRenderer::InjectTranslucentVolumeLightingArray(const TArray<FSortedLightSceneInfo, SceneRenderingAllocator>& SortedLights, int32 NumLights)
{
	SCOPE_CYCLE_COUNTER(STAT_TranslucentInjectTime);

	//@todo - support multiple views
	const FViewInfo& View = Views[0];

	TArray<FTranslucentLightInjectionData, SceneRenderingAllocator> LightInjectionData;
	LightInjectionData.Empty(NumLights);

	for (int32 LightIndex = 0; LightIndex < NumLights; LightIndex++)
	{
		const FSortedLightSceneInfo& SortedLightInfo = SortedLights[LightIndex];
		const FLightSceneInfoCompact& LightSceneInfoCompact = SortedLightInfo.SceneInfo;
		const FLightSceneInfo* const LightSceneInfo = LightSceneInfoCompact.LightSceneInfo;

		AddLightForInjection(*this, *LightSceneInfo, NULL, LightInjectionData);
	}

	// non-shadowed, non-light function lights
	InjectTranslucentLightArray(View, LightInjectionData);
}

/** Pixel shader used to inject simple lights into the translucent lighting volume */
class FSimpleLightTranslucentLightingInjectPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FSimpleLightTranslucentLightingInjectPS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform) 
	{ 
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4); 
	}

	FSimpleLightTranslucentLightingInjectPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
		VolumeCascadeIndex.Bind(Initializer.ParameterMap, TEXT("VolumeCascadeIndex"));
		SimpleLightPositionAndRadius.Bind(Initializer.ParameterMap, TEXT("SimpleLightPositionAndRadius"));
		SimpleLightColorAndExponent.Bind(Initializer.ParameterMap, TEXT("SimpleLightColorAndExponent"));
	}
	FSimpleLightTranslucentLightingInjectPS() {}

	void SetParameters(const FViewInfo& View, const FSimpleLightEntry& SimpleLight, int32 VolumeCascadeIndexValue)
	{
		FGlobalShader::SetParameters(GetPixelShader(), View);

		SetShaderValue(GetPixelShader(), VolumeCascadeIndex, VolumeCascadeIndexValue);
		SetShaderValue(GetPixelShader(), SimpleLightPositionAndRadius, SimpleLight.PositionAndRadius);
		SetShaderValue(GetPixelShader(), SimpleLightColorAndExponent, FVector4(SimpleLight.Color, SimpleLight.Exponent));
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << VolumeCascadeIndex;
		Ar << SimpleLightPositionAndRadius;
		Ar << SimpleLightColorAndExponent;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderParameter VolumeCascadeIndex;
	FShaderParameter SimpleLightPositionAndRadius;
	FShaderParameter SimpleLightColorAndExponent;
};

IMPLEMENT_SHADER_TYPE(,FSimpleLightTranslucentLightingInjectPS,TEXT("TranslucentLightInjectionShaders"),TEXT("SimpleLightInjectMainPS"),SF_Pixel);

FGlobalBoundShaderState InjectSimpleLightBoundShaderState;

void FDeferredShadingSceneRenderer::InjectSimpleTranslucentVolumeLightingArray(const TArray<FSimpleLightEntry, SceneRenderingAllocator>& SimpleLights)
{
	SCOPE_CYCLE_COUNTER(STAT_TranslucentInjectTime);

	int32 NumLightsToInject = 0;

	for (int32 LightIndex = 0; LightIndex < SimpleLights.Num(); LightIndex++)
	{
		if (SimpleLights[LightIndex].bAffectTranslucency)
		{
			NumLightsToInject++;
		}
	}

	if (NumLightsToInject > 0)
	{
		//@todo - support multiple views
		const FViewInfo& View = Views[0];

		INC_DWORD_STAT_BY(STAT_NumLightsInjectedIntoTranslucency, NumLightsToInject);

		// Inject into each volume cascade
		// Operate on one cascade at a time to reduce render target switches
		for (int32 VolumeCascadeIndex = 0; VolumeCascadeIndex < TVC_MAX; VolumeCascadeIndex++)
		{
			const IPooledRenderTarget* RT0 = GSceneRenderTargets.TranslucencyLightingVolumeAmbient[VolumeCascadeIndex];
			const IPooledRenderTarget* RT1 = GSceneRenderTargets.TranslucencyLightingVolumeDirectional[VolumeCascadeIndex];

			GRenderTargetPool.VisualizeTexture.SetCheckPoint(RT0);
			GRenderTargetPool.VisualizeTexture.SetCheckPoint(RT1);

			FTextureRHIParamRef RenderTargets[2];
			RenderTargets[0] = RT0->GetRenderTargetItem().TargetableTexture;
			RenderTargets[1] = RT1->GetRenderTargetItem().TargetableTexture;

			RHISetRenderTargets(ARRAY_COUNT(RenderTargets), RenderTargets, FTextureRHIRef(), 0, NULL);

			RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
			RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());

			for (int32 LightIndex = 0; LightIndex < SimpleLights.Num(); LightIndex++)
			{
				const FSimpleLightEntry& SimpleLight = SimpleLights[LightIndex];

				if (SimpleLight.bAffectTranslucency)
				{
					const FVolumeBounds VolumeBounds = CalculateLightVolumeBounds((const FSphere&)SimpleLight.PositionAndRadius, View, VolumeCascadeIndex, false);

					if (VolumeBounds.IsValid())
					{
						TShaderMapRef<FWriteToSliceVS> VertexShader(GetGlobalShaderMap());
						TShaderMapRef<FWriteToSliceGS> GeometryShader(GetGlobalShaderMap());
						TShaderMapRef<FSimpleLightTranslucentLightingInjectPS> PixelShader(GetGlobalShaderMap());
						SetGlobalBoundShaderState(InjectSimpleLightBoundShaderState, GScreenVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader, *GeometryShader);

						VertexShader->SetParameters(VolumeBounds, GTranslucencyLightingVolumeDim);
						GeometryShader->SetParameters(VolumeBounds);
						PixelShader->SetParameters(View, SimpleLight, VolumeCascadeIndex);

						// Accumulate the contribution of multiple lights
						RHISetBlendState(TStaticBlendState<
							CW_RGB, BO_Add, BF_One, BF_One, BO_Add, BF_Zero, BF_One,
							CW_RGB, BO_Add, BF_One, BF_One, BO_Add, BF_Zero, BF_One>::GetRHI());

						RasterizeToVolumeTexture(VolumeBounds);
					}
				}
			}

			RHICopyToResolveTarget(RT0->GetRenderTargetItem().TargetableTexture, RT0->GetRenderTargetItem().ShaderResourceTexture, true, FResolveParams());
			RHICopyToResolveTarget(RT1->GetRenderTargetItem().TargetableTexture, RT1->GetRenderTargetItem().ShaderResourceTexture, true, FResolveParams());
		}
	}
}

FGlobalBoundShaderState FilterBoundShaderState;

void FDeferredShadingSceneRenderer::FilterTranslucentVolumeLighting()
{
	if (GUseTranslucentLightingVolumes)
	{
		if (GUseTranslucencyVolumeBlur)
		{
			SCOPED_DRAW_EVENT(FilterTranslucentVolume, DEC_SCENE_ITEMS);

			RHISetRasterizerState(TStaticRasterizerState<FM_Solid,CM_None>::GetRHI());
			RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());
			RHISetBlendState(TStaticBlendState<>::GetRHI());

			// Filter each cascade
			for (int32 VolumeCascadeIndex = 0; VolumeCascadeIndex < TVC_MAX; VolumeCascadeIndex++)
			{
				const IPooledRenderTarget* RT0 = GSceneRenderTargets.GetTranslucencyVolumeAmbient((ETranslucencyVolumeCascade)VolumeCascadeIndex);
				const IPooledRenderTarget* RT1 = GSceneRenderTargets.GetTranslucencyVolumeDirectional((ETranslucencyVolumeCascade)VolumeCascadeIndex);

				GRenderTargetPool.VisualizeTexture.SetCheckPoint(RT0);
				GRenderTargetPool.VisualizeTexture.SetCheckPoint(RT1);

				FTextureRHIParamRef RenderTargets[2];
				RenderTargets[0] = RT0->GetRenderTargetItem().TargetableTexture;
				RenderTargets[1] = RT1->GetRenderTargetItem().TargetableTexture;

				RHISetRenderTargets(ARRAY_COUNT(RenderTargets), RenderTargets, FTextureRHIRef(), 0, NULL);

				//@todo - support multiple views
				const FViewInfo& View = Views[0];

				const FVolumeBounds VolumeBounds(GTranslucencyLightingVolumeDim);
				TShaderMapRef<FWriteToSliceVS> VertexShader(GetGlobalShaderMap());
				TShaderMapRef<FWriteToSliceGS> GeometryShader(GetGlobalShaderMap());
				TShaderMapRef<FFilterTranslucentVolumePS> PixelShader(GetGlobalShaderMap());
				SetGlobalBoundShaderState(FilterBoundShaderState, GScreenVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader, *GeometryShader);

				VertexShader->SetParameters(VolumeBounds, GTranslucencyLightingVolumeDim);
				GeometryShader->SetParameters(VolumeBounds);
				PixelShader->SetParameters(View, VolumeCascadeIndex);

				RasterizeToVolumeTexture(VolumeBounds);

				RHICopyToResolveTarget(RT0->GetRenderTargetItem().TargetableTexture, RT0->GetRenderTargetItem().ShaderResourceTexture, true, FResolveParams());
				RHICopyToResolveTarget(RT1->GetRenderTargetItem().TargetableTexture, RT1->GetRenderTargetItem().ShaderResourceTexture, true, FResolveParams());
			}
		}
	}
}
