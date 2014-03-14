// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DistortionRendering.cpp: Distortion rendering implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "EnginePrivate.h"
#include "ScenePrivate.h"
#include "ScreenRendering.h"
#include "PostProcessing.h"
#include "RenderingCompositionGraph.h"
#include "SceneFilterRendering.h"

/**
* A pixel shader for rendering the full screen refraction pass
*/
class FDistortionApplyScreenPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FDistortionApplyScreenPS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform) { return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3); }

	FDistortionApplyScreenPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		DistortionTexture.Bind(Initializer.ParameterMap,TEXT("DistortionTexture"));
		DistortionTextureSampler.Bind(Initializer.ParameterMap,TEXT("DistortionTextureSampler"));
		SceneColorTexture.Bind(Initializer.ParameterMap,TEXT("SceneColorTexture"));
		SceneColorTextureSampler.Bind(Initializer.ParameterMap,TEXT("SceneColorTextureSampler"));
		SceneColorRect.Bind(Initializer.ParameterMap,TEXT("SceneColorRect"));
	}
	FDistortionApplyScreenPS() {}

	void SetParameters(const FRenderingCompositePassContext& Context)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FTextureRHIParamRef DistortionTextureValue = GSceneRenderTargets.LightAttenuation->GetRenderTargetItem().ShaderResourceTexture;
		FTextureRHIParamRef SceneColorTextureValue = GSceneRenderTargets.SceneColor->GetRenderTargetItem().ShaderResourceTexture;

		// Here we use SF_Point as in fullscreen the pixels are 1:1 mapped.
		SetTextureParameter(
			ShaderRHI,
			DistortionTexture,
			DistortionTextureSampler,
			TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			DistortionTextureValue
			);

		SetTextureParameter(
			ShaderRHI,
			SceneColorTexture,
			SceneColorTextureSampler,
			TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
			SceneColorTextureValue
			);

		FIntPoint SceneBufferSize = GSceneRenderTargets.GetBufferSizeXY();
		FIntRect ViewportRect = Context.GetViewport();
		FVector4 SceneColorRectValue = FVector4((float)ViewportRect.Min.X/SceneBufferSize.X,
												(float)ViewportRect.Min.Y/SceneBufferSize.Y,
												(float)ViewportRect.Max.X/SceneBufferSize.X,
												(float)ViewportRect.Max.Y/SceneBufferSize.Y);
		SetShaderValue(ShaderRHI, SceneColorRect, SceneColorRectValue);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << DistortionTexture << DistortionTextureSampler << SceneColorTexture << SceneColorTextureSampler << SceneColorRect;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderResourceParameter DistortionTexture;
	FShaderResourceParameter DistortionTextureSampler;
	FShaderResourceParameter SceneColorTexture;
	FShaderResourceParameter SceneColorTextureSampler;
	FShaderParameter SceneColorRect;
	FShaderParameter DistortionParams;
};

/** distortion apply screen pixel shader implementation */
IMPLEMENT_SHADER_TYPE(,FDistortionApplyScreenPS,TEXT("DistortApplyScreenPS"),TEXT("Main"),SF_Pixel);

/**
* Policy for drawing distortion mesh accumulated offsets
*/
class FDistortMeshAccumulatePolicy
{	
public:
	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return Material && IsTranslucentBlendMode(Material->GetBlendMode()) && Material->IsDistorted();
	}
};

/**
* A vertex shader for rendering distortion meshes
*/
template<class DistortMeshPolicy>
class TDistortionMeshVS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(TDistortionMeshVS,MeshMaterial);

protected:

	TDistortionMeshVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:	FMeshMaterialShader(Initializer)
	{
	}

	TDistortionMeshVS()
	{
	}

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3) && DistortMeshPolicy::ShouldCache(Platform,Material,VertexFactoryType);
	}

