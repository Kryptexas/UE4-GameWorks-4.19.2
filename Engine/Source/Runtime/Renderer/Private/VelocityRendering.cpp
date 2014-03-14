// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VelocityRendering.cpp: Velocity rendering implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "../../Engine/Private/SkeletalRenderGPUSkin.h"		// GPrevPerBoneMotionBlur

//=============================================================================
/** Encapsulates the Velocity vertex shader. */
class FVelocityVS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FVelocityVS,MeshMaterial);

public:

	void SetParameters(const FVertexFactory* VertexFactory,const FMaterialRenderProxy* MaterialRenderProxy,const FViewInfo& View)
	{
		FMeshMaterialShader::SetParameters(GetVertexShader(),MaterialRenderProxy,*MaterialRenderProxy->GetMaterial(GRHIFeatureLevel),View,ESceneRenderTargetsMode::DontSet);
	}

	void SetMesh(const FVertexFactory* VertexFactory, const FMeshBatch& Mesh, int32 BatchElementIndex, const FViewInfo& View, const FPrimitiveSceneProxy* Proxy, const FMatrix& InPreviousLocalToWorld)
	{
		FMeshMaterialShader::SetMesh(GetVertexShader(), VertexFactory, View, Proxy, Mesh.Elements[BatchElementIndex]);

		SetShaderValue(GetVertexShader(), PreviousLocalToWorld, InPreviousLocalToWorld.ConcatTranslation(View.PrevViewMatrices.PreViewTranslation));
	}

	bool SupportsVelocity() const
	{
		return PreviousLocalToWorld.IsBound();
	}

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		//Only compile the velocity shaders for the default material or if it's masked,
		return ((Material->IsSpecialEngineMaterial() || Material->IsMasked() 
			//or if the material is opaque and two-sided,
			|| (Material->IsTwoSided() && !IsTranslucentBlendMode(Material->GetBlendMode()))
			// or if the material modifies meshes
			|| Material->MaterialModifiesMeshPosition()))
			&& IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

protected:
	FVelocityVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FMeshMaterialShader(Initializer)
	{
		PreviousLocalToWorld.Bind(Initializer.ParameterMap,TEXT("PreviousLocalToWorld"));
	}
	FVelocityVS() {}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FMeshMaterialShader::Serialize(Ar);
		Ar << PreviousLocalToWorld;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderParameter PreviousLocalToWorld;
};


//=============================================================================
/** Encapsulates the Velocity hull shader. */
class FVelocityHS : public FBaseHS
{
	DECLARE_SHADER_TYPE(FVelocityHS,MeshMaterial);

protected:
	FVelocityHS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FBaseHS(Initializer)
	{}

	FVelocityHS() {}

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return FBaseHS::ShouldCache(Platform, Material, VertexFactoryType) &&
				FVelocityVS::ShouldCache(Platform, Material, VertexFactoryType); // same rules as VS
	}
};

//=============================================================================
/** Encapsulates the Velocity domain shader. */
class FVelocityDS : public FBaseDS
{
	DECLARE_SHADER_TYPE(FVelocityDS,MeshMaterial);

public:

	void SetParameters(const FMaterialRenderProxy* MaterialRenderProxy,const FViewInfo& View)
	{
		FMeshMaterialShader::SetParameters(GetDomainShader(), MaterialRenderProxy, *MaterialRenderProxy->GetMaterial(GRHIFeatureLevel), View, ESceneRenderTargetsMode::DontSet);
	}

protected:
	FVelocityDS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FBaseDS(Initializer)
	{}

	FVelocityDS() {}

	static bool ShouldCache(EShaderPlatform Platform, const FMaterial* Material, const FVertexFactoryType* VertexFactoryType)
	{
		return FBaseDS::ShouldCache(Platform, Material, VertexFactoryType) &&
				FVelocityVS::ShouldCache(Platform, Material, VertexFactoryType); // same rules as VS
	}
};

IMPLEMENT_MATERIAL_SHADER_TYPE(,FVelocityVS,TEXT("VelocityShader"),TEXT("MainVertexShader"),SF_Vertex); 
IMPLEMENT_MATERIAL_SHADER_TYPE(,FVelocityHS,TEXT("VelocityShader"),TEXT("MainHull"),SF_Hull); 
IMPLEMENT_MATERIAL_SHADER_TYPE(,FVelocityDS,TEXT("VelocityShader"),TEXT("MainDomain"),SF_Domain);

