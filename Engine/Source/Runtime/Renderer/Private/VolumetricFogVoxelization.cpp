// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VolumetricFogVoxelization.cpp
=============================================================================*/

#include "VolumetricFog.h"
#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "SceneUtils.h"
#include "VolumetricFogShared.h"

int32 GVolumetricFogVoxelizationSlicesPerGSPass = 8;
FAutoConsoleVariableRef CVarVolumetricFogVoxelizationSlicesPerPass(
	TEXT("r.VolumetricFog.VoxelizationSlicesPerGSPass"),
	GVolumetricFogVoxelizationSlicesPerGSPass,
	TEXT("How many depth slices to render in a single voxelization pass (max geometry shader expansion).  Must recompile voxelization shaders to propagate changes."),
	ECVF_ReadOnly
	);

int32 GVolumetricFogVoxelizationShowOnlyPassIndex = -1;
FAutoConsoleVariableRef CVarVolumetricFogVoxelizationShowOnlyPassIndex(
	TEXT("r.VolumetricFog.VoxelizationShowOnlyPassIndex"),
	GVolumetricFogVoxelizationShowOnlyPassIndex,
	TEXT("When >= 0, indicates a single voxelization pass to render for debugging."),
	ECVF_RenderThreadSafe
	);

static FORCEINLINE int32 GetVoxelizationSlicesPerPass(EShaderPlatform Platform)
{
	return RHISupportsGeometryShaders(Platform) ? GVolumetricFogVoxelizationSlicesPerGSPass : 1;
}

class FVoxelizeVolumeVS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FVoxelizeVolumeVS,MeshMaterial);

protected:

	FVoxelizeVolumeVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:	FMeshMaterialShader(Initializer)
	{
		VoxelizationPassIndex.Bind(Initializer.ParameterMap, TEXT("VoxelizationPassIndex"));
		ViewToVolumeClip.Bind(Initializer.ParameterMap, TEXT("ViewToVolumeClip"));
		VolumetricFogParameters.Bind(Initializer.ParameterMap);
	}

	FVoxelizeVolumeVS()
	{
	}

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) 
			&& DoesPlatformSupportVolumetricFogVoxelization(Platform)
			&& Material->GetMaterialDomain() == MD_Volume;
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		FMeshMaterialShader::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
		if (RHISupportsGeometryShaders(Platform))
		{
			OutEnvironment.CompilerFlags.Add( CFLAG_VertexToGeometryShader );
		}
	}

public:
	
	void SetParameters(
		FRHICommandList& RHICmdList, 
		const FVertexFactory* VertexFactory,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FViewInfo& View, 
		const FVolumetricFogIntegrationParameterData& IntegrationData,
		const TUniformBufferRef<FViewUniformShaderParameters>& VoxelizeViewUniformBuffer,
		FVector2D Jitter)
	{
		FMeshMaterialShader::SetParameters(RHICmdList, GetVertexShader(), MaterialRenderProxy, *MaterialRenderProxy->GetMaterial(View.GetFeatureLevel()), View, VoxelizeViewUniformBuffer, ESceneRenderTargetsMode::SetTextures);

		if (!RHISupportsGeometryShaders(View.GetShaderPlatform()))
		{
			VolumetricFogParameters.Set(RHICmdList, GetVertexShader(), View, IntegrationData);

			FMatrix ProjectionMatrix = View.ViewMatrices.ComputeProjectionNoAAMatrix();

			ProjectionMatrix.M[2][0] += Jitter.X;
			ProjectionMatrix.M[2][1] += Jitter.Y;

			const FMatrix ViewToVolumeClipValue = ProjectionMatrix;
			SetShaderValue(RHICmdList, GetVertexShader(), ViewToVolumeClip, ViewToVolumeClipValue);
		}
	}

	void SetMesh(FRHICommandList& RHICmdList, const FVertexFactory* VertexFactory,const FSceneView& View,const FPrimitiveSceneProxy* Proxy,const FMeshBatchElement& BatchElement,const FDrawingPolicyRenderState& DrawRenderState,int32 VoxelizationPassIndexValue)
	{
		FMeshMaterialShader::SetMesh(RHICmdList, GetVertexShader(),VertexFactory,View,Proxy,BatchElement,DrawRenderState);
		if (!RHISupportsGeometryShaders(View.GetShaderPlatform()))
		{
			SetShaderValue(RHICmdList, GetVertexShader(), VoxelizationPassIndex, VoxelizationPassIndexValue);
		}
	}

	virtual bool Serialize(FArchive& Ar)
	{		
		bool bShaderHasOutdatedParameters = FMeshMaterialShader::Serialize(Ar);
		Ar << VoxelizationPassIndex;
		Ar << ViewToVolumeClip;
		Ar << VolumetricFogParameters;
		return bShaderHasOutdatedParameters;
	}