public:
	
	void SetParameters(const FVertexFactory* VertexFactory,const FMaterialRenderProxy* MaterialRenderProxy,const FSceneView* View)
	{
		FMeshMaterialShader::SetParameters(GetVertexShader(),MaterialRenderProxy,*MaterialRenderProxy->GetMaterial(GRHIFeatureLevel),*View,ESceneRenderTargetsMode::SetTextures);
	}

	void SetMesh(const FVertexFactory* VertexFactory,const FSceneView& View,const FPrimitiveSceneProxy* Proxy,const FMeshBatchElement& BatchElement)
	{
		FMeshMaterialShader::SetMesh(GetVertexShader(),VertexFactory,View,Proxy,BatchElement);
	}
};


/**
 * A hull shader for rendering distortion meshes
 */
template<class DistortMeshPolicy>
class TDistortionMeshHS : public FBaseHS
{
	DECLARE_SHADER_TYPE(TDistortionMeshHS,MeshMaterial);

protected:

	TDistortionMeshHS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FBaseHS(Initializer)
	{}

	TDistortionMeshHS() {}

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return FBaseHS::ShouldCache(Platform, Material, VertexFactoryType)
			&& IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3)
			&& DistortMeshPolicy::ShouldCache(Platform, Material, VertexFactoryType);
	}
};

/**
 * A domain shader for rendering distortion meshes
 */
template<class DistortMeshPolicy>
class TDistortionMeshDS : public FBaseDS
{
	DECLARE_SHADER_TYPE(TDistortionMeshDS,MeshMaterial);

protected:

	TDistortionMeshDS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FBaseDS(Initializer)
	{}

	TDistortionMeshDS() {}

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return FBaseDS::ShouldCache(Platform, Material, VertexFactoryType)
			&& IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3)
			&& DistortMeshPolicy::ShouldCache(Platform, Material, VertexFactoryType);
	}
};


IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TDistortionMeshVS<FDistortMeshAccumulatePolicy>,TEXT("DistortAccumulateVS"),TEXT("Main"),SF_Vertex); 
IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TDistortionMeshHS<FDistortMeshAccumulatePolicy>,TEXT("DistortAccumulateVS"),TEXT("MainHull"),SF_Hull); 
IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TDistortionMeshDS<FDistortMeshAccumulatePolicy>,TEXT("DistortAccumulateVS"),TEXT("MainDomain"),SF_Domain);


/**
* A pixel shader to render distortion meshes
*/
template<class DistortMeshPolicy>
class TDistortionMeshPS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(TDistortionMeshPS,MeshMaterial);

public:
	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3) && DistortMeshPolicy::ShouldCache(Platform,Material,VertexFactoryType);
	}

	TDistortionMeshPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:	FMeshMaterialShader(Initializer)
	{
//later?		MaterialParameters.Bind(Initializer.ParameterMap);
		DistortionParams.Bind(Initializer.ParameterMap,TEXT("DistortionParams"));
	}

	TDistortionMeshPS()
	{
	}

	void SetParameters(
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FSceneView& View
		)
	{
		FMeshMaterialShader::SetParameters(GetPixelShader(),MaterialRenderProxy,*MaterialRenderProxy->GetMaterial(GRHIFeatureLevel),View,ESceneRenderTargetsMode::SetTextures);


		float Ratio = View.UnscaledViewRect.Width() / (float)View.UnscaledViewRect.Height();
		float Params[4];
		Params[0] = View.ViewMatrices.ProjMatrix.M[0][0];
		Params[1] = Ratio;
		Params[2] = (float)View.UnscaledViewRect.Width();
		Params[3] = (float)View.UnscaledViewRect.Height();

		SetShaderValue(GetPixelShader(), DistortionParams, Params);

	}

	void SetMesh(const FVertexFactory* VertexFactory,const FSceneView& View,const FPrimitiveSceneProxy* Proxy,const FMeshBatchElement& BatchElement)
	{
		FMeshMaterialShader::SetMesh(GetPixelShader(),VertexFactory,View,Proxy,BatchElement);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FMeshMaterialShader::Serialize(Ar);
		Ar << DistortionParams;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderParameter DistortionParams;
};

//** distortion accumulate pixel shader type implementation */
IMPLEMENT_MATERIAL_SHADER_TYPE(template<>,TDistortionMeshPS<FDistortMeshAccumulatePolicy>,TEXT("DistortAccumulatePS"),TEXT("Main"),SF_Pixel);