//=============================================================================
/** Encapsulates the Velocity pixel shader. */
class FVelocityPS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FVelocityPS,MeshMaterial);
public:
	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		//Only compile the velocity shaders for the default material or if it's masked,
		return ((Material->IsSpecialEngineMaterial() || Material->IsMasked() 
			//or if the material is opaque and two-sided,
			|| (Material->IsTwoSided() && !IsTranslucentBlendMode(Material->GetBlendMode()))
			// or if the material modifies meshes
			|| Material->MaterialModifiesMeshPosition()))
			&& IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM4);
	}

	FVelocityPS() {}

	FVelocityPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMeshMaterialShader(Initializer)
	{
	}

	void SetParameters(const FVertexFactory* VertexFactory,const FMaterialRenderProxy* MaterialRenderProxy,const FViewInfo& View)
	{
		FMeshMaterialShader::SetParameters(GetPixelShader(),MaterialRenderProxy,*MaterialRenderProxy->GetMaterial(GRHIFeatureLevel),View,ESceneRenderTargetsMode::DontSet);
	}

	void SetMesh(const FVertexFactory* VertexFactory, const FMeshBatch& Mesh,int32 BatchElementIndex,const FViewInfo& View, const FPrimitiveSceneProxy* Proxy, bool bBackFace)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();
		
		FMeshMaterialShader::SetMesh(ShaderRHI, VertexFactory, View, Proxy, Mesh.Elements[BatchElementIndex]);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FMeshMaterialShader::Serialize(Ar);
		return bShaderHasOutdatedParameters;
	}
};

IMPLEMENT_MATERIAL_SHADER_TYPE(,FVelocityPS,TEXT("VelocityShader"),TEXT("MainPixelShader"),SF_Pixel);


//=============================================================================
/** FVelocityDrawingPolicy - Policy to wrap FMeshDrawingPolicy with new shaders. */

FVelocityDrawingPolicy::FVelocityDrawingPolicy(
	const FVertexFactory* InVertexFactory,
	const FMaterialRenderProxy* InMaterialRenderProxy,
	const FMaterial& InMaterialResource
	)
	:	FMeshDrawingPolicy(InVertexFactory,InMaterialRenderProxy,InMaterialResource)	
{
	const FMaterialShaderMap* MaterialShaderIndex = InMaterialResource.GetRenderingThreadShaderMap();
	const FMeshMaterialShaderMap* MeshShaderIndex = MaterialShaderIndex->GetMeshShaderMap(InVertexFactory->GetType());

	HullShader = NULL;
	DomainShader = NULL;

	const EMaterialTessellationMode MaterialTessellationMode = InMaterialResource.GetTessellationMode();
	if(RHISupportsTessellation(GRHIShaderPlatform)
		&& InVertexFactory->GetType()->SupportsTessellationShaders() 
		&& MaterialTessellationMode != MTM_NoTessellation)
	{
		bool HasHullShader = MeshShaderIndex->HasShader(&FVelocityHS::StaticType);
		bool HasDomainShader = MeshShaderIndex->HasShader(&FVelocityDS::StaticType);

		HullShader = HasHullShader ? MeshShaderIndex->GetShader<FVelocityHS>() : NULL;
		DomainShader = HasDomainShader ? MeshShaderIndex->GetShader<FVelocityDS>() : NULL;
	}

	bool HasVertexShader = MeshShaderIndex->HasShader(&FVelocityVS::StaticType);
	VertexShader = HasVertexShader ? MeshShaderIndex->GetShader<FVelocityVS>() : NULL;

	bool HasPixelShader = MeshShaderIndex->HasShader(&FVelocityPS::StaticType);
	PixelShader = HasPixelShader ? MeshShaderIndex->GetShader<FVelocityPS>() : NULL;
}

bool FVelocityDrawingPolicy::SupportsVelocity() const
{
	return (VertexShader && PixelShader) ? VertexShader->SupportsVelocity() : false;
}

void FVelocityDrawingPolicy::DrawShared(const FSceneView* SceneView, FBoundShaderStateRHIRef ShaderState) const
{
	// NOTE: Assuming this cast is always safe!
	FViewInfo* View = (FViewInfo*)SceneView;

	// Set the depth-only shader parameters for the material.
	RHISetBoundShaderState(ShaderState);
	VertexShader->SetParameters(VertexFactory, MaterialRenderProxy, *View);
	PixelShader->SetParameters(VertexFactory, MaterialRenderProxy, *View);

	if(HullShader && DomainShader)
	{
		HullShader->SetParameters(MaterialRenderProxy, *View);
		DomainShader->SetParameters(MaterialRenderProxy, *View);
	}

	// Set the shared mesh resources.
	FMeshDrawingPolicy::DrawShared( View );
}