protected:

	FShaderParameter VoxelizationPassIndex;
	FShaderParameter ViewToVolumeClip;
	FVolumetricFogIntegrationParameters VolumetricFogParameters;
};

IMPLEMENT_MATERIAL_SHADER_TYPE(,FVoxelizeVolumeVS,TEXT("VolumetricFogVoxelization"),TEXT("VoxelizeVS"),SF_Vertex); 

class FVoxelizeVolumeGS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FVoxelizeVolumeGS,MeshMaterial);

protected:

	FVoxelizeVolumeGS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:	FMeshMaterialShader(Initializer)
	{
		VoxelizationPassIndex.Bind(Initializer.ParameterMap, TEXT("VoxelizationPassIndex"));
		ViewToVolumeClip.Bind(Initializer.ParameterMap, TEXT("ViewToVolumeClip"));
		VolumetricFogParameters.Bind(Initializer.ParameterMap);
	}

	FVoxelizeVolumeGS()
	{
	}

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) 
			&& RHISupportsGeometryShaders(Platform)
			&& DoesPlatformSupportVolumetricFogVoxelization(Platform)
			&& Material->GetMaterialDomain() == MD_Volume;
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		FMeshMaterialShader::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("MAX_SLICES_PER_VOXELIZATION_PASS"), GetVoxelizationSlicesPerPass(Platform));
	}

public:
	
	void SetParameters(
		FRHICommandList& RHICmdList, 
		const FVertexFactory* VertexFactory,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FViewInfo& View, 
		const FVolumetricFogIntegrationParameterData& IntegrationData,
		const TUniformBufferRef<FViewUniformShaderParameters>& VoxelizeViewUniformBuffer,
		FVector2D Jitter)
	{
		FMeshMaterialShader::SetParameters(RHICmdList, (FGeometryShaderRHIParamRef)GetGeometryShader(), MaterialRenderProxy, *MaterialRenderProxy->GetMaterial(View.GetFeatureLevel()), View, VoxelizeViewUniformBuffer, ESceneRenderTargetsMode::SetTextures);
		VolumetricFogParameters.Set(RHICmdList, GetGeometryShader(), View, IntegrationData);

		FMatrix ProjectionMatrix = View.ViewMatrices.ComputeProjectionNoAAMatrix();

		ProjectionMatrix.M[2][0] += Jitter.X;
		ProjectionMatrix.M[2][1] += Jitter.Y;

		const FMatrix ViewToVolumeClipValue = ProjectionMatrix;
		SetShaderValue(RHICmdList, GetGeometryShader(), ViewToVolumeClip, ViewToVolumeClipValue);
	}

	void SetMesh(FRHICommandList& RHICmdList, const FVertexFactory* VertexFactory,const FSceneView& View,const FPrimitiveSceneProxy* Proxy,const FMeshBatchElement& BatchElement,const FDrawingPolicyRenderState& DrawRenderState,int32 VoxelizationPassIndexValue)
	{
		FMeshMaterialShader::SetMesh(RHICmdList, (FGeometryShaderRHIParamRef)GetGeometryShader(),VertexFactory,View,Proxy,BatchElement,DrawRenderState);
		SetShaderValue(RHICmdList, GetGeometryShader(), VoxelizationPassIndex, VoxelizationPassIndexValue);
	}

	virtual bool Serialize(FArchive& Ar)
	{		
		bool bShaderHasOutdatedParameters = FMeshMaterialShader::Serialize(Ar);
		Ar << VoxelizationPassIndex;
		Ar << ViewToVolumeClip;
		Ar << VolumetricFogParameters;
		return bShaderHasOutdatedParameters;
	}

protected:

	FShaderParameter VoxelizationPassIndex;
	FShaderParameter ViewToVolumeClip;
	FVolumetricFogIntegrationParameters VolumetricFogParameters;
};

IMPLEMENT_MATERIAL_SHADER_TYPE(,FVoxelizeVolumeGS,TEXT("VolumetricFogVoxelization"),TEXT("VoxelizeGS"),SF_Geometry); 