/*-----------------------------------------------------------------------------
TDistortionMeshDrawingPolicy
-----------------------------------------------------------------------------*/

/**
* Distortion mesh drawing policy
*/
template<class DistortMeshPolicy>
class TDistortionMeshDrawingPolicy : public FMeshDrawingPolicy
{
public:
	/** context type */
	typedef FMeshDrawingPolicy::ElementDataType ElementDataType;

	/**
	* Constructor
	* @param InIndexBuffer - index buffer for rendering
	* @param InVertexFactory - vertex factory for rendering
	* @param InMaterialRenderProxy - material instance for rendering
	* @param bInOverrideWithShaderComplexity - whether to override with shader complexity
	*/
	TDistortionMeshDrawingPolicy(
		const FVertexFactory* InVertexFactory,
		const FMaterialRenderProxy* InMaterialRenderProxy,
		const FMaterial& MaterialResouce,
		bool bInitializeOffsets,
		bool bInOverrideWithShaderComplexity
		);

	// FMeshDrawingPolicy interface.

	/**
	* Match two draw policies
	* @param Other - draw policy to compare
	* @return true if the draw policies are a match
	*/
	bool Matches(const TDistortionMeshDrawingPolicy& Other) const;

	/**
	* Executes the draw commands which can be shared between any meshes using this drawer.
	* @param CI - The command interface to execute the draw commands on.
	* @param View - The view of the scene being drawn.
	*/
	void DrawShared(const FSceneView* View,FBoundShaderStateRHIParamRef BoundShaderState) const;
	
	/** 
	* Create bound shader state using the vertex decl from the mesh draw policy
	* as well as the shaders needed to draw the mesh
	* @return new bound shader state object
	*/
	FBoundShaderStateRHIRef CreateBoundShaderState();

	/**
	* Sets the render states for drawing a mesh.
	* @param PrimitiveSceneProxy - The primitive drawing the dynamic mesh.  If this is a view element, this will be NULL.
	* @param Mesh - mesh element with data needed for rendering
	* @param ElementData - context specific data for mesh rendering
	*/
	void SetMeshRenderState(
		const FSceneView& View,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const FMeshBatch& Mesh,
		int32 BatchElementIndex,
		bool bBackFace,
		const ElementDataType& ElementData
		) const;

private:
	/** vertex shader based on policy type */
	TDistortionMeshVS<DistortMeshPolicy>* VertexShader;

	TDistortionMeshHS<DistortMeshPolicy>* HullShader;
	TDistortionMeshDS<DistortMeshPolicy>* DomainShader;

	/** whether we are initializing offsets or accumulating them */
	bool bInitializeOffsets;
	/** pixel shader based on policy type */
	TDistortionMeshPS<DistortMeshPolicy>* DistortPixelShader;
	/** pixel shader used to initialize offsets */
//later	FShaderComplexityAccumulatePixelShader* InitializePixelShader;
};

/**
* Constructor
* @param InIndexBuffer - index buffer for rendering
* @param InVertexFactory - vertex factory for rendering
* @param InMaterialRenderProxy - material instance for rendering
*/
template<class DistortMeshPolicy> 
TDistortionMeshDrawingPolicy<DistortMeshPolicy>::TDistortionMeshDrawingPolicy(
	const FVertexFactory* InVertexFactory,
	const FMaterialRenderProxy* InMaterialRenderProxy,
	const FMaterial& InMaterialResource,
	bool bInInitializeOffsets,
	bool bInOverrideWithShaderComplexity
	)