void FVelocityDrawingPolicy::SetMeshRenderState(
	const FSceneView& SceneView,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	const FMeshBatch& Mesh,
	int32 BatchElementIndex,
	bool bBackFace,
	const ElementDataType& ElementData
	) const
{
	const FMeshBatchElement& BatchElement = Mesh.Elements[BatchElementIndex];
	FMatrix PreviousLocalToWorld;
	
	FViewInfo& View = (FViewInfo&)SceneView;	// NOTE: Assuming this cast is always safe!

	// hack
	FScene* Scene = (FScene*)PrimitiveSceneProxy->GetScene();

	// previous transform can be stored in the scene for each primitive
	if(Scene->MotionBlurInfoData.GetPrimitiveMotionBlurInfo(PrimitiveSceneProxy->GetPrimitiveSceneInfo(), PreviousLocalToWorld))
	{
		VertexShader->SetMesh(VertexFactory, Mesh, BatchElementIndex, View, PrimitiveSceneProxy, PreviousLocalToWorld);
	}	
	else
	{
		const FMatrix& LocalToWorld = PrimitiveSceneProxy->GetLocalToWorld();		// different implementation from UE4
		VertexShader->SetMesh(VertexFactory, Mesh, BatchElementIndex, View, PrimitiveSceneProxy, LocalToWorld);
	}

	if (HullShader && DomainShader)
	{
		DomainShader->SetMesh(VertexFactory, View, PrimitiveSceneProxy, Mesh.Elements[BatchElementIndex]);
		HullShader->SetMesh(VertexFactory, View, PrimitiveSceneProxy, Mesh.Elements[BatchElementIndex]);
	}

	PixelShader->SetMesh(VertexFactory, Mesh, BatchElementIndex, View, PrimitiveSceneProxy, bBackFace);
	FMeshDrawingPolicy::SetMeshRenderState(View, PrimitiveSceneProxy, Mesh, BatchElementIndex, bBackFace, ElementData);
}

bool FVelocityDrawingPolicy::HasVelocity(const FViewInfo& View, const FPrimitiveSceneInfo* PrimitiveSceneInfo)
{
	checkSlow(IsInRenderingThread());

	// No velocity if motionblur is off, or if it's a non-moving object (treat as background in that case)
	if(View.bCameraCut || !PrimitiveSceneInfo->Proxy->IsMovable())
	{
		return false;
	}

	if(PrimitiveSceneInfo->Proxy->AlwaysHasVelocity())
	{
		return true;
	}

	if(PrimitiveSceneInfo->bVelocityIsSupressed)
	{
		return false;
	}

	// check if the primitive has moved
	{
		FMatrix PreviousLocalToWorld;

		// hack
		FScene* Scene = PrimitiveSceneInfo->Scene;

		if(Scene->MotionBlurInfoData.GetPrimitiveMotionBlurInfo(PrimitiveSceneInfo, PreviousLocalToWorld))
		{
			check(PrimitiveSceneInfo->Proxy);

			const FMatrix& LocalToWorld = PrimitiveSceneInfo->Proxy->GetLocalToWorld();

			// Hasn't moved (treat as background by not rendering any special velocities)?
			if(LocalToWorld.Equals(PreviousLocalToWorld, 0.0001f))
			{
				return false;
			}
		}
		else 
		{
			return false;
		}
	}

	return true;
}

FBoundShaderStateRHIRef FVelocityDrawingPolicy::CreateBoundShaderState()
{
	FBoundShaderStateRHIRef BoundShaderState;

	BoundShaderState = RHICreateBoundShaderState(
		FMeshDrawingPolicy::GetVertexDeclaration(), 
		VertexShader->GetVertexShader(),
		GETSAFERHISHADER_HULL(HullShader), 
		GETSAFERHISHADER_DOMAIN(DomainShader),
		PixelShader->GetPixelShader(),
		FGeometryShaderRHIRef());

	return BoundShaderState;
}

int32 Compare(const FVelocityDrawingPolicy& A,const FVelocityDrawingPolicy& B)
{
	COMPAREDRAWINGPOLICYMEMBERS(VertexShader);
	COMPAREDRAWINGPOLICYMEMBERS(PixelShader);
	COMPAREDRAWINGPOLICYMEMBERS(HullShader);
	COMPAREDRAWINGPOLICYMEMBERS(DomainShader);
	COMPAREDRAWINGPOLICYMEMBERS(VertexFactory);
	COMPAREDRAWINGPOLICYMEMBERS(MaterialRenderProxy);
	return 0;
}