class FVoxelizeVolumePS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(FVoxelizeVolumePS,MeshMaterial);

protected:

	FVoxelizeVolumePS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		:	FMeshMaterialShader(Initializer)
	{
		VolumetricFogParameters.Bind(Initializer.ParameterMap);
	}

	FVoxelizeVolumePS()
	{
	}

	static bool ShouldCache(EShaderPlatform Platform,const FMaterial* Material,const FVertexFactoryType* VertexFactoryType)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5) 
			&& DoesPlatformSupportVolumetricFogVoxelization(Platform)
			&& Material->GetMaterialDomain() == MD_Volume;
	}

public:
	
	void SetParameters(
		FRHICommandList& RHICmdList, 
		const FVertexFactory* VertexFactory,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FViewInfo& View, 
		const FVolumetricFogIntegrationParameterData& IntegrationData,
		const TUniformBufferRef<FViewUniformShaderParameters>& VoxelizeViewUniformBuffer)
	{
		FMeshMaterialShader::SetParameters(RHICmdList, GetPixelShader(), MaterialRenderProxy, *MaterialRenderProxy->GetMaterial(View.GetFeatureLevel()), View, VoxelizeViewUniformBuffer, ESceneRenderTargetsMode::SetTextures);
		VolumetricFogParameters.Set(RHICmdList, GetPixelShader(), View, IntegrationData);
	}

	void SetMesh(FRHICommandList& RHICmdList, const FVertexFactory* VertexFactory,const FSceneView& View,const FPrimitiveSceneProxy* Proxy,const FMeshBatchElement& BatchElement,const FDrawingPolicyRenderState& DrawRenderState)
	{
		FMeshMaterialShader::SetMesh(RHICmdList, GetPixelShader(),VertexFactory,View,Proxy,BatchElement,DrawRenderState);
	}

	virtual bool Serialize(FArchive& Ar)
	{		
		bool bShaderHasOutdatedParameters = FMeshMaterialShader::Serialize(Ar);
		Ar << VolumetricFogParameters;
		return bShaderHasOutdatedParameters;
	}

protected:

	FVolumetricFogIntegrationParameters VolumetricFogParameters;
};

IMPLEMENT_MATERIAL_SHADER_TYPE(,FVoxelizeVolumePS,TEXT("VolumetricFogVoxelization"),TEXT("VoxelizePS"),SF_Pixel); 

class FVoxelizeVolumeDrawingPolicy : public FMeshDrawingPolicy
{
public:
	/** context type */
	typedef FMeshDrawingPolicy::ElementDataType ElementDataType;

	FVoxelizeVolumeDrawingPolicy(
		const FVertexFactory* InVertexFactory,
		const FMaterialRenderProxy* InMaterialRenderProxy,
		const FMaterial& MaterialResouce,
		ERHIFeatureLevel::Type InFeatureLevel,
		const FMeshDrawingPolicyOverrideSettings& InOverrideSettings
		);

	// FMeshDrawingPolicy interface.

	FDrawingPolicyMatchResult Matches(const FVoxelizeVolumeDrawingPolicy& Other) const;

	void SetupPipelineState(FDrawingPolicyRenderState& DrawRenderState, const FSceneView& View) const
	{
		DrawRenderState.SetBlendState(TStaticBlendState<
			CW_RGBA, BO_Add, BF_One, BF_One, BO_Add, BF_One, BF_One,
			CW_RGBA, BO_Add, BF_One, BF_One, BO_Add, BF_One, BF_One>::GetRHI());
		DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());
		DrawRenderState.ModifyViewOverrideFlags() |= EDrawingPolicyOverrideFlags::TwoSided;
	}

	void SetSharedState(
		FRHICommandList& RHICmdList, 
		const FViewInfo& View, 
		const FVolumetricFogIntegrationParameterData& IntegrationData,
		const TUniformBufferRef<FViewUniformShaderParameters>& VoxelizeViewUniformBuffer, 
		FVector2D Jitter, 
		const ContextDataType PolicyContext, 
		FDrawingPolicyRenderState& DrawRenderState) const;
	
	FBoundShaderStateInput GetBoundShaderStateInput(ERHIFeatureLevel::Type InFeatureLevel);

	void SetMeshRenderState(
		FRHICommandList& RHICmdList, 
		const FSceneView& View,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const FMeshBatch& Mesh,
		int32 BatchElementIndex,
		int32 VoxelizationPassIndex,
		const FDrawingPolicyRenderState& DrawRenderState,
		const ElementDataType& ElementData,
		const ContextDataType PolicyContext
		) const;