:	FMeshDrawingPolicy(InVertexFactory,InMaterialRenderProxy,InMaterialResource,bInOverrideWithShaderComplexity)
,	bInitializeOffsets(bInInitializeOffsets)
{
	HullShader = NULL;
	DomainShader = NULL;

	const EMaterialTessellationMode MaterialTessellationMode = MaterialResource->GetTessellationMode();
	if(RHISupportsTessellation(GRHIShaderPlatform)
		&& InVertexFactory->GetType()->SupportsTessellationShaders() 
		&& MaterialTessellationMode != MTM_NoTessellation)
	{
		HullShader = InMaterialResource.GetShader<TDistortionMeshHS<DistortMeshPolicy>>(VertexFactory->GetType());
		DomainShader = InMaterialResource.GetShader<TDistortionMeshDS<DistortMeshPolicy>>(VertexFactory->GetType());
	}

	VertexShader = InMaterialResource.GetShader<TDistortionMeshVS<DistortMeshPolicy> >(InVertexFactory->GetType());

	if (bInitializeOffsets)
	{
//later		InitializePixelShader = GetGlobalShaderMap()->GetShader<FShaderComplexityAccumulatePixelShader>();
		DistortPixelShader = NULL;
	}
	else
	{
		DistortPixelShader = InMaterialResource.GetShader<TDistortionMeshPS<DistortMeshPolicy> >(InVertexFactory->GetType());
//later		InitializePixelShader = NULL;
	}
}

/**
* Match two draw policies
* @param Other - draw policy to compare
* @return true if the draw policies are a match
*/
template<class DistortMeshPolicy>
bool TDistortionMeshDrawingPolicy<DistortMeshPolicy>::Matches(
	const TDistortionMeshDrawingPolicy& Other
	) const
{
	return FMeshDrawingPolicy::Matches(Other) &&
		VertexShader == Other.VertexShader &&
		HullShader == Other.HullShader &&
		DomainShader == Other.DomainShader &&
		bInitializeOffsets == Other.bInitializeOffsets &&
		DistortPixelShader == Other.DistortPixelShader; //&&
	//later	InitializePixelShader == Other.InitializePixelShader;
}

/**
* Executes the draw commands which can be shared between any meshes using this drawer.
* @param CI - The command interface to execute the draw commands on.
* @param View - The view of the scene being drawn.
*/
template<class DistortMeshPolicy>
void TDistortionMeshDrawingPolicy<DistortMeshPolicy>::DrawShared(
	const FSceneView* View,
	FBoundShaderStateRHIParamRef BoundShaderState
	) const
{
	// Set shared mesh resources
	FMeshDrawingPolicy::DrawShared(View);

	// Set the actual shader & vertex declaration state
	RHISetBoundShaderState(BoundShaderState);

	// Set the translucent shader parameters for the material instance
	VertexShader->SetParameters(VertexFactory,MaterialRenderProxy,View);

	if(HullShader && DomainShader)
	{
		HullShader->SetParameters(MaterialRenderProxy,*View);
		DomainShader->SetParameters(MaterialRenderProxy,*View);
	}

	if (bOverrideWithShaderComplexity)
	{
		checkSlow(!bInitializeOffsets);
//later		TShaderMapRef<FShaderComplexityAccumulatePixelShader> ShaderComplexityPixelShader(GetGlobalShaderMap());
		//don't add any vertex complexity
//later		ShaderComplexityPixelShader->SetParameters(0, DistortPixelShader->GetNumInstructions());
	}
	else
	{
		if (bInitializeOffsets)
		{
//later			InitializePixelShader->SetParameters(0, 0);
		}
		else
		{
			DistortPixelShader->SetParameters(MaterialRenderProxy,*View);
		}
	}
}

/** 
* Create bound shader state using the vertex decl from the mesh draw policy
* as well as the shaders needed to draw the mesh
* @return new bound shader state object
*/
template<class DistortMeshPolicy>
FBoundShaderStateRHIRef TDistortionMeshDrawingPolicy<DistortMeshPolicy>::CreateBoundShaderState()
{
	FPixelShaderRHIParamRef PixelShaderRHIRef = NULL;

	if (bOverrideWithShaderComplexity)
	{
		checkSlow(!bInitializeOffsets);
//later		TShaderMapRef<FShaderComplexityAccumulatePixelShader> ShaderComplexityAccumulatePixelShader(GetGlobalShaderMap());
//later		PixelShaderRHIRef = ShaderComplexityAccumulatePixelShader->GetPixelShader();
	}

	if (bInitializeOffsets)
	{
//later		PixelShaderRHIRef = InitializePixelShader->GetPixelShader();
	}
	else
	{
		PixelShaderRHIRef = DistortPixelShader->GetPixelShader();
	}

	FBoundShaderStateRHIRef BoundShaderState;

	BoundShaderState = RHICreateBoundShaderState(
		FMeshDrawingPolicy::GetVertexDeclaration(), 
		VertexShader->GetVertexShader(),
		GETSAFERHISHADER_HULL(HullShader), 
		GETSAFERHISHADER_DOMAIN(DomainShader),
		PixelShaderRHIRef,
		FGeometryShaderRHIRef());

	return BoundShaderState;
}