//=============================================================================
/** Policy to wrap FMeshDrawingPolicy with new shaders. */

void FVelocityDrawingPolicyFactory::AddStaticMesh(FScene* Scene, FStaticMesh* StaticMesh, ContextType)
{
	// Velocity only needs to be directly rendered for movable meshes.
	if(StaticMesh->PrimitiveSceneInfo->Proxy->IsMovable())
	{
	    const FMaterialRenderProxy* MaterialRenderProxy = StaticMesh->MaterialRenderProxy;
	    const FMaterial* Material = MaterialRenderProxy->GetMaterial(GRHIFeatureLevel);
	    EBlendMode BlendMode = Material->GetBlendMode();
	    if(BlendMode == BLEND_Opaque || BlendMode == BLEND_Masked)
	    {
		    if(!Material->IsMasked() && !Material->IsTwoSided() && !Material->MaterialModifiesMeshPosition())
		    {
			    // Default material doesn't handle masked or mesh-mod, and doesn't have the correct bIsTwoSided setting.
			    MaterialRenderProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy(false);
		    }

			FVelocityDrawingPolicy DrawingPolicy(StaticMesh->VertexFactory, MaterialRenderProxy,*MaterialRenderProxy->GetMaterial(GRHIFeatureLevel));
			if (DrawingPolicy.SupportsVelocity())
			{
				// Add the static mesh to the depth-only draw list.
				Scene->VelocityDrawList.AddMesh(StaticMesh, FVelocityDrawingPolicy::ElementDataType(), DrawingPolicy);
			}
	    }
	}
}

bool FVelocityDrawingPolicyFactory::DrawDynamicMesh(
	const FViewInfo& View,
	ContextType DrawingContext,
	const FMeshBatch& Mesh,
	bool bBackFace,
	bool bPreFog,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	FHitProxyId HitProxyId
	)
{
	// Only draw opaque materials in the depth pass.
	const FMaterialRenderProxy* MaterialRenderProxy = Mesh.MaterialRenderProxy;
	const FMaterial* Material = MaterialRenderProxy->GetMaterial(GRHIFeatureLevel);
	EBlendMode BlendMode = Material->GetBlendMode();

	if(BlendMode == BLEND_Opaque || BlendMode == BLEND_Masked)
	{
		// This should be enforced at a higher level
		//@todo - figure out why this is failing and re-enable
		//check(FVelocityDrawingPolicy::HasVelocity(View, PrimitiveSceneInfo));
		if(!Material->IsMasked() && !Material->IsTwoSided() && !Material->MaterialModifiesMeshPosition())
		{
			// Default material doesn't handle masked, and doesn't have the correct bIsTwoSided setting.
			MaterialRenderProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy(false);
		}
		FVelocityDrawingPolicy DrawingPolicy(Mesh.VertexFactory, MaterialRenderProxy, *MaterialRenderProxy->GetMaterial(GRHIFeatureLevel));
		if(DrawingPolicy.SupportsVelocity())
		{			
			DrawingPolicy.DrawShared(&View,DrawingPolicy.CreateBoundShaderState());
			for(int32 BatchElementIndex = 0; BatchElementIndex < Mesh.Elements.Num(); ++BatchElementIndex)
			{
				DrawingPolicy.SetMeshRenderState(View, PrimitiveSceneProxy, Mesh, BatchElementIndex, bBackFace, FMeshDrawingPolicy::ElementDataType());
				DrawingPolicy.DrawMesh(Mesh, BatchElementIndex);
			}
			return true;
		}
		return false;
	}
	else
	{
		return false;
	}
}


int32 GetMotionBlurQualityFromCVar()
{
	int32 MotionBlurQuality;

	static const auto ICVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.MotionBlurQuality"));
	MotionBlurQuality = FMath::Clamp(ICVar->GetValueOnRenderThread(), 0, 4);

	return MotionBlurQuality;
}

bool IsMotionBlurEnabled(const FViewInfo& View)
{
	if(!(GRHIFeatureLevel >= ERHIFeatureLevel::SM4))
	{
		return false; 
	}

	int32 MotionBlurQuality = GetMotionBlurQualityFromCVar();

	return View.Family->EngineShowFlags.PostProcessing
		&& View.Family->EngineShowFlags.MotionBlur
		&& View.FinalPostProcessSettings.MotionBlurAmount > 0.001f
		&& View.FinalPostProcessSettings.MotionBlurMax > 0.001f
		&& View.Family->bRealtimeUpdate
		&& MotionBlurQuality > 0
		&& !View.bIsSceneCapture
		&& !(View.Family->Views.Num() > 1);
}

