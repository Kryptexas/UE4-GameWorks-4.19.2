// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SceneHitProxyRendering.cpp: Scene hit proxy rendering.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "RenderResource.h"

/**
 * A vertex shader for rendering the depth of a mesh.
 */
class FHitProxyVS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FHitProxyVS,MeshMaterial);

public:

	void SetParameters(const FMaterialRenderProxy* MaterialRenderProxy,const FSceneView& View)
	{
		FMeshMaterialShader::SetParameters(GetVertexShader(),MaterialRenderProxy,*MaterialRenderProxy->GetMaterial(GRHIFeatureLevel),View,ESceneRenderTargetsMode::SetTextures);
	}

	void SetMesh(const FVertexFactory* VertexFactory,const FSceneView& View,const FPrimitiveSceneProxy* Proxy,const FMeshBatchElement& BatchElement)
	{
		FMeshMaterialShader::SetMesh(GetVertexShader(),VertexFactory,View,Proxy,BatchElement);
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FMeshMaterialShader::Serialize(Ar);
		return bShaderHasOutdatedParameters;
	}

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		// Only compile the hit proxy vertex shader on PC
		return IsPCPlatform(Platform) && 
			IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3) &&
			// and only compile for the default material or materials that are masked.
			(Material->IsSpecialEngineMaterial() || Material->IsMasked() || Material->MaterialModifiesMeshPosition() || Material->IsTwoSided());
	}

protected:

	FHitProxyVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FMeshMaterialShader(Initializer)
	{}
	FHitProxyVS() {}
};

IMPLEMENT_MATERIAL_SHADER_TYPE(,FHitProxyVS,TEXT("HitProxyVertexShader"),TEXT("Main"),SF_Vertex); 

/**
 * A hull shader for rendering the depth of a mesh.
 */
class FHitProxyHS : public FBaseHS
{
	DECLARE_SHADER_TYPE(FHitProxyHS,MeshMaterial);
protected:

	FHitProxyHS() {}

	FHitProxyHS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FBaseHS(Initializer)
	{}

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return FBaseHS::ShouldCache(Platform, Material, VertexFactoryType)
			&& FHitProxyVS::ShouldCache(Platform,Material,VertexFactoryType);
	}
};

/**
 * A domain shader for rendering the depth of a mesh.
 */
class FHitProxyDS : public FBaseDS
{
	DECLARE_SHADER_TYPE(FHitProxyDS,MeshMaterial);

protected:

	FHitProxyDS() {}

	FHitProxyDS(const ShaderMetaType::CompiledShaderInitializerType& Initializer) :
		FBaseDS(Initializer)
	{}

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return FBaseDS::ShouldCache(Platform, Material, VertexFactoryType)
			&& FHitProxyVS::ShouldCache(Platform,Material,VertexFactoryType);
	}
};

IMPLEMENT_MATERIAL_SHADER_TYPE(,FHitProxyHS,TEXT("HitProxyVertexShader"),TEXT("MainHull"),SF_Hull); 
IMPLEMENT_MATERIAL_SHADER_TYPE(,FHitProxyDS,TEXT("HitProxyVertexShader"),TEXT("MainDomain"),SF_Domain);

/**
 * A pixel shader for rendering the depth of a mesh.
 */
class FHitProxyPS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FHitProxyPS,MeshMaterial);
public:

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		// Only compile the hit proxy vertex shader on PC
		return IsPCPlatform(Platform) 
			&& IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3) 
			// and only compile for default materials or materials that are masked.
			&& (Material->IsSpecialEngineMaterial() || Material->IsMasked() || Material->MaterialModifiesMeshPosition() || Material->IsTwoSided());
	}

	FHitProxyPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FMeshMaterialShader(Initializer)
	{
		HitProxyId.Bind(Initializer.ParameterMap,TEXT("HitProxyId"), SPF_Mandatory);
	}

	FHitProxyPS() {}

	void SetParameters(const FMaterialRenderProxy* MaterialRenderProxy,const FSceneView& View)
	{
		FMeshMaterialShader::SetParameters(GetPixelShader(),MaterialRenderProxy,*MaterialRenderProxy->GetMaterial(GRHIFeatureLevel),View,ESceneRenderTargetsMode::SetTextures);
	}

	void SetMesh(const FVertexFactory* VertexFactory,const FSceneView& View,const FPrimitiveSceneProxy* Proxy,const FMeshBatchElement& BatchElement)
	{
		FMeshMaterialShader::SetMesh(GetPixelShader(),VertexFactory,View,Proxy,BatchElement);
	}

	void SetHitProxyId(FHitProxyId HitProxyIdValue)
	{
		SetShaderValue(GetPixelShader(),HitProxyId,HitProxyIdValue.GetColor().ReinterpretAsLinear());
	}

	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FMeshMaterialShader::Serialize(Ar);
		Ar << HitProxyId;
		return bShaderHasOutdatedParameters;
	}