/**
* Sets the render states for drawing a mesh.
* @param PrimitiveSceneProxy - The primitive drawing the dynamic mesh.  If this is a view element, this will be NULL.
* @param Mesh - mesh element with data needed for rendering
* @param ElementData - context specific data for mesh rendering
*/
template<class DistortMeshPolicy>
void TDistortionMeshDrawingPolicy<DistortMeshPolicy>::SetMeshRenderState(
	const FSceneView& View,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	const FMeshBatch& Mesh,
	int32 BatchElementIndex,
	bool bBackFace,
	const ElementDataType& ElementData
	) const
{
	const FMeshBatchElement& BatchElement = Mesh.Elements[BatchElementIndex];

	EmitMeshDrawEvents(PrimitiveSceneProxy, Mesh);

	// Set transforms
	VertexShader->SetMesh(VertexFactory,View,PrimitiveSceneProxy,BatchElement);

	if(HullShader && DomainShader)
	{
		HullShader->SetMesh(VertexFactory,View,PrimitiveSceneProxy,BatchElement);
		DomainShader->SetMesh(VertexFactory,View,PrimitiveSceneProxy,BatchElement);
	}
	

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	// Don't set pixel shader constants if we are overriding with the shader complexity pixel shader
	if (!bOverrideWithShaderComplexity)
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	{
		if (!bInitializeOffsets)
		{
			DistortPixelShader->SetMesh(VertexFactory,View,PrimitiveSceneProxy,BatchElement);
		}
	}
	
	// Set rasterizer state.
	RHISetRasterizerState(GetStaticRasterizerState<true>(
		(Mesh.bWireframe || IsWireframe()) ? FM_Wireframe : FM_Solid,
		IsTwoSided() ? CM_None : (XOR( XOR(View.bReverseCulling,bBackFace), Mesh.ReverseCulling) ? CM_CCW : CM_CW)));
}

/*-----------------------------------------------------------------------------
TDistortionMeshDrawingPolicyFactory
-----------------------------------------------------------------------------*/

/**
* Distortion mesh draw policy factory. 
* Creates the policies needed for rendering a mesh based on its material
*/
template<class DistortMeshPolicy>
class TDistortionMeshDrawingPolicyFactory
{
public:
	enum { bAllowSimpleElements = false };
	typedef bool ContextType;

	/**
	* Render a dynamic mesh using a distortion mesh draw policy 
	* @return true if the mesh rendered
	*/
	static bool DrawDynamicMesh(
		const FSceneView& View,
		ContextType DrawingContext,
		const FMeshBatch& Mesh,
		bool bBackFace,
		bool bPreFog,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		FHitProxyId HitProxyId
		);

	/**
	* Render a dynamic mesh using a distortion mesh draw policy 
	* @return true if the mesh rendered
	*/
	static bool DrawStaticMesh(
		const FSceneView* View,
		ContextType DrawingContext,
		const FStaticMesh& StaticMesh,
		uint64 BatchElementMask,
		bool bBackFace,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		FHitProxyId HitProxyId
		);

	static bool IsMaterialIgnored(const FMaterialRenderProxy* Material);
};