private:
	FVoxelizeVolumeVS* VertexShader;
	FVoxelizeVolumeGS* GeometryShader;
	FVoxelizeVolumePS* PixelShader;
};

FVoxelizeVolumeDrawingPolicy::FVoxelizeVolumeDrawingPolicy(
	const FVertexFactory* InVertexFactory,
	const FMaterialRenderProxy* InMaterialRenderProxy,
	const FMaterial& InMaterialResource,
	ERHIFeatureLevel::Type InFeatureLevel,
	const FMeshDrawingPolicyOverrideSettings& InOverrideSettings
	)
:	FMeshDrawingPolicy(InVertexFactory,InMaterialRenderProxy,InMaterialResource,InOverrideSettings)
,	GeometryShader(nullptr)
{
	VertexShader = InMaterialResource.GetShader<FVoxelizeVolumeVS>(InVertexFactory->GetType());
	if (RHISupportsGeometryShaders(GShaderPlatformForFeatureLevel[InFeatureLevel]))
	{
		GeometryShader = InMaterialResource.GetShader<FVoxelizeVolumeGS>(InVertexFactory->GetType());
	}
	PixelShader = InMaterialResource.GetShader<FVoxelizeVolumePS>(InVertexFactory->GetType());
}

FDrawingPolicyMatchResult FVoxelizeVolumeDrawingPolicy::Matches(
	const FVoxelizeVolumeDrawingPolicy& Other
	) const
{
	DRAWING_POLICY_MATCH_BEGIN
		DRAWING_POLICY_MATCH(FMeshDrawingPolicy::Matches(Other)) &&
		DRAWING_POLICY_MATCH(VertexShader == Other.VertexShader) &&
		DRAWING_POLICY_MATCH(GeometryShader == Other.GeometryShader) &&
		DRAWING_POLICY_MATCH(PixelShader == Other.PixelShader);
	DRAWING_POLICY_MATCH_END
}

void FVoxelizeVolumeDrawingPolicy::SetSharedState(
	FRHICommandList& RHICmdList, 
	const FViewInfo& View,
	const FVolumetricFogIntegrationParameterData& IntegrationData,
	const TUniformBufferRef<FViewUniformShaderParameters>& VoxelizeViewUniformBuffer,
	FVector2D Jitter,
	const ContextDataType PolicyContext,
	FDrawingPolicyRenderState& DrawRenderState
	) const
{
	// Set shared mesh resources
	FMeshDrawingPolicy::SetSharedState(RHICmdList, DrawRenderState, &View, PolicyContext);

	VertexShader->SetParameters(RHICmdList, VertexFactory, MaterialRenderProxy, View, IntegrationData, VoxelizeViewUniformBuffer, Jitter);
	if (GeometryShader)
	{
		GeometryShader->SetParameters(RHICmdList, VertexFactory, MaterialRenderProxy, View, IntegrationData, VoxelizeViewUniformBuffer, Jitter);
	}
	PixelShader->SetParameters(RHICmdList, VertexFactory, MaterialRenderProxy, View, IntegrationData, VoxelizeViewUniformBuffer);
}

FBoundShaderStateInput FVoxelizeVolumeDrawingPolicy::GetBoundShaderStateInput(ERHIFeatureLevel::Type InFeatureLevel)
{
	return FBoundShaderStateInput(
		FMeshDrawingPolicy::GetVertexDeclaration(), 
		VertexShader->GetVertexShader(),
		NULL,
		NULL,
		PixelShader->GetPixelShader(), 
		GeometryShader ? GeometryShader->GetGeometryShader() : NULL);
}

void FVoxelizeVolumeDrawingPolicy::SetMeshRenderState(
	FRHICommandList& RHICmdList, 
	const FSceneView& View,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	const FMeshBatch& Mesh,
	int32 BatchElementIndex,
	int32 VoxelizationPassIndex,
	const FDrawingPolicyRenderState& DrawRenderState,
	const ElementDataType& ElementData,
	const ContextDataType PolicyContext
	) const
{
	const FMeshBatchElement& BatchElement = Mesh.Elements[BatchElementIndex];

	VertexShader->SetMesh(RHICmdList, VertexFactory, View, PrimitiveSceneProxy, BatchElement, DrawRenderState, VoxelizationPassIndex);
	if (GeometryShader)
	{
		GeometryShader->SetMesh(RHICmdList, VertexFactory, View, PrimitiveSceneProxy, BatchElement, DrawRenderState, VoxelizationPassIndex);
	}
	PixelShader->SetMesh(RHICmdList, VertexFactory, View, PrimitiveSceneProxy, BatchElement, DrawRenderState);
}