private:
	FShaderParameter HitProxyId;
};

IMPLEMENT_MATERIAL_SHADER_TYPE(,FHitProxyPS,TEXT("HitProxyPixelShader"),TEXT("Main"),SF_Pixel);

FHitProxyDrawingPolicy::FHitProxyDrawingPolicy(
	const FVertexFactory* InVertexFactory,
	const FMaterialRenderProxy* InMaterialRenderProxy
	):
	FMeshDrawingPolicy(InVertexFactory,InMaterialRenderProxy,*InMaterialRenderProxy->GetMaterial(GRHIFeatureLevel))
{
	HullShader = NULL;
	DomainShader = NULL;

	EMaterialTessellationMode MaterialTessellationMode = MaterialResource->GetTessellationMode();
	if(RHISupportsTessellation(GRHIShaderPlatform)
		&& InVertexFactory->GetType()->SupportsTessellationShaders() 
		&& MaterialTessellationMode != MTM_NoTessellation)
	{
		HullShader = MaterialResource->GetShader<FHitProxyHS>(VertexFactory->GetType());
		DomainShader = MaterialResource->GetShader<FHitProxyDS>(VertexFactory->GetType());
	}
	VertexShader = MaterialResource->GetShader<FHitProxyVS>(InVertexFactory->GetType());
	PixelShader = MaterialResource->GetShader<FHitProxyPS>(InVertexFactory->GetType());
}

void FHitProxyDrawingPolicy::DrawShared(const FSceneView* View,FBoundShaderStateRHIParamRef BoundShaderState) const
{
	// Set the actual shader & vertex declaration state
	RHISetBoundShaderState( BoundShaderState);

	// Set the depth-only shader parameters for the material.
	VertexShader->SetParameters(MaterialRenderProxy,*View);
	PixelShader->SetParameters(MaterialRenderProxy,*View);

	if(HullShader && DomainShader)
	{
		HullShader->SetParameters(MaterialRenderProxy,*View);
		DomainShader->SetParameters(MaterialRenderProxy,*View);
	}

	// Set the shared mesh resources.
	FMeshDrawingPolicy::DrawShared(View);
}

/** 
* Create bound shader state using the vertex decl from the mesh draw policy
* as well as the shaders needed to draw the mesh
* @return new bound shader state object
*/
FBoundShaderStateRHIRef FHitProxyDrawingPolicy::CreateBoundShaderState()
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

void FHitProxyDrawingPolicy::SetMeshRenderState(
	const FSceneView& View,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	const FMeshBatch& Mesh,
	int32 BatchElementIndex,
	bool bBackFace,
	const FHitProxyId HitProxyId
	) const
{
	EmitMeshDrawEvents(PrimitiveSceneProxy, Mesh);

	const FMeshBatchElement& BatchElement = Mesh.Elements[BatchElementIndex];

	VertexShader->SetMesh(VertexFactory,View,PrimitiveSceneProxy,BatchElement);

	if(HullShader && DomainShader)
	{
		HullShader->SetMesh(VertexFactory,View,PrimitiveSceneProxy,BatchElement);
		DomainShader->SetMesh(VertexFactory,View,PrimitiveSceneProxy,BatchElement);
	}

	PixelShader->SetMesh(VertexFactory,View,PrimitiveSceneProxy,BatchElement);
	// Per-instance hitproxies are supplied by the vertex factory
	if( PrimitiveSceneProxy && PrimitiveSceneProxy->HasPerInstanceHitProxies() )
	{
		PixelShader->SetHitProxyId( FColor(0) );
	}
	else
	{
		PixelShader->SetHitProxyId( HitProxyId);	
	}

	RHISetRasterizerState(GetStaticRasterizerState<false>(
		(Mesh.bWireframe || IsWireframe()) ? FM_Wireframe : FM_Solid,
		((IsTwoSided() && !NeedsBackfacePass()) || Mesh.bDisableBackfaceCulling) ? CM_None :
			(XOR(XOR(View.bReverseCulling,bBackFace), Mesh.ReverseCulling) ? CM_CCW : CM_CW)
		));
}