/**
* Render a dynamic mesh using a distortion mesh draw policy 
* @return true if the mesh rendered
*/
template<class DistortMeshPolicy>
bool TDistortionMeshDrawingPolicyFactory<DistortMeshPolicy>::DrawDynamicMesh(
	const FSceneView& View,
	ContextType bInitializeOffsets,
	const FMeshBatch& Mesh,
	bool bBackFace,
	bool bPreFog,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	FHitProxyId HitProxyId
	)
{
	bool bDistorted = Mesh.MaterialRenderProxy && Mesh.MaterialRenderProxy->GetMaterial(GRHIFeatureLevel)->IsDistorted();

	if(bDistorted && !bBackFace)
	{
		// draw dynamic mesh element using distortion mesh policy
		TDistortionMeshDrawingPolicy<DistortMeshPolicy> DrawingPolicy(
			Mesh.VertexFactory,
			Mesh.MaterialRenderProxy,
			*Mesh.MaterialRenderProxy->GetMaterial(GRHIFeatureLevel),
			bInitializeOffsets,
			View.Family->EngineShowFlags.ShaderComplexity
			);
		DrawingPolicy.DrawShared(&View,DrawingPolicy.CreateBoundShaderState());
		for( int32 BatchElementIndex=0;BatchElementIndex < Mesh.Elements.Num();BatchElementIndex++ )
		{
			DrawingPolicy.SetMeshRenderState(View,PrimitiveSceneProxy,Mesh,BatchElementIndex,bBackFace,typename TDistortionMeshDrawingPolicy<DistortMeshPolicy>::ElementDataType());
			DrawingPolicy.DrawMesh(Mesh,BatchElementIndex);
		}

		return true;
	}
	else
	{
		return false;
	}
}

/**
* Render a dynamic mesh using a distortion mesh draw policy 
* @return true if the mesh rendered
*/
template<class DistortMeshPolicy>
bool TDistortionMeshDrawingPolicyFactory<DistortMeshPolicy>::DrawStaticMesh(
	const FSceneView* View,
	ContextType bInitializeOffsets,
	const FStaticMesh& StaticMesh,
	uint64 BatchElementMask,
	bool bBackFace,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	FHitProxyId HitProxyId
	)
{
	bool bDistorted = StaticMesh.MaterialRenderProxy && StaticMesh.MaterialRenderProxy->GetMaterial(GRHIFeatureLevel)->IsDistorted();
	
	if(bDistorted && !bBackFace)
	{
		// draw static mesh element using distortion mesh policy
		TDistortionMeshDrawingPolicy<DistortMeshPolicy> DrawingPolicy(
			StaticMesh.VertexFactory,
			StaticMesh.MaterialRenderProxy,
			*StaticMesh.MaterialRenderProxy->GetMaterial(GRHIFeatureLevel),
			bInitializeOffsets,
			View->Family->EngineShowFlags.ShaderComplexity
			);
		DrawingPolicy.DrawShared(View,DrawingPolicy.CreateBoundShaderState());
		int32 BatchElementIndex = 0;
		do
		{
			if(BatchElementMask & 1)
			{
				DrawingPolicy.SetMeshRenderState(*View,PrimitiveSceneProxy,StaticMesh,BatchElementIndex,bBackFace,typename TDistortionMeshDrawingPolicy<DistortMeshPolicy>::ElementDataType());
				DrawingPolicy.DrawMesh(StaticMesh, BatchElementIndex);
			}
			BatchElementMask >>= 1;
			BatchElementIndex++;
		} while(BatchElementMask);

		return true;
	}
	else
	{
		return false;
	}
}

template<typename DistortMeshPolicy>
bool TDistortionMeshDrawingPolicyFactory<DistortMeshPolicy>::IsMaterialIgnored(const FMaterialRenderProxy* MaterialRenderProxy)
{
	// Non-distorted materials are ignored when rendering distortion.
	return MaterialRenderProxy && !MaterialRenderProxy->GetMaterial(GRHIFeatureLevel)->IsDistorted();
}

/*-----------------------------------------------------------------------------
	FDistortionPrimSet
-----------------------------------------------------------------------------*/