void VoxelizeVolumePrimitive(
	FRHICommandListImmediate& RHICmdList,
	const FViewInfo& View,
	const FVolumetricFogIntegrationParameterData& IntegrationData,
	const TUniformBufferRef<FViewUniformShaderParameters>& VoxelizeViewUniformBuffer,
	FVector2D Jitter,
	FIntVector VolumetricFogGridSize,
	FVector GridZParams,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	const FMeshBatch& Mesh)
{
	const FMaterial& Material = *Mesh.MaterialRenderProxy->GetMaterial(View.GetFeatureLevel());

	if (Material.GetMaterialDomain() == MD_Volume)
	{
		FVoxelizeVolumeDrawingPolicy DrawingPolicy(
			Mesh.VertexFactory,
			Mesh.MaterialRenderProxy,
			Material,
			View.GetFeatureLevel(),
			ComputeMeshOverrideSettings(Mesh));

		FDrawingPolicyRenderState DrawRenderState(View);
		DrawingPolicy.SetupPipelineState(DrawRenderState, View);
		CommitGraphicsPipelineState(RHICmdList, DrawingPolicy, DrawRenderState, DrawingPolicy.GetBoundShaderStateInput(View.GetFeatureLevel()));
		DrawingPolicy.SetSharedState(RHICmdList, View, IntegrationData, VoxelizeViewUniformBuffer, Jitter, FVoxelizeVolumeDrawingPolicy::ContextDataType(), DrawRenderState);

		FBoxSphereBounds Bounds = PrimitiveSceneProxy->GetBounds();
		//@todo - compute NumSlices based on the largest particle size.  Bounds is overly conservative in most cases.
		float BoundsCenterDepth = View.ViewMatrices.GetViewMatrix().TransformPosition(Bounds.Origin).Z;
		int32 NearSlice = ComputeZSliceFromDepth(BoundsCenterDepth - Bounds.SphereRadius, GridZParams);
		int32 FarSlice = ComputeZSliceFromDepth(BoundsCenterDepth + Bounds.SphereRadius, GridZParams);

		NearSlice = FMath::Clamp(NearSlice, 0, VolumetricFogGridSize.Z - 1);
		FarSlice = FMath::Clamp(FarSlice, 0, VolumetricFogGridSize.Z - 1);

		const int32 NumSlices = FarSlice - NearSlice + 1;
		const int32 NumVoxelizationPasses = FMath::DivideAndRoundUp(NumSlices, GetVoxelizationSlicesPerPass(View.GetShaderPlatform()));

		TDrawEvent<FRHICommandList> MeshEvent;
		BeginMeshDrawEvent(RHICmdList, PrimitiveSceneProxy, Mesh, MeshEvent);

		for (int32 VoxelizationPassIndex = 0; VoxelizationPassIndex < NumVoxelizationPasses; VoxelizationPassIndex++)
		{
			if (GVolumetricFogVoxelizationShowOnlyPassIndex < 0 || GVolumetricFogVoxelizationShowOnlyPassIndex == VoxelizationPassIndex)
			{
				for (int32 BatchElementIndex = 0; BatchElementIndex < Mesh.Elements.Num(); BatchElementIndex++)
				{
					DrawingPolicy.SetMeshRenderState(RHICmdList, View, PrimitiveSceneProxy, Mesh, BatchElementIndex, VoxelizationPassIndex, DrawRenderState, FVoxelizeVolumeDrawingPolicy::ElementDataType(), FVoxelizeVolumeDrawingPolicy::ContextDataType());
					DrawingPolicy.DrawMesh(RHICmdList, Mesh, BatchElementIndex);
				}
			}
		}
	}
}