void FHitProxyDrawingPolicyFactory::AddStaticMesh(FScene* Scene,FStaticMesh* StaticMesh,ContextType)
{
	checkSlow( Scene->RequiresHitProxies() );

	// Add the static mesh to the DPG's hit proxy draw list.
	const FMaterialRenderProxy* MaterialRenderProxy = StaticMesh->MaterialRenderProxy;
	const FMaterial* Material = MaterialRenderProxy->GetMaterial(GRHIFeatureLevel);
	if ( !Material->IsMasked() && !Material->IsTwoSided() && !Material->MaterialModifiesMeshPosition() )
	{
		// Default material doesn't handle masked, and doesn't have the correct bIsTwoSided setting.
		MaterialRenderProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy(false);
	}
	
	Scene->HitProxyDrawList.AddMesh(
		StaticMesh,
		StaticMesh->HitProxyId,
		FHitProxyDrawingPolicy(StaticMesh->VertexFactory,MaterialRenderProxy)
		);

#if WITH_EDITOR
	// If the mesh isn't translucent then we'll also add it to the "opaque-only" draw list.  Depending
	// on user preferences in the editor, we may use this draw list to disallow selection of
	// translucent objects in perspective viewports
	if( !IsTranslucentBlendMode( Material->GetBlendMode() ) )
	{
		Scene->HitProxyDrawList_OpaqueOnly.AddMesh(
			StaticMesh,
			StaticMesh->HitProxyId,
			FHitProxyDrawingPolicy(StaticMesh->VertexFactory,MaterialRenderProxy)
			);
	}
#endif

}

bool FHitProxyDrawingPolicyFactory::DrawDynamicMesh(
	const FSceneView& View,
	ContextType DrawingContext,
	const FMeshBatch& Mesh,
	bool bBackFace,
	bool bPreFog,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	FHitProxyId HitProxyId
	)
{
	if (!PrimitiveSceneProxy || PrimitiveSceneProxy->IsSelectable())
	{
		const FMaterialRenderProxy* MaterialRenderProxy = Mesh.MaterialRenderProxy;
		const FMaterial* Material = MaterialRenderProxy->GetMaterial(GRHIFeatureLevel);
#if WITH_EDITOR
		const HHitProxy* HitProxy = GetHitProxyById( HitProxyId );
		// Only draw translucent primitives to the hit proxy if the user wants those objects to be selectable
		if( View.bAllowTranslucentPrimitivesInHitProxy || !IsTranslucentBlendMode( Material->GetBlendMode() ) || ( HitProxy && HitProxy->AlwaysAllowsTranslucentPrimitives() ) )
#endif
		{
			if ( !Material->IsMasked() && !Material->IsTwoSided() && !Material->MaterialModifiesMeshPosition() )
			{
				// Default material doesn't handle masked, and doesn't have the correct bIsTwoSided setting.
				MaterialRenderProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy(false);
			}
			FHitProxyDrawingPolicy DrawingPolicy( Mesh.VertexFactory, MaterialRenderProxy );
			DrawingPolicy.DrawShared(&View,DrawingPolicy.CreateBoundShaderState());
			for( int32 BatchElementIndex=0;BatchElementIndex<Mesh.Elements.Num();BatchElementIndex++ )
			{
				DrawingPolicy.SetMeshRenderState(View,PrimitiveSceneProxy,Mesh,BatchElementIndex,bBackFace,HitProxyId);
				DrawingPolicy.DrawMesh(Mesh,BatchElementIndex);
			}
			return true;
		}
	}
	return false;
}