bool FDistortionPrimSet::DrawAccumulatedOffsets(const FViewInfo* ViewInfo,bool bInitializeOffsets)
{
	bool bDirty=false;

	// Draw the view's elements with the translucent drawing policy.
	bDirty |= DrawViewElements<TDistortionMeshDrawingPolicyFactory<FDistortMeshAccumulatePolicy> >(
		*ViewInfo,
		bInitializeOffsets,
		0,	// DPG Index?
		false // Distortion is rendered post fog.
		);

	if( Prims.Num() )
	{
		// For drawing scene prims with dynamic relevance.
		TDynamicPrimitiveDrawer<TDistortionMeshDrawingPolicyFactory<FDistortMeshAccumulatePolicy> > Drawer(
			ViewInfo,
			bInitializeOffsets,
			false // Distortion is rendered post fog.
			);

		// Draw sorted scene prims
		for( int32 PrimIdx=0; PrimIdx < Prims.Num(); PrimIdx++ )
		{
			FPrimitiveSceneProxy* PrimitiveSceneProxy = Prims[PrimIdx];
			const FPrimitiveViewRelevance& ViewRelevance = ViewInfo->PrimitiveViewRelevanceMap[PrimitiveSceneProxy->GetPrimitiveSceneInfo()->GetIndex()];

			if( ViewRelevance.bDistortionRelevance )
			{
				// Render dynamic scene prim
				if( ViewRelevance.bDynamicRelevance )
				{				
					Drawer.SetPrimitive(PrimitiveSceneProxy);
					PrimitiveSceneProxy->DrawDynamicElements(
						&Drawer,
						ViewInfo
						);
				}
				// Render static scene prim
				if( ViewRelevance.bStaticRelevance )
				{
					// Render static meshes from static scene prim
					for( int32 StaticMeshIdx=0; StaticMeshIdx < PrimitiveSceneProxy->GetPrimitiveSceneInfo()->StaticMeshes.Num(); StaticMeshIdx++ )
					{
						FStaticMesh& StaticMesh = PrimitiveSceneProxy->GetPrimitiveSceneInfo()->StaticMeshes[StaticMeshIdx];
						if( ViewInfo->StaticMeshVisibilityMap[StaticMesh.Id]
							// Only render static mesh elements using translucent materials
							&& StaticMesh.IsTranslucent() )
						{
							bDirty |= TDistortionMeshDrawingPolicyFactory<FDistortMeshAccumulatePolicy>::DrawStaticMesh(
								ViewInfo,
								bInitializeOffsets,
								StaticMesh,
								StaticMesh.Elements.Num() == 1 ? 1 : ViewInfo->StaticMeshBatchVisibility[StaticMesh.Id],
								false,
								PrimitiveSceneProxy,
								StaticMesh.HitProxyId
								);
						}
					}
				}
			}
		}
		// Mark dirty if dynamic drawer rendered
		bDirty |= Drawer.IsDirty();
	}
	return bDirty;
}

void FDistortionPrimSet::AddScenePrimitive(FPrimitiveSceneProxy* PrimitiveSceneProxy,const FViewInfo& ViewInfo)
{
	Prims.Add(PrimitiveSceneProxy);
}

/*-----------------------------------------------------------------------------
	FDeferredShadingSceneRenderer
-----------------------------------------------------------------------------*/

/** 
 * Renders the scene's distortion 
 */