void FDeferredShadingSceneRenderer::VoxelizeFogVolumePrimitives(
	FRHICommandListImmediate& RHICmdList, 
	const FViewInfo& View,
	const FVolumetricFogIntegrationParameterData& IntegrationData,
	FIntVector VolumetricFogGridSize,
	FVector GridZParams,
	float VolumetricFogDistance)
{
	if (View.VolumetricPrimSet.NumPrims() > 0
		&& DoesPlatformSupportVolumetricFogVoxelization(View.GetShaderPlatform()))
	{
		SCOPED_DRAW_EVENT(RHICmdList, VoxelizeVolumePrimitives);

		FViewUniformShaderParameters VoxelizeParameters = *View.CachedViewUniformShaderParameters;

		// Update the parts of VoxelizeParameters which are dependent on the buffer size and view rect
		View.SetupViewRectUniformBufferParameters(
			VoxelizeParameters,
			FIntPoint(VolumetricFogGridSize.X, VolumetricFogGridSize.Y),
			FIntRect(0, 0, VolumetricFogGridSize.X, VolumetricFogGridSize.Y),
			View.ViewMatrices,
			View.PrevViewMatrices
		);

		TUniformBufferRef<FViewUniformShaderParameters> VoxelizeViewUniformBuffer = TUniformBufferRef<FViewUniformShaderParameters>::CreateUniformBufferImmediate(VoxelizeParameters, UniformBuffer_SingleFrame);

		FVector2D Jitter(IntegrationData.FrameJitterOffsetValues[0].X / VolumetricFogGridSize.X, IntegrationData.FrameJitterOffsetValues[0].Y / VolumetricFogGridSize.Y);

		FTextureRHIParamRef RenderTargets[2] =
		{
			IntegrationData.VBufferARenderTarget->GetRenderTargetItem().TargetableTexture,
			IntegrationData.VBufferBRenderTarget->GetRenderTargetItem().TargetableTexture
		};

		SetRenderTargets(RHICmdList, ARRAY_COUNT(RenderTargets), RenderTargets, FTextureRHIParamRef(), 0, NULL);

		for (int32 PrimIdx = 0; PrimIdx < View.VolumetricPrimSet.NumPrims(); PrimIdx++)
		{
			const FPrimitiveSceneProxy* PrimitiveSceneProxy = View.VolumetricPrimSet.GetPrim(PrimIdx);
			const FPrimitiveSceneInfo* PrimitiveSceneInfo = PrimitiveSceneProxy->GetPrimitiveSceneInfo();

			if (View.PrimitiveVisibilityMap[PrimitiveSceneInfo->GetIndex()])
			{
				const FPrimitiveViewRelevance& ViewRelevance = View.PrimitiveViewRelevanceMap[PrimitiveSceneInfo->GetIndex()];
				FBoxSphereBounds Bounds = PrimitiveSceneProxy->GetBounds();

				if ((View.ViewMatrices.GetViewOrigin() - Bounds.Origin).SizeSquared() < (VolumetricFogDistance + Bounds.SphereRadius) * (VolumetricFogDistance + Bounds.SphereRadius))
				{
					// Range in View.DynamicMeshElements[] corresponding to this PrimitiveSceneInfo
					FInt32Range Range = View.GetDynamicMeshElementRange(PrimitiveSceneInfo->GetIndex());

					for (int32 MeshBatchIndex = Range.GetLowerBoundValue(); MeshBatchIndex < Range.GetUpperBoundValue(); MeshBatchIndex++)
					{
						const FMeshBatchAndRelevance& MeshBatchAndRelevance = View.DynamicMeshElements[MeshBatchIndex];

						checkSlow(MeshBatchAndRelevance.PrimitiveSceneProxy == PrimitiveSceneInfo->Proxy);

						const FMeshBatch& MeshBatch = *MeshBatchAndRelevance.Mesh;
						VoxelizeVolumePrimitive(RHICmdList, View, IntegrationData, VoxelizeViewUniformBuffer, Jitter, VolumetricFogGridSize, GridZParams, PrimitiveSceneProxy, MeshBatch);
					}
				}

				if (ViewRelevance.bStaticRelevance)
				{
					for (int32 StaticMeshIdx = 0; StaticMeshIdx < PrimitiveSceneInfo->StaticMeshes.Num(); StaticMeshIdx++)
					{
						const FStaticMesh& StaticMesh = PrimitiveSceneInfo->StaticMeshes[StaticMeshIdx];

						if (View.StaticMeshVisibilityMap[StaticMesh.Id])
						{
							VoxelizeVolumePrimitive(RHICmdList, View, IntegrationData, VoxelizeViewUniformBuffer, Jitter, VolumetricFogGridSize, GridZParams, PrimitiveSceneProxy, StaticMesh);
						}
					}
				}
			}
		}
	}
}