void FDeferredShadingSceneRenderer::RenderHitProxies()
{
#if WITH_EDITOR
	// Initialize global system textures (pass-through if already initialized).
	GSystemTextures.InitializeTextures();

	// Allocate the maximum scene render target space for the current view family.
	GSceneRenderTargets.Allocate(ViewFamily);

	// Write to the hit proxy render target.
	GSceneRenderTargets.BeginRenderingHitProxies();

	// Clear color for each view.
	for(int32 ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		const FViewInfo& View = Views[ViewIndex];
		RHISetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);
		RHIClear(true,FLinearColor::White,false,1,false,0, FIntRect());
	}

	// Find the visible primitives.
	InitViews();

	// Dynamic vertex and index buffers need to be committed before rendering.
	FGlobalDynamicVertexBuffer::Get().Commit();
	FGlobalDynamicIndexBuffer::Get().Commit();

	// Depth tests + writes, no alpha blending.
	// Note, this is a reversed Z depth surface, using CF_GreaterEqual.
	RHISetDepthStencilState(TStaticDepthStencilState<true,CF_GreaterEqual>::GetRHI());
	RHISetBlendState(TStaticBlendState<>::GetRHI());

	const bool bNeedToSwitchVerticalAxis = IsES2Platform(GRHIShaderPlatform) && !IsPCPlatform(GRHIShaderPlatform);

	for(int32 ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		const FViewInfo& View = Views[ViewIndex];

		// Set the device viewport for the view.
		RHISetViewport(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0.0f, View.ViewRect.Max.X, View.ViewRect.Max.Y, 1.0f);

		// Clear the depth buffer for each DPG.
		// Note, this is a reversed Z depth surface, so 0.0f is the far plane.
		RHIClear(false,FLinearColor::Black,true,0.0f,true,0, FIntRect());

			
		// Adjust the visibility map for this view
		if( !View.bAllowTranslucentPrimitivesInHitProxy )
		{
			// Draw the scene's hit proxy draw lists. (opaque primitives only)
			Scene->HitProxyDrawList_OpaqueOnly.DrawVisible(View,View.StaticMeshVisibilityMap,View.StaticMeshBatchVisibility);
		}
		else
		{
			// Draw the scene's hit proxy draw lists.
			Scene->HitProxyDrawList.DrawVisible(View,View.StaticMeshVisibilityMap,View.StaticMeshBatchVisibility);
		}

		const bool bPreFog = true;
		const bool bHitTesting = true;

		// Draw all dynamic primitives using the hit proxy drawing policy.
		DrawDynamicPrimitiveSet<FHitProxyDrawingPolicyFactory>(
			View,
			View.VisibleDynamicPrimitives,
			FHitProxyDrawingPolicyFactory::ContextType(),
			bPreFog,
			bHitTesting
			);

		// Draw all visible editor primitives using the hit proxy drawing policy
		DrawDynamicPrimitiveSet<FHitProxyDrawingPolicyFactory>(
			View,
			View.VisibleEditorPrimitives,
			FHitProxyDrawingPolicyFactory::ContextType(),
			bPreFog,
			bHitTesting
			);


		// Draw the view's elements.
		DrawViewElements<FHitProxyDrawingPolicyFactory>(View,FHitProxyDrawingPolicyFactory::ContextType(),SDPG_World,bPreFog);

		// Draw the view's batched simple elements(lines, sprites, etc).
		View.BatchedViewElements.Draw(bNeedToSwitchVerticalAxis, View.ViewProjectionMatrix, View.ViewRect.Width(), View.ViewRect.Height(), true);

		// Some elements should never be occluded (e.g. gizmos).
		// So we render those twice, first to overwrite potentially nearer objects,
		// then again to allows proper occlusion within those elements.
		RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());
		// Draw the view's foreground elements last.
		DrawViewElements<FHitProxyDrawingPolicyFactory>(View,FHitProxyDrawingPolicyFactory::ContextType(),SDPG_Foreground,bPreFog);

		View.TopBatchedViewElements.Draw(bNeedToSwitchVerticalAxis, View.ViewProjectionMatrix, View.ViewRect.Width(), View.ViewRect.Height(), true);

		// Note, this is a reversed Z depth surface, using CF_GreaterEqual.
		RHISetDepthStencilState(TStaticDepthStencilState<true,CF_GreaterEqual>::GetRHI());
		// Draw the view's foreground elements last.
		DrawViewElements<FHitProxyDrawingPolicyFactory>(View,FHitProxyDrawingPolicyFactory::ContextType(),SDPG_Foreground,bPreFog);

		View.TopBatchedViewElements.Draw(bNeedToSwitchVerticalAxis, View.ViewProjectionMatrix, View.ViewRect.Width(), View.ViewRect.Height(), true);
	}

	// Finish drawing to the hit proxy render target.
	GSceneRenderTargets.FinishRenderingHitProxies();

	// After scene rendering, disable the depth buffer.
	RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());

	//
	// Copy the hit proxy buffer into the view family's render target.
	//
	
	// Set up a FTexture that is used to draw the hit proxy buffer to the view family's render target.
	FTexture HitProxyRenderTargetTexture;
	HitProxyRenderTargetTexture.TextureRHI = GSceneRenderTargets.GetHitProxyTexture();
	HitProxyRenderTargetTexture.SamplerStateRHI = TStaticSamplerState<>::GetRHI();

	// Generate the vertices and triangles mapping the hit proxy RT pixels into the view family's RT pixels.
	FBatchedElements BatchedElements;
	for(int32 ViewIndex = 0;ViewIndex < Views.Num();ViewIndex++)
	{
		const FViewInfo& View = Views[ViewIndex];

		FIntPoint BufferSize = GSceneRenderTargets.GetBufferSizeXY();
		float InvBufferSizeX = 1.0f / BufferSize.X;
		float InvBufferSizeY = 1.0f / BufferSize.Y;

		const float U0 = View.ViewRect.Min.X * InvBufferSizeX;
		const float V0 = View.ViewRect.Min.Y * InvBufferSizeY;
		const float U1 = View.ViewRect.Max.X * InvBufferSizeX;
		const float V1 = View.ViewRect.Max.Y * InvBufferSizeY;

		const int32 V00 = BatchedElements.AddVertex(FVector4(View.ViewRect.Min.X,	View.ViewRect.Min.Y, 0, 1),FVector2D(U0, V0),	FLinearColor::White, FHitProxyId());
		const int32 V10 = BatchedElements.AddVertex(FVector4(View.ViewRect.Max.X,	View.ViewRect.Min.Y,	0, 1),FVector2D(U1, V0),	FLinearColor::White, FHitProxyId());
		const int32 V01 = BatchedElements.AddVertex(FVector4(View.ViewRect.Min.X,	View.ViewRect.Max.Y,	0, 1),FVector2D(U0, V1),	FLinearColor::White, FHitProxyId());
		const int32 V11 = BatchedElements.AddVertex(FVector4(View.ViewRect.Max.X,	View.ViewRect.Max.Y,	0, 1),FVector2D(U1, V1),	FLinearColor::White, FHitProxyId());

		BatchedElements.AddTriangle(V00,V10,V11,&HitProxyRenderTargetTexture,BLEND_Opaque);
		BatchedElements.AddTriangle(V00,V11,V01,&HitProxyRenderTargetTexture,BLEND_Opaque);
	}

	// Generate a transform which maps from view family RT pixel coordinates to Normalized Device Coordinates.
	FIntPoint RenderTargetSize = ViewFamily.RenderTarget->GetSizeXY();

	const FMatrix PixelToView =
		FTranslationMatrix(FVector(-GPixelCenterOffset,-GPixelCenterOffset,0)) *
			FMatrix(
				FPlane(	1.0f / ((float)RenderTargetSize.X / 2.0f),	0.0,										0.0f,	0.0f	),
				FPlane(	0.0f,										-GProjectionSignY / ((float)RenderTargetSize.Y / 2.0f),	0.0f,	0.0f	),
				FPlane(	0.0f,										0.0f,										1.0f,	0.0f	),
				FPlane(	-1.0f,										GProjectionSignY,										0.0f,	1.0f	)
				);

	// Draw the triangles to the view family's render target.
	RHISetRenderTarget(ViewFamily.RenderTarget->GetRenderTargetTexture(),FTextureRHIRef());
	BatchedElements.Draw(
				bNeedToSwitchVerticalAxis,
				PixelToView,
				RenderTargetSize.X,
				RenderTargetSize.Y,
				false,
				1.0f
				);
#endif
}