void FDeferredShadingSceneRenderer::RenderDistortion()
{
	SCOPED_DRAW_EVENT(Distortion, DEC_SCENE_ITEMS);

	// do we need to render the distortion pass?
	bool bRender=false;
	for(int32 ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		const FViewInfo& View = Views[ViewIndex];
		if( View.DistortionPrimSet.NumPrims() > 0)
		{
			bRender=true;
			break;
		}
	}

	bool bDirty = false;

	// Render accumulated distortion offsets
	if( bRender)
	{
		SCOPED_DRAW_EVENT(DistortionAccum, DEC_SCENE_ITEMS);

		GSceneRenderTargets.BeginRenderingDistortionAccumulation();

		for(int32 ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
		{
			SCOPED_CONDITIONAL_DRAW_EVENTF(EventView, Views.Num() > 1, DEC_SCENE_ITEMS, TEXT("View%d"), ViewIndex);

			FViewInfo& View = Views[ViewIndex];
			// viewport to match view size
			RHISetViewport(View.ViewRect.Min.X,View.ViewRect.Min.Y,0.0f,View.ViewRect.Max.X,View.ViewRect.Max.Y,1.0f);

			// clear offsets to 0, stencil to 0
			RHIClear(true,FLinearColor(0,0,0,0),false,0,true,0,FIntRect());

			// enable depth test but disable depth writes
			// Note, this is a reversed Z depth surface, using CF_GreaterEqual.
			RHISetDepthStencilState(TStaticDepthStencilState<false,CF_GreaterEqual>::GetRHI());

			// additive blending of offsets (or complexity if the shader complexity viewmode is enabled)
			RHISetBlendState(TStaticBlendState<CW_RGBA,BO_Add,BF_One,BF_One,BO_Add,BF_One,BF_One>::GetRHI());

			// draw only distortion meshes to accumulate their offsets
			bDirty |= View.DistortionPrimSet.DrawAccumulatedOffsets(&View,false);
		}

		if (bDirty)
		{
			// restore default stencil state
			// Note, this is a reversed Z depth surface, using CF_GreaterEqual.
			RHISetDepthStencilState(TStaticDepthStencilState<true,CF_GreaterEqual>::GetRHI());

			// resolve using the current ResolveParams 
			GSceneRenderTargets.FinishRenderingDistortionAccumulation();
		}
	}

	if(bDirty)
	{
		SCOPED_DRAW_EVENT(DistortionApply, DEC_SCENE_ITEMS);

		GSceneRenderTargets.ResolveSceneColor(FResolveRect(0, 0, ViewFamily.FamilySizeX, ViewFamily.FamilySizeY));

// OCULUS BEGIN: select ONE render target for all views (eyes)
		TRefCountPtr<IPooledRenderTarget> NewSceneColor;
		GRenderTargetPool.FindFreeElement(GSceneRenderTargets.SceneColor->GetDesc(), NewSceneColor, TEXT("RefractedSceneColor"));
		const FSceneRenderTargetItem& DestRenderTarget = NewSceneColor->GetRenderTargetItem();
// OCULUS END

		// Apply distortion as a full-screen pass		
		for(int32 ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
		{
			SCOPED_CONDITIONAL_DRAW_EVENTF(EventView, Views.Num() > 1, DEC_SCENE_ITEMS, TEXT("View%d"), ViewIndex);

			FViewInfo& View = Views[ViewIndex];

//			const FSceneView& View = Context.View;

			// set the state
			RHISetBlendState(TStaticBlendState<>::GetRHI());
			RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
			RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());

			{
// OCULUS BEGIN: Weird, different eyes may have different render targets!
// 				TRefCountPtr<IPooledRenderTarget> NewSceneColor;
// 				GRenderTargetPool.FindFreeElement(GSceneRenderTargets.SceneColor->GetDesc(), NewSceneColor, TEXT("RefractedSceneColor"));
// 				const FSceneRenderTargetItem& DestRenderTarget = NewSceneColor->GetRenderTargetItem(); 
// OCULUS END

				RHISetRenderTarget( DestRenderTarget.TargetableTexture, FTextureRHIParamRef());

				// useful wehen we move this into the compositing graph
				FRenderingCompositePassContext Context(View);

				// Set the view family's render target/viewport.
				Context.SetViewportAndCallRHI(View.ViewRect);

				TShaderMapRef<FPostProcessVS> VertexShader(GetGlobalShaderMap());
				TShaderMapRef<FDistortionApplyScreenPS> PixelShader(GetGlobalShaderMap());

				static FGlobalBoundShaderState BoundShaderState;
				SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

				VertexShader->SetParameters(Context);
				PixelShader->SetParameters(Context);

				// Draw a quad mapping scene color to the view's render target
				DrawRectangle(
					0, 0,
					View.ViewRect.Width(), View.ViewRect.Height(),
					View.ViewRect.Min.X, View.ViewRect.Min.Y, 
					View.ViewRect.Width(), View.ViewRect.Height(),
					View.ViewRect.Size(),
					GSceneRenderTargets.SceneColor->GetDesc().Extent,
					EDRF_UseTriangleOptimization);

				RHICopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());

// OCULUS BEGIN
// 				GSceneRenderTargets.SceneColor = NewSceneColor;
// 				check(GSceneRenderTargets.SceneColor);
// OCULUS END				

			}
		
		}

// OCULUS BEGIN
		GSceneRenderTargets.SceneColor = NewSceneColor;
		check(GSceneRenderTargets.SceneColor);
// OCULUS END				
		GSceneRenderTargets.FinishRenderingSceneColor(false);
	}
}