void FDeferredShadingSceneRenderer::RenderVelocities(const FViewInfo& View, TRefCountPtr<IPooledRenderTarget>& VelocityRT, bool bLastFrame)
{
	SCOPE_CYCLE_COUNTER( STAT_RenderVelocities );

	bool bTemporalAA = (View.FinalPostProcessSettings.AntiAliasingMethod == AAM_TemporalAA) && !View.bCameraCut;

	if(!IsMotionBlurEnabled(View) && !bTemporalAA)
	{
		return;
	}

	SCOPED_DRAW_EVENT(RenderVelocities, DEC_SCENE_ITEMS);

	const FIntPoint BufferSize = GSceneRenderTargets.GetBufferSizeXY();
	const FIntPoint VelocityBufferSize = BufferSize;		// full resolution so we can reuse the existing full res z buffer

	if(!VelocityRT)
	{
		FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(VelocityBufferSize, PF_G16R16, TexCreate_None, TexCreate_RenderTargetable, false));

		GRenderTargetPool.FindFreeElement(Desc, VelocityRT, TEXT("Velocity"));
	}

	GPrevPerBoneMotionBlur.LockData();

	const uint32 MinX = View.ViewRect.Min.X * VelocityBufferSize.X / BufferSize.X;
	const uint32 MinY = View.ViewRect.Min.Y * VelocityBufferSize.Y / BufferSize.Y;
	const uint32 MaxX = View.ViewRect.Max.X * VelocityBufferSize.X / BufferSize.X;
	const uint32 MaxY = View.ViewRect.Max.Y * VelocityBufferSize.Y / BufferSize.Y;
	RHISetRenderTarget(VelocityRT->GetRenderTargetItem().TargetableTexture, GSceneRenderTargets.GetSceneDepthTexture());

	RHISetViewport(MinX, MinY, 0.0f, MaxX, MaxY, 1.0f);

	// Clear the velocity buffer (0.0f means "use static background velocity").
	RHIClear(true, FLinearColor::Black, false, 1.0f, false, 0, FIntRect());

	// Blending is not supported with the velocity buffer format on other platforms
	// opaque velocities in R|G channels, B is used to identify object motion
	RHISetBlendState(TStaticBlendState<CW_RGBA>::GetRHI());
	// Use depth tests, no z-writes, backface-culling. 

	// Note, this is a reversed Z depth surface, using CF_GreaterEqual.
	RHISetDepthStencilState(TStaticDepthStencilState<false,CF_GreaterEqual>::GetRHI());
			
	RHISetRasterizerState(GetStaticRasterizerState<true>(FM_Solid, CM_CW));

	// Draw velocities for movable static meshes.
	Scene->VelocityDrawList.DrawVisible(View,View.StaticMeshVelocityMap, View.StaticMeshBatchVisibility);

	// Draw velocities for movable dynamic meshes.
	TDynamicPrimitiveDrawer<FVelocityDrawingPolicyFactory> Drawer(
		&View,FVelocityDrawingPolicyFactory::ContextType(DDM_AllOccluders),true,false,true
		);
	for(int32 PrimitiveIndex = 0;PrimitiveIndex < View.VisibleDynamicPrimitives.Num();PrimitiveIndex++)
	{
		const FPrimitiveSceneInfo* PrimitiveSceneInfo = View.VisibleDynamicPrimitives[PrimitiveIndex];

		if(!PrimitiveSceneInfo->ShouldRenderVelocity(View))
		{
			continue;
		}

		FScopeCycleCounter Context(PrimitiveSceneInfo->Proxy->GetStatId());

		Drawer.SetPrimitive(PrimitiveSceneInfo->Proxy);
		PrimitiveSceneInfo->Proxy->DrawDynamicElements(&Drawer, &View);
	}				
	Drawer.IsDirty();

	RHICopyToResolveTarget(VelocityRT->GetRenderTargetItem().TargetableTexture, VelocityRT->GetRenderTargetItem().ShaderResourceTexture, false, FResolveParams());

	// restore any color write state changes
	RHISetBlendState(TStaticBlendState<>::GetRHI());
	RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
	GPrevPerBoneMotionBlur.UnlockData(bLastFrame);

	// to be able to observe results with VisualizeTexture
	GRenderTargetPool.VisualizeTexture.SetCheckPoint(VelocityRT);
}
