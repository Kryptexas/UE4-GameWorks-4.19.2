// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessMotionBlur.cpp: Post process MotionBlur implementation.
=============================================================================*/

#include "PostProcess/PostProcessMotionBlur.h"
#include "StaticBoundShaderState.h"
#include "CanvasTypes.h"
#include "RenderTargetTemp.h"
#include "SceneUtils.h"
#include "PostProcess/SceneRenderTargets.h"
#include "SceneRenderTargetParameters.h"
#include "ScenePrivate.h"
#include "PostProcess/SceneFilterRendering.h"
#include "CompositionLighting/PostProcessAmbientOcclusion.h"
#include "PostProcess/PostProcessing.h"
#include "DeferredShadingRenderer.h"
#include "ClearQuad.h"
#include "PipelineStateCache.h"
#include "SpriteIndexBuffer.h"

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
static TAutoConsoleVariable<int32> CVarMotionBlurFiltering(
	TEXT("r.MotionBlurFiltering"),
	0,
	TEXT("Useful developer variable\n")
	TEXT("0: off (default, expected by the shader for better quality)\n")
	TEXT("1: on"),
	ECVF_Cheat | ECVF_RenderThreadSafe);
#endif

static TAutoConsoleVariable<float> CVarMotionBlur2ndScale(
	TEXT("r.MotionBlur2ndScale"),
	1.0f,
	TEXT(""),
	ECVF_Cheat | ECVF_RenderThreadSafe);

const int32 kMotionBlurTileSize = 16;

const int32 GMotionBlurComputeTileSizeX = 8;
const int32 GMotionBlurComputeTileSizeY = 8;

FIntPoint GetNumTiles16x16( FIntPoint PixelExtent )
{
	uint32 TilesX = FMath::DivideAndRoundUp(PixelExtent.X, kMotionBlurTileSize);
	uint32 TilesY = FMath::DivideAndRoundUp(PixelExtent.Y, kMotionBlurTileSize);
	return FIntPoint( TilesX, TilesY );
}

FVector4 GetMotionBlurParameters( const FRenderingCompositePassContext& Context, float Scale = 1.0f )
{
	const float TileSize = kMotionBlurTileSize;

	const float SizeX = Context.SceneColorViewRect.Width();
	const float SizeY = Context.SceneColorViewRect.Height();
	const float AspectRatio = SizeY / SizeX;

	const FSceneViewState* ViewState = (FSceneViewState*) Context.ViewState;
	float MotionBlurTimeScale = ViewState ? ViewState->MotionBlurTimeScale : 1.0f;
	float MotionBlurScale = 0.5f * MotionBlurTimeScale * Context.View.FinalPostProcessSettings.MotionBlurAmount;

	// 0:no 1:full screen width, percent conversion
	float MaxVelocity = Context.View.FinalPostProcessSettings.MotionBlurMax / 100.0f;

	// Scale by 0.5 due to blur samples going both ways
	float PixelScale = Scale * SizeX * 0.5f;

	FVector4 MotionBlurParameters(
		AspectRatio,
		PixelScale * MotionBlurScale,			// Scale for pixels
		PixelScale * MotionBlurScale / TileSize,// Scale for tiles
		FMath::Abs( PixelScale ) * MaxVelocity	// Max velocity pixels
	);

	return MotionBlurParameters;
}

FVector4 GetSceneColorBufferUVToViewBufferUV(const FRenderingCompositePassContext& Context)
{
	const FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(Context.RHICmdList);

	FVector2D SceneColorOffset = FVector2D(Context.SceneColorViewRect.Min) / FVector2D(Context.ReferenceBufferSize);
	FVector2D SceneColorExtent = FVector2D(Context.SceneColorViewRect.Size()) / FVector2D(Context.ReferenceBufferSize);
	FVector2D ViewOffset = FVector2D(Context.View.ViewRect.Min) / FVector2D(SceneContext.GetBufferSizeXY());
	FVector2D ViewExtent = FVector2D(Context.View.ViewRect.Size()) / FVector2D(SceneContext.GetBufferSizeXY());

	FVector4 SceneColorBufferUVToViewBufferUVValue;

	SceneColorBufferUVToViewBufferUVValue.X = ViewExtent.X / SceneColorExtent.X;
	SceneColorBufferUVToViewBufferUVValue.Y = ViewExtent.Y / SceneColorExtent.Y;
	SceneColorBufferUVToViewBufferUVValue.Z = ViewOffset.X - SceneColorBufferUVToViewBufferUVValue.X * SceneColorOffset.X;
	SceneColorBufferUVToViewBufferUVValue.W = ViewOffset.Y - SceneColorBufferUVToViewBufferUVValue.Y * SceneColorOffset.Y;

	return SceneColorBufferUVToViewBufferUVValue;
}

class FPostProcessVelocityFlattenCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessVelocityFlattenCS, Global);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
	}

	FPostProcessVelocityFlattenCS() {}

public:
	FShaderParameter		OutVelocityFlat;		// UAV
	FShaderParameter		OutMaxTileVelocity;		// UAV

	FPostProcessVelocityFlattenCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		MotionBlurParameters.Bind(Initializer.ParameterMap, TEXT("MotionBlurParameters"));
		OutVelocityFlat.Bind(Initializer.ParameterMap, TEXT("OutVelocityFlat"));
		OutMaxTileVelocity.Bind(Initializer.ParameterMap, TEXT("OutMaxTileVelocity"));
	}

	void SetCS( FRHICommandList& RHICmdList, const FRenderingCompositePassContext& Context, const FSceneView& View )
	{
		const FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();

		FGlobalShader::SetParameters<FViewUniformShaderParameters>(RHICmdList, ShaderRHI, Context.View.ViewUniformBuffer);
		PostprocessParameter.SetCS(ShaderRHI, Context, Context.RHICmdList, TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI());

		SetShaderValue(Context.RHICmdList, ShaderRHI, MotionBlurParameters, GetMotionBlurParameters( Context ) );
	}
	
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter;
		Ar << MotionBlurParameters;
		Ar << OutVelocityFlat;
		Ar << OutMaxTileVelocity;
		return bShaderHasOutdatedParameters;
	}

private:
	FPostProcessPassParameters	PostprocessParameter;
	FShaderParameter			MotionBlurParameters;
};

IMPLEMENT_SHADER_TYPE(,FPostProcessVelocityFlattenCS,TEXT("/Engine/Private/PostProcessVelocityFlatten.usf"),TEXT("VelocityFlattenMain"),SF_Compute);


FRCPassPostProcessVelocityFlatten::FRCPassPostProcessVelocityFlatten()
{}

void FRCPassPostProcessVelocityFlatten::Process(FRenderingCompositePassContext& Context)
{
	const FPooledRenderTargetDesc* InputDesc = GetInputDesc(ePId_Input0);

	const FViewInfo& View = Context.View;

	SCOPED_DRAW_EVENTF(Context.RHICmdList, MotionBlur, TEXT("VelocityFlattenCS %dx%d"),
		View.ViewRect.Width(), View.ViewRect.Height());

	const FSceneRenderTargetItem& DestRenderTarget0 = PassOutputs[0].RequestSurface(Context);
	const FSceneRenderTargetItem& DestRenderTarget1 = PassOutputs[1].RequestSurface(Context);

	TShaderMapRef< FPostProcessVelocityFlattenCS > ComputeShader( Context.GetShaderMap() );

	SetRenderTarget(Context.RHICmdList, FTextureRHIRef(), FTextureRHIRef());

	Context.SetViewportAndCallRHI( View.ViewRect );
	Context.RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());

	// set destination
	Context.RHICmdList.SetUAVParameter(ComputeShader->GetComputeShader(), ComputeShader->OutVelocityFlat.GetBaseIndex(), DestRenderTarget0.UAV);
	Context.RHICmdList.SetUAVParameter(ComputeShader->GetComputeShader(), ComputeShader->OutMaxTileVelocity.GetBaseIndex(), DestRenderTarget1.UAV);

	ComputeShader->SetCS(Context.RHICmdList, Context, View);

	FIntPoint ThreadGroupCountValue = GetNumTiles16x16(View.ViewRect.Size());
	DispatchComputeShader(Context.RHICmdList, *ComputeShader, ThreadGroupCountValue.X, ThreadGroupCountValue.Y, 1);

	//	void FD3D11DynamicRHI::RHIGraphicsWaitOnAsyncComputeJob( uint32 FenceIndex )
	Context.RHICmdList.FlushComputeShaderCache();

	// un-set destination
	Context.RHICmdList.SetUAVParameter(ComputeShader->GetComputeShader(), ComputeShader->OutVelocityFlat.GetBaseIndex(), NULL);
	Context.RHICmdList.SetUAVParameter(ComputeShader->GetComputeShader(), ComputeShader->OutMaxTileVelocity.GetBaseIndex(), NULL);

	Context.RHICmdList.CopyToResolveTarget(DestRenderTarget0.TargetableTexture, DestRenderTarget0.ShaderResourceTexture, false, FResolveParams());
	Context.RHICmdList.CopyToResolveTarget(DestRenderTarget1.TargetableTexture, DestRenderTarget1.ShaderResourceTexture, false, FResolveParams());
}

FPooledRenderTargetDesc FRCPassPostProcessVelocityFlatten::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	if( InPassOutputId == ePId_Output0 )
	{
		// Flattened velocity
		FPooledRenderTargetDesc Ret = GetInput(ePId_Input0)->GetOutput()->RenderTargetDesc;
		Ret.Reset();
		Ret.ClearValue = FClearValueBinding::None;
		Ret.Format = PF_FloatR11G11B10;
		Ret.TargetableFlags |= TexCreate_UAV;
		Ret.TargetableFlags |= TexCreate_RenderTargetable;
		Ret.Flags |= GFastVRamConfig.VelocityFlat;
		Ret.DebugName = TEXT("VelocityFlat");

		return Ret;
	}
	else
	{
		// Max tile velocity
		FPooledRenderTargetDesc UnmodifiedRet = GetInput(ePId_Input0)->GetOutput()->RenderTargetDesc;
		UnmodifiedRet.Reset();

		FIntPoint PixelExtent = UnmodifiedRet.Extent;
		FIntPoint TileCount = GetNumTiles16x16(PixelExtent);

		FPooledRenderTargetDesc Ret(FPooledRenderTargetDesc::Create2DDesc(TileCount, PF_FloatRGBA, FClearValueBinding::None, TexCreate_None, TexCreate_RenderTargetable | TexCreate_UAV, false));
		Ret.Flags |= GFastVRamConfig.VelocityMax;
		Ret.DebugName = TEXT("MaxVelocity");

		return Ret;
	}
}


TGlobalResource< FSpriteIndexBuffer<8> > GScatterQuadIndexBuffer;

class FPostProcessVelocityScatterVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessVelocityScatterVS,Global);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	/** Default constructor. */
	FPostProcessVelocityScatterVS() {}
	
public:
	FPostProcessPassParameters PostprocessParameter;
	FShaderParameter DrawMax;
	FShaderParameter MotionBlurParameters;

	/** Initialization constructor. */
	FPostProcessVelocityScatterVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		DrawMax.Bind(Initializer.ParameterMap, TEXT("bDrawMax"));
		MotionBlurParameters.Bind( Initializer.ParameterMap, TEXT("MotionBlurParameters") );
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter << DrawMax << MotionBlurParameters;
		return bShaderHasOutdatedParameters;
	}

	/** to have a similar interface as all other shaders */
	void SetParameters(const FRenderingCompositePassContext& Context, int32 bDrawMax)
	{
		const FVertexShaderRHIParamRef ShaderRHI = GetVertexShader();

		FGlobalShader::SetParameters<FViewUniformShaderParameters>(Context.RHICmdList, ShaderRHI, Context.View.ViewUniformBuffer);
		PostprocessParameter.SetVS(ShaderRHI, Context, TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI());

		SetShaderValue(Context.RHICmdList, ShaderRHI, DrawMax, bDrawMax);
		SetShaderValue(Context.RHICmdList, ShaderRHI, MotionBlurParameters, GetMotionBlurParameters( Context ) );
	}

	static const TCHAR* GetSourceFilename()
	{
		return TEXT("/Engine/Private/PostProcessMotionBlur.usf");
	}

	static const TCHAR* GetFunctionName()
	{
		return TEXT("VelocityScatterVS");
	}
};

class FPostProcessVelocityScatterPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessVelocityScatterPS, Global);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	/** Default constructor. */
	FPostProcessVelocityScatterPS() {}

public:
	/** Initialization constructor. */
	FPostProcessVelocityScatterPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{}

	void SetParameters(const FRenderingCompositePassContext& Context)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters<FViewUniformShaderParameters>(Context.RHICmdList, ShaderRHI, Context.View.ViewUniformBuffer);
	}
};

IMPLEMENT_SHADER_TYPE(,FPostProcessVelocityScatterVS,TEXT("/Engine/Private/PostProcessMotionBlur.usf"),TEXT("VelocityScatterVS"),SF_Vertex);
IMPLEMENT_SHADER_TYPE(,FPostProcessVelocityScatterPS,TEXT("/Engine/Private/PostProcessMotionBlur.usf"),TEXT("VelocityScatterPS"),SF_Pixel);


void FRCPassPostProcessVelocityScatter::Process(FRenderingCompositePassContext& Context)
{
	const FPooledRenderTargetDesc* InputDesc = GetInputDesc(ePId_Input0);
	
	const FViewInfo& View = Context.View;

	SCOPED_DRAW_EVENTF(Context.RHICmdList, MotionBlur, TEXT("VelocityScatter %dx%d"),
		View.ViewRect.Width(), View.ViewRect.Height());

	FIntPoint SrcSize = InputDesc->Extent;
	FIntPoint DestSize = PassOutputs[0].RenderTargetDesc.Extent;

	FIntPoint TileCount = GetNumTiles16x16( View.ViewRect.Size() );

	const FSceneRenderTargetItem& DestRenderTarget = PassOutputs[0].RequestSurface(Context);

	TRefCountPtr<IPooledRenderTarget> DepthTarget;
	FPooledRenderTargetDesc Desc( FPooledRenderTargetDesc::Create2DDesc( DestSize, PF_ShadowDepth, FClearValueBinding::DepthOne, TexCreate_None, TexCreate_DepthStencilTargetable, false ) );
	GRenderTargetPool.FindFreeElement(Context.RHICmdList, Desc, DepthTarget, TEXT("VelocityScatterDepth") );

	// Set the view family's render target/viewport.
	FRHIRenderTargetView ColorView(DestRenderTarget.TargetableTexture, 0, -1, ERenderTargetLoadAction::ELoad, ERenderTargetStoreAction::EStore);
	FRHIDepthRenderTargetView DepthView(DepthTarget->GetRenderTargetItem().TargetableTexture, ERenderTargetLoadAction::EClear, ERenderTargetStoreAction::EStore, ERenderTargetLoadAction::ELoad, ERenderTargetStoreAction::EStore);
	FRHISetRenderTargetsInfo RTInfo(1, &ColorView, DepthView);

	// clear depth
	// Max >= Min so no need to clear on second pass
	Context.RHICmdList.SetRenderTargetsAndClear(RTInfo);
	Context.SetViewportAndCallRHI(0, 0, 0.0f, TileCount.X, TileCount.Y, 1.0f);
	
	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	Context.RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);

	// Min,Max
	for( int i = 0; i < 2; i++ )
	{
		if( i == 0 )
		{
			// min
			GraphicsPSOInit.BlendState = TStaticBlendStateWriteMask< CW_RGBA >::GetRHI();
			GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
			GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState< true, CF_Less >::GetRHI();
		}
		else
		{
			// max
			GraphicsPSOInit.BlendState = TStaticBlendStateWriteMask< CW_BA >::GetRHI();
			GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
			GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState< true, CF_Greater >::GetRHI();
		}

		TShaderMapRef< FPostProcessVelocityScatterVS > VertexShader(Context.GetShaderMap());
		TShaderMapRef< FPostProcessVelocityScatterPS > PixelShader(Context.GetShaderMap());

		GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GEmptyVertexDeclaration.VertexDeclarationRHI;
		GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
		GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
		GraphicsPSOInit.PrimitiveType = PT_TriangleList;

		SetGraphicsPipelineState(Context.RHICmdList, GraphicsPSOInit);

		VertexShader->SetParameters( Context, i );
		PixelShader->SetParameters( Context );

		// needs to be the same on shader side (faster on NVIDIA and AMD)
		int32 QuadsPerInstance = 8;

		Context.RHICmdList.SetStreamSource(0, NULL, 0);
		Context.RHICmdList.DrawIndexedPrimitive(GScatterQuadIndexBuffer.IndexBufferRHI, PT_TriangleList, 0, 0, 32, 0, 2 * QuadsPerInstance, FMath::DivideAndRoundUp(TileCount.X * TileCount.Y, QuadsPerInstance));
	}

	Context.RHICmdList.CopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
}

FPooledRenderTargetDesc FRCPassPostProcessVelocityScatter::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret = GetInput(ePId_Input0)->GetOutput()->RenderTargetDesc;
	Ret.Reset();

	Ret.DebugName = TEXT("ScatteredMaxVelocity");

	return Ret;
}


class FPostProcessVelocityGatherCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessVelocityGatherCS, Global);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	/** Default constructor. */
	FPostProcessVelocityGatherCS() {}

public:
	FShaderParameter		OutScatteredMaxVelocity;

	/** Initialization constructor. */
	FPostProcessVelocityGatherCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		MotionBlurParameters.Bind(Initializer.ParameterMap, TEXT("MotionBlurParameters"));
		OutScatteredMaxVelocity.Bind(Initializer.ParameterMap, TEXT("OutScatteredMaxVelocity"));
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter;
		Ar << MotionBlurParameters;
		Ar << OutScatteredMaxVelocity;
		return bShaderHasOutdatedParameters;
	}

	void SetParameters(const FRenderingCompositePassContext& Context)
	{
		const FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();

		FGlobalShader::SetParameters<FViewUniformShaderParameters>(Context.RHICmdList, ShaderRHI, Context.View.ViewUniformBuffer);

		PostprocessParameter.SetCS(ShaderRHI, Context, Context.RHICmdList, TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI());
		SetShaderValue(Context.RHICmdList, ShaderRHI, MotionBlurParameters, GetMotionBlurParameters( Context ) );
	}
	
private:
	FPostProcessPassParameters	PostprocessParameter;
	FShaderParameter			MotionBlurParameters;
};

IMPLEMENT_SHADER_TYPE(,FPostProcessVelocityGatherCS,TEXT("/Engine/Private/PostProcessVelocityFlatten.usf"),TEXT("VelocityGatherCS"),SF_Compute);

void FRCPassPostProcessVelocityGather::Process(FRenderingCompositePassContext& Context)
{
	const FPooledRenderTargetDesc* InputDesc = GetInputDesc(ePId_Input0);

	const FViewInfo& View = Context.View;

	SCOPED_DRAW_EVENTF(Context.RHICmdList, MotionBlur, TEXT("VelocityGatherCS %dx%d"),
		View.ViewRect.Width(), View.ViewRect.Height());

	FIntPoint TileCount = GetNumTiles16x16( View.ViewRect.Size() );
	
	const FSceneRenderTargetItem& DestRenderTarget = PassOutputs[0].RequestSurface(Context);

	SetRenderTarget(Context.RHICmdList, FTextureRHIRef(), FTextureRHIRef());
	Context.SetViewportAndCallRHI( 0, 0, 0.0f, TileCount.X, TileCount.Y, 1.0f );
	
	TShaderMapRef< FPostProcessVelocityGatherCS > ComputeShader( Context.GetShaderMap() );
	Context.RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());

	// set destination
	Context.RHICmdList.SetUAVParameter( ComputeShader->GetComputeShader(), ComputeShader->OutScatteredMaxVelocity.GetBaseIndex(), DestRenderTarget.UAV );

	ComputeShader->SetParameters( Context );

	FIntPoint GroupCount = GetNumTiles16x16( TileCount );
	DispatchComputeShader(Context.RHICmdList, *ComputeShader, GroupCount.X, GroupCount.Y, 1);

	// un-set destination
	Context.RHICmdList.SetUAVParameter( ComputeShader->GetComputeShader(), ComputeShader->OutScatteredMaxVelocity.GetBaseIndex(), NULL );

	Context.RHICmdList.CopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
}

FPooledRenderTargetDesc FRCPassPostProcessVelocityGather::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret = GetInput(ePId_Input0)->GetOutput()->RenderTargetDesc;
	Ret.Reset();

	Ret.TargetableFlags |= TexCreate_UAV;
	Ret.TargetableFlags |= TexCreate_RenderTargetable;
	Ret.Flags |= GFastVRamConfig.MotionBlur;
	Ret.DebugName = TEXT("ScatteredMaxVelocity");

	return Ret;
}


/**
 * @param Quality 0: visualize, 1:low, 2:medium, 3:high, 4:very high
 * @param InnerLoopConfig 0: fastest, 1: clamp uv, 2: clamp uv and handle different viewport res between scene color and depth.
 */
template< uint32 Quality, uint32 InnerLoopConfig >
class FPostProcessMotionBlurPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessMotionBlurPS, Global);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters,OutEnvironment);
		OutEnvironment.SetDefine(TEXT("MOTION_BLUR_QUALITY"), Quality);
		OutEnvironment.SetDefine(TEXT("MOTION_BLUR_INNER_LOOP_CONFIG"), InnerLoopConfig);
	}

	/** Default constructor. */
	FPostProcessMotionBlurPS() {}

public:
	FPostProcessPassParameters PostprocessParameter;
	FDeferredPixelShaderParameters DeferredParameters;
	FShaderParameter MotionBlurParameters;
	FShaderParameter SceneColorBufferUVToViewBufferUV;
	FShaderParameter TileUVMax;

	/** Initialization constructor. */
	FPostProcessMotionBlurPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		DeferredParameters.Bind(Initializer.ParameterMap);
		MotionBlurParameters.Bind(Initializer.ParameterMap, TEXT("MotionBlurParameters"));
		SceneColorBufferUVToViewBufferUV.Bind(Initializer.ParameterMap, TEXT("SceneColorBufferUVToViewBufferUV"));
		TileUVMax.Bind(Initializer.ParameterMap, TEXT("TileUVMax"));
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter << DeferredParameters << MotionBlurParameters << SceneColorBufferUVToViewBufferUV << TileUVMax;
		return bShaderHasOutdatedParameters;
	}

	void SetParameters(const FRenderingCompositePassContext& Context, float Scale)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters<FViewUniformShaderParameters>(Context.RHICmdList, ShaderRHI, Context.View.ViewUniformBuffer);

		DeferredParameters.Set(Context.RHICmdList, ShaderRHI, Context.View, MD_PostProcess);

		{
			bool bFiltered = false;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			bFiltered = CVarMotionBlurFiltering.GetValueOnRenderThread() != 0;
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

			if(bFiltered)
			{
				//PostprocessParameter.SetPS(ShaderRHI, Context, TStaticSamplerState<SF_Bilinear,AM_Border,AM_Border,AM_Clamp>::GetRHI());

				FSamplerStateRHIParamRef Filters[] =
				{
					TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
					TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
					TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
					TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
				};

				PostprocessParameter.SetPS(Context.RHICmdList, ShaderRHI, Context, 0, eFC_0000, Filters);
			}
			else
			{
				PostprocessParameter.SetPS(Context.RHICmdList, ShaderRHI, Context, TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI());
			}
		}

		SetShaderValue(Context.RHICmdList, ShaderRHI, MotionBlurParameters, GetMotionBlurParameters( Context, Scale ) );
		SetShaderValue(Context.RHICmdList, ShaderRHI, SceneColorBufferUVToViewBufferUV, GetSceneColorBufferUVToViewBufferUV(Context));

		if (TileUVMax.IsBound())
		{
			FIntPoint VelocityBufferSize = FSceneRenderTargets::Get(Context.RHICmdList).GetBufferSizeXY();

			FIntPoint TileBufferSize = GetNumTiles16x16(VelocityBufferSize);
			FIntPoint TileCount = GetNumTiles16x16(Context.View.ViewRect.Size());
			FVector2D TileUVMaxValue(
				(TileCount.X - 0.5f) / float(TileBufferSize.X),
				(TileCount.Y - 0.5f) / float(TileBufferSize.Y));

			SetShaderValue(Context.RHICmdList, ShaderRHI, TileUVMax, TileUVMaxValue);
		}
	}

	static const TCHAR* GetSourceFilename()
	{
		return TEXT("/Engine/Private/PostProcessMotionBlur.usf");
	}

	static const TCHAR* GetFunctionName()
	{
		return TEXT("MainPS");
	}
};

#define VARIATION2(A, B) typedef FPostProcessMotionBlurPS<A, B> FPostProcessMotionBlurPS##A##B; \
	IMPLEMENT_SHADER_TYPE2(FPostProcessMotionBlurPS##A##B, SF_Pixel);

#define VARIATION1(A) VARIATION2(A, 0) VARIATION2(A, 1) VARIATION2(A, 2)

VARIATION1(1)
VARIATION1(2)
VARIATION1(3)
VARIATION1(4)

#undef VARIATION1
#undef VARIATION2

/**
 * @param Quality 0: visualize, 1:low, 2:medium, 3:high, 4:very high
 * @param InnerLoopConfig 0: fastest, 1: clamp uv, 2: clamp uv and handle different viewport res between scene color and depth.
 */
template< uint32 Quality, uint32 InnerLoopConfig >
class FPostProcessMotionBlurCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessMotionBlurCS, Global);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		// CS Params
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEX"), GMotionBlurComputeTileSizeX);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEY"), GMotionBlurComputeTileSizeY);

		// PS params
		OutEnvironment.SetDefine(TEXT("MOTION_BLUR_QUALITY"), Quality);
		OutEnvironment.SetDefine(TEXT("MOTION_BLUR_INNER_LOOP_CONFIG"), InnerLoopConfig);
	}

	/** Default constructor. */
	FPostProcessMotionBlurCS() {}

public:
	// CS params
	FRWShaderParameter OutComputeTex;
	FShaderParameter MotionBlurComputeParams;

	// PS params
	FPostProcessPassParameters PostprocessParameter;
	FDeferredPixelShaderParameters DeferredParameters;
	FShaderParameter MotionBlurParameters;
	FShaderParameter SceneColorBufferUVToViewBufferUV;
	FShaderParameter TileUVMax;

	/** Initialization constructor. */
	FPostProcessMotionBlurCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		// CS params
		OutComputeTex.Bind(Initializer.ParameterMap, TEXT("OutComputeTex"));
		MotionBlurComputeParams.Bind(Initializer.ParameterMap, TEXT("MotionBlurComputeParams"));

		// PS params
		PostprocessParameter.Bind(Initializer.ParameterMap);
		DeferredParameters.Bind(Initializer.ParameterMap);
		MotionBlurParameters.Bind(Initializer.ParameterMap, TEXT("MotionBlurParameters"));
		SceneColorBufferUVToViewBufferUV.Bind(Initializer.ParameterMap, TEXT("SceneColorBufferUVToViewBufferUV"));
		TileUVMax.Bind(Initializer.ParameterMap, TEXT("TileUVMax"));
	}

	template <typename TRHICmdList>
	void SetParameters(TRHICmdList& RHICmdList, const FRenderingCompositePassContext& Context, const FIntPoint& DestSize, FUnorderedAccessViewRHIParamRef DestUAV, float Scale)
	{
		const FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();

		// CS params
		FGlobalShader::SetParameters<FViewUniformShaderParameters>(RHICmdList, ShaderRHI, Context.View.ViewUniformBuffer);
		OutComputeTex.SetTexture(RHICmdList, ShaderRHI, nullptr, DestUAV);
		
		FVector4 MotionBlurComputeValues(0, 0, 1.f / (float)DestSize.X, 1.f / (float)DestSize.Y);
		SetShaderValue(RHICmdList, ShaderRHI, MotionBlurComputeParams, MotionBlurComputeValues);

		// PS params
		DeferredParameters.Set(RHICmdList, ShaderRHI, Context.View, MD_PostProcess);

		{
			bool bFiltered = false;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			bFiltered = CVarMotionBlurFiltering.GetValueOnRenderThread() != 0;
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

			if(bFiltered)
			{
				FSamplerStateRHIParamRef Filters[] =
				{
					TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
					TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
					TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
					TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI(),
				};

				PostprocessParameter.SetCS( ShaderRHI, Context, RHICmdList, 0, eFC_0000, Filters );
			}
			else
			{
				PostprocessParameter.SetCS(ShaderRHI, Context, RHICmdList, TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI());
			}
		}

		SetShaderValue(RHICmdList, ShaderRHI, MotionBlurParameters, GetMotionBlurParameters(Context, Scale));
		SetShaderValue(Context.RHICmdList, ShaderRHI, SceneColorBufferUVToViewBufferUV, GetSceneColorBufferUVToViewBufferUV(Context));

		if (TileUVMax.IsBound())
		{
			FIntPoint VelocityBufferSize = FSceneRenderTargets::Get(Context.RHICmdList).GetBufferSizeXY();

			FIntPoint TileBufferSize = GetNumTiles16x16(VelocityBufferSize);
			FIntPoint TileCount = GetNumTiles16x16(Context.View.ViewRect.Size());
			FVector2D TileUVMaxValue(
				(TileCount.X - 0.5f) / float(TileBufferSize.X),
				(TileCount.Y - 0.5f) / float(TileBufferSize.Y));

			SetShaderValue(Context.RHICmdList, ShaderRHI, TileUVMax, TileUVMaxValue);
		}
	}

	template <typename TRHICmdList>
	void UnsetParameters(TRHICmdList& RHICmdList)
	{
		const FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();
		OutComputeTex.UnsetUAV(RHICmdList, ShaderRHI);
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		// CS params
		Ar << OutComputeTex << MotionBlurComputeParams;
		// PS params
		Ar << PostprocessParameter << DeferredParameters << MotionBlurParameters << SceneColorBufferUVToViewBufferUV << TileUVMax;
		return bShaderHasOutdatedParameters;
	}

	static const TCHAR* GetSourceFilename()
	{
		return TEXT("/Engine/Private/PostProcessMotionBlur.usf");
	}

	static const TCHAR* GetFunctionName()
	{
		return TEXT("MainCS");
	}
};

#define VARIATION2(A, B) typedef FPostProcessMotionBlurCS<A, B> FPostProcessMotionBlurCS##A##B; \
	IMPLEMENT_SHADER_TYPE2(FPostProcessMotionBlurCS##A##B, SF_Compute);

#define VARIATION1(A) VARIATION2(A, 0) VARIATION2(A, 1) VARIATION2(A, 2)

VARIATION1(1)
VARIATION1(2)
VARIATION1(3)
VARIATION1(4)

#undef VARIATION1
#undef VARIATION2


template< uint32 Quality, uint32 InnerLoopConfig >
static void SetMotionBlurShaderNewTempl( const FRenderingCompositePassContext& Context, float Scale )
{
	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	Context.RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();

	TShaderMapRef< FPostProcessVS > VertexShader( Context.GetShaderMap() );
	TShaderMapRef< FPostProcessMotionBlurPS< Quality, InnerLoopConfig > >	PixelShader( Context.GetShaderMap() );

	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
	GraphicsPSOInit.PrimitiveType = PT_TriangleList;

	SetGraphicsPipelineState(Context.RHICmdList, GraphicsPSOInit);

	VertexShader->SetParameters(Context);
	PixelShader->SetParameters(Context, Scale);
}


template< uint32 Quality >
static void SetMotionBlurShaderNewTempl(const FRenderingCompositePassContext& Context, float Scale, uint32 InnerLoopConfig)
{
	switch (InnerLoopConfig)
	{
	case 0: SetMotionBlurShaderNewTempl<Quality, 0>(Context, Scale); break;
	case 1: SetMotionBlurShaderNewTempl<Quality, 1>(Context, Scale); break;
	case 2: SetMotionBlurShaderNewTempl<Quality, 2>(Context, Scale); break;
	default: check(0);
	}
}


void FRCPassPostProcessMotionBlur::Process(FRenderingCompositePassContext& Context)
{
	const FPooledRenderTargetDesc* InputDesc = GetInputDesc(ePId_Input0);
	AsyncEndFence = FComputeFenceRHIRef();

	if(!InputDesc)
	{
		// input is not hooked up correctly
		return;
	}

	const FViewInfo& View = Context.View;

	// we assume the input and output is full resolution

	FIntPoint SrcSize = InputDesc->Extent;
	FIntPoint DestSize = PassOutputs[0].RenderTargetDesc.Extent;

	FIntRect SrcRect = Context.SceneColorViewRect;
	FIntRect DestRect = SrcRect;

	float BlurScaleLUT[] =
	{
		1.0f - 0.5f / 4.0f,
		1.0f - 0.5f / 6.0f,
		1.0f - 0.5f / 8.0f,
		1.0f - 0.5f / 16.0f,
		1.0f /  4.0f * CVarMotionBlur2ndScale.GetValueOnRenderThread(),
		1.0f /  6.0f * CVarMotionBlur2ndScale.GetValueOnRenderThread(),
		1.0f /  8.0f * CVarMotionBlur2ndScale.GetValueOnRenderThread(),
		1.0f / 16.0f * CVarMotionBlur2ndScale.GetValueOnRenderThread(),
	};
	float Scale = Pass >= 0 ? BlurScaleLUT[ (Pass * 4) + (Quality - 1) ] : 1.0f;

	SCOPED_DRAW_EVENTF(Context.RHICmdList, MotionBlur, TEXT("MotionBlur%s %dx%d"),
		bIsComputePass?TEXT("Compute"):TEXT(""), SrcRect.Width(), SrcRect.Height());

	const FSceneRenderTargetItem& DestRenderTarget = PassOutputs[0].RequestSurface(Context);

	uint32 InnerLoopConfig = 1;
	if (Context.View.PrimaryScreenPercentageMethod == EPrimaryScreenPercentageMethod::TemporalUpscale)
	{
		InnerLoopConfig = 2;
	}
	else if (DestSize == SrcRect.Size() && SrcRect.Min.X == 0 && SrcRect.Min.Y == 0)
	{
		InnerLoopConfig = 0;
	}

	if (bIsComputePass)
	{
		// Common setup
		SetRenderTarget(Context.RHICmdList, nullptr, nullptr);
		Context.SetViewportAndCallRHI(DestRect, 0.0f, 1.0f);
		
		static FName AsyncEndFenceName(TEXT("AsyncMotionBlurEndFence"));
		AsyncEndFence = Context.RHICmdList.CreateComputeFence(AsyncEndFenceName);

		if (IsAsyncComputePass())
		{
			// Async path
			FRHIAsyncComputeCommandListImmediate& RHICmdListComputeImmediate = FRHICommandListExecutor::GetImmediateAsyncComputeCommandList();
			{
				SCOPED_COMPUTE_EVENT(RHICmdListComputeImmediate, AsyncMotionBlur);
				WaitForInputPassComputeFences(RHICmdListComputeImmediate);
				RHICmdListComputeImmediate.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EGfxToCompute, DestRenderTarget.UAV);
				DispatchCS(RHICmdListComputeImmediate, Context, DestSize, DestRect, DestRenderTarget.UAV, Scale, InnerLoopConfig);
				RHICmdListComputeImmediate.TransitionResource(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToGfx, DestRenderTarget.UAV, AsyncEndFence);
			}
			FRHIAsyncComputeCommandListImmediate::ImmediateDispatch(RHICmdListComputeImmediate);
		}
		else
		{
			// Direct path
			WaitForInputPassComputeFences(Context.RHICmdList);
			Context.RHICmdList.TransitionResource(EResourceTransitionAccess::ERWBarrier, EResourceTransitionPipeline::EGfxToCompute, DestRenderTarget.UAV);
			DispatchCS(Context.RHICmdList, Context, DestSize, DestRect, DestRenderTarget.UAV, Scale, InnerLoopConfig);			
			Context.RHICmdList.TransitionResource(EResourceTransitionAccess::EReadable, EResourceTransitionPipeline::EComputeToGfx, DestRenderTarget.UAV, AsyncEndFence);
		}
	}
	else
	{
		WaitForInputPassComputeFences(Context.RHICmdList);

		ERenderTargetLoadAction LoadAction = Context.GetLoadActionForRenderTarget(DestRenderTarget);

		FRHIRenderTargetView RtView = FRHIRenderTargetView(DestRenderTarget.TargetableTexture, LoadAction);
		FRHISetRenderTargetsInfo Info(1, &RtView, FRHIDepthRenderTargetView());
		Context.RHICmdList.SetRenderTargetsAndClear(Info);
		Context.SetViewportAndCallRHI(SrcRect);

		if(Quality == 1)
		{
			SetMotionBlurShaderNewTempl<1>( Context, Scale, InnerLoopConfig );
		}
		else if(Quality == 2)
		{
			SetMotionBlurShaderNewTempl<2>( Context, Scale, InnerLoopConfig );
		}
		else if(Quality == 3 || Pass > 0 )
		{
			SetMotionBlurShaderNewTempl<3>( Context, Scale, InnerLoopConfig );
		}
		else
		{
			check(Quality == 4);
			SetMotionBlurShaderNewTempl<4>( Context, Scale, InnerLoopConfig );
		}

		TShaderMapRef<FPostProcessVS> VertexShader(Context.GetShaderMap());
	
		DrawPostProcessPass(
			Context.RHICmdList,
			0, 0,
			SrcRect.Width(), SrcRect.Height(),
			SrcRect.Min.X, SrcRect.Min.Y,
			SrcRect.Width(), SrcRect.Height(),
			SrcRect.Size(),
			SrcSize,
			*VertexShader,
			View.StereoPass,
			Context.HasHmdMesh(),
			EDRF_UseTriangleOptimization);

		Context.RHICmdList.CopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
	}
}

template <typename TRHICmdList>
void FRCPassPostProcessMotionBlur::DispatchCS(TRHICmdList& RHICmdList, FRenderingCompositePassContext& Context, const FIntPoint& DestSize, const FIntRect& DestRect, FUnorderedAccessViewRHIParamRef DestUAV, float Scale, uint32 InnerLoopConfig)
{
	auto ShaderMap = Context.GetShaderMap();

	uint32 GroupSizeX = FMath::DivideAndRoundUp(DestSize.X, GMotionBlurComputeTileSizeX);
	uint32 GroupSizeY = FMath::DivideAndRoundUp(DestSize.Y, GMotionBlurComputeTileSizeY);

#define DISPATCH_CASE(A, B)																\
	case A:																				\
	{																					\
		TShaderMapRef<FPostProcessMotionBlurCS<A, B>> ComputeShader(ShaderMap);			\
		RHICmdList.SetComputeShader(ComputeShader->GetComputeShader());					\
		ComputeShader->SetParameters(RHICmdList, Context, DestSize, DestUAV, Scale);	\
		DispatchComputeShader(RHICmdList, *ComputeShader, GroupSizeX, GroupSizeY, 1);	\
		ComputeShader->UnsetParameters(RHICmdList);										\
	}																					\
	break;

	const uint32 CSQuality = (Pass > 0) ? 3 : Quality;

	if (InnerLoopConfig == 0)
	{
		switch (CSQuality)
		{
		DISPATCH_CASE(1, 0)
		DISPATCH_CASE(2, 0)
		DISPATCH_CASE(3, 0)
		DISPATCH_CASE(4, 0)
		default:
			check(0);
		}
	}
	else if (InnerLoopConfig == 1)
	{
		switch (CSQuality)
		{
		DISPATCH_CASE(1, 1)
		DISPATCH_CASE(2, 1)
		DISPATCH_CASE(3, 1)
		DISPATCH_CASE(4, 1)
		default:
			check(0);
		}
	}
	else if (InnerLoopConfig == 2)
	{
		switch (CSQuality)
		{
		DISPATCH_CASE(1, 2)
		DISPATCH_CASE(2, 2)
		DISPATCH_CASE(3, 2)
		DISPATCH_CASE(4, 2)
		default:
			check(0);
		}
	}
	else
	{
		check(0);
	}

#undef DISPATCH_CASE
}

FPooledRenderTargetDesc FRCPassPostProcessMotionBlur::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret = GetInput(ePId_Input0)->GetOutput()->RenderTargetDesc;
	Ret.Reset();

	Ret.TargetableFlags &= ~(TexCreate_RenderTargetable | TexCreate_UAV);
	Ret.TargetableFlags |= bIsComputePass ? TexCreate_UAV : TexCreate_RenderTargetable;

	if (!FPostProcessing::HasAlphaChannelSupport())
	{
		Ret.Format = PF_FloatRGB;
	}
	Ret.Flags |= GFastVRamConfig.MotionBlur;
	Ret.DebugName = TEXT("MotionBlur");
	Ret.AutoWritable = false;

	return Ret;
}




class FPostProcessVisualizeMotionBlurPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessVisualizeMotionBlurPS, Global);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM4);
	}

	/** Default constructor. */
	FPostProcessVisualizeMotionBlurPS() {}

public:
	FPostProcessPassParameters PostprocessParameter;
	FDeferredPixelShaderParameters DeferredParameters;
	FShaderParameter PrevViewProjMatrix;

	/** Initialization constructor. */
	FPostProcessVisualizeMotionBlurPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		DeferredParameters.Bind(Initializer.ParameterMap);
		PrevViewProjMatrix.Bind(Initializer.ParameterMap, TEXT("PrevViewProjMatrix"));
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar) override
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter << DeferredParameters << PrevViewProjMatrix;
		return bShaderHasOutdatedParameters;
	}

	template <typename TRHICmdList>
	void SetParameters(TRHICmdList& RHICmdList, const FRenderingCompositePassContext& Context)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters<FViewUniformShaderParameters>(RHICmdList, ShaderRHI, Context.View.ViewUniformBuffer);

		DeferredParameters.Set(RHICmdList, ShaderRHI, Context.View, MD_PostProcess);

		{
			bool bFiltered = false;

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
			bFiltered = CVarMotionBlurFiltering.GetValueOnRenderThread() != 0;
#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

			if(bFiltered)
			{
				PostprocessParameter.SetPS(RHICmdList, ShaderRHI, Context, TStaticSamplerState<SF_Bilinear,AM_Border,AM_Border,AM_Clamp>::GetRHI());
			}
			else
			{
				PostprocessParameter.SetPS(RHICmdList, ShaderRHI, Context, TStaticSamplerState<SF_Point,AM_Border,AM_Border,AM_Clamp>::GetRHI());
			}
		}
		
		if( Context.View.Family->EngineShowFlags.CameraInterpolation )
		{
			// Instead of finding the world space position of the current pixel, calculate the world space position offset by the camera position, 
			// then translate by the difference between last frame's camera position and this frame's camera position,
			// then apply the rest of the transforms.  This effectively avoids precision issues near the extents of large levels whose world space position is very large.
			FVector ViewOriginDelta = Context.View.ViewMatrices.GetViewOrigin() - Context.View.PrevViewInfo.ViewMatrices.GetViewOrigin();
			SetShaderValue(RHICmdList, ShaderRHI, PrevViewProjMatrix, FTranslationMatrix(ViewOriginDelta) * Context.View.PrevViewInfo.ViewMatrices.ComputeViewRotationProjectionMatrix());
		}
		else
		{
			SetShaderValue(RHICmdList, ShaderRHI, PrevViewProjMatrix, Context.View.ViewMatrices.ComputeViewRotationProjectionMatrix());
		}
	}

	static const TCHAR* GetSourceFilename()
	{
		return TEXT("/Engine/Private/PostProcessMotionBlur.usf");
	}

	static const TCHAR* GetFunctionName()
	{
		return TEXT("VisualizeMotionBlurPS");
	}
};

IMPLEMENT_SHADER_TYPE3(FPostProcessVisualizeMotionBlurPS, SF_Pixel);

void FRCPassPostProcessVisualizeMotionBlur::Process(FRenderingCompositePassContext& Context)
{
	SCOPED_DRAW_EVENT(Context.RHICmdList, VisualizeMotionBlur);

	const FPooledRenderTargetDesc* InputDesc = GetInputDesc(ePId_Input0);

	if(!InputDesc)
	{
		// input is not hooked up correctly
		return;
	}

	const FViewInfo& View = Context.View;
	const FSceneViewFamily& ViewFamily = *(View.Family);

	FIntPoint TexSize = InputDesc->Extent;

	// we assume the input and output is full resolution

	FIntPoint SrcSize = InputDesc->Extent;
	FIntPoint DestSize = PassOutputs[0].RenderTargetDesc.Extent;

	// e.g. 4 means the input texture is 4x smaller than the buffer size
	uint32 ScaleFactor = FSceneRenderTargets::Get(Context.RHICmdList).GetBufferSizeXY().X / SrcSize.X;

	FIntRect SrcRect = FIntRect::DivideAndRoundUp(View.ViewRect, ScaleFactor);
	FIntRect DestRect = SrcRect;

	const FSceneRenderTargetItem& DestRenderTarget = PassOutputs[0].RequestSurface(Context);

	// Set the view family's render target/viewport.
	FRHIRenderTargetView RtView = FRHIRenderTargetView(DestRenderTarget.TargetableTexture, ERenderTargetLoadAction::ENoAction);
	FRHISetRenderTargetsInfo Info(1, &RtView, FRHIDepthRenderTargetView());
	Context.RHICmdList.SetRenderTargetsAndClear(Info);
	Context.SetViewportAndCallRHI(SrcRect);

	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	Context.RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();

	TShaderMapRef<FPostProcessVS> VertexShader(Context.GetShaderMap());
	TShaderMapRef<FPostProcessVisualizeMotionBlurPS> PixelShader(Context.GetShaderMap());

	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = GETSAFERHISHADER_VERTEX(*VertexShader);
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = GETSAFERHISHADER_PIXEL(*PixelShader);
	GraphicsPSOInit.PrimitiveType = PT_TriangleList;

	SetGraphicsPipelineState(Context.RHICmdList, GraphicsPSOInit);

	VertexShader->SetParameters(Context);
	PixelShader->SetParameters(Context.RHICmdList, Context);

	// Draw a quad mapping scene color to the view's render target
	DrawRectangle(
		Context.RHICmdList,
		0, 0,
		SrcRect.Width(), SrcRect.Height(),
		SrcRect.Min.X, SrcRect.Min.Y, 
		SrcRect.Width(), SrcRect.Height(),
		SrcRect.Size(),
		SrcSize,
		*VertexShader,
		EDRF_UseTriangleOptimization);

	FRenderTargetTemp TempRenderTarget(View, (const FTexture2DRHIRef&)DestRenderTarget.TargetableTexture);
	FCanvas Canvas(&TempRenderTarget, NULL, ViewFamily.CurrentRealTime, ViewFamily.CurrentWorldTime, ViewFamily.DeltaWorldTime, Context.GetFeatureLevel());

	float X = 20;
	float Y = 38;
	const float YStep = 14;
	const float ColumnWidth = 200;

	FString Line;

	Line = FString::Printf(TEXT("Visualize MotionBlur"));
	Canvas.DrawShadowedString(X, Y += YStep, *Line, GetStatsFont(), FLinearColor(1, 1, 0));
	
	static const auto MotionBlurDebugVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.MotionBlurDebug"));
	const int32 MotionBlurDebug = MotionBlurDebugVar ? MotionBlurDebugVar->GetValueOnRenderThread() : 0;

	Line = FString::Printf(TEXT("%d, %d"), ViewFamily.FrameNumber, MotionBlurDebug);
	Canvas.DrawShadowedString(X, Y += YStep, TEXT("FrameNo, r.MotionBlurDebug:"), GetStatsFont(), FLinearColor(1, 1, 0));
	Canvas.DrawShadowedString(X + ColumnWidth, Y, *Line, GetStatsFont(), FLinearColor(1, 1, 0));

	static const auto VelocityTestVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.VelocityTest"));
	const int32 VelocityTest = VelocityTestVar ? VelocityTestVar->GetValueOnRenderThread() : 0;
	
	extern bool IsParallelVelocity();

	Line = FString::Printf(TEXT("%d, %d, %d"), ViewFamily.bWorldIsPaused, VelocityTest, IsParallelVelocity());
	Canvas.DrawShadowedString(X, Y += YStep, TEXT("Paused, r.VelocityTest, Parallel:"), GetStatsFont(), FLinearColor(1, 1, 0));
	Canvas.DrawShadowedString(X + ColumnWidth, Y, *Line, GetStatsFont(), FLinearColor(1, 1, 0));

	const FScene* Scene = (const FScene*)View.Family->Scene;

	Canvas.DrawShadowedString(X, Y += YStep, TEXT("MotionBlurInfoData (per object):"), GetStatsFont(), FLinearColor(1, 1, 0));
	Canvas.DrawShadowedString(X + ColumnWidth, Y, *Scene->MotionBlurInfoData.GetDebugString(), GetStatsFont(), FLinearColor(1, 1, 0));	

	const FSceneViewState *SceneViewState = (const FSceneViewState*)View.State;

	Line = FString::Printf(TEXT("View=%.4x PrevView=%.4x"),
		View.ViewMatrices.GetViewMatrix().ComputeHash() & 0xffff,
		View.PrevViewInfo.ViewMatrices.GetViewMatrix().ComputeHash() & 0xffff);
	Canvas.DrawShadowedString(X, Y += YStep, TEXT("ViewMatrix:"), GetStatsFont(), FLinearColor(1, 1, 0));
	Canvas.DrawShadowedString(X + ColumnWidth, Y, *Line, GetStatsFont(), FLinearColor(1, 1, 0));

	Canvas.Flush_RenderThread(Context.RHICmdList);

	Context.RHICmdList.CopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
}

FPooledRenderTargetDesc FRCPassPostProcessVisualizeMotionBlur::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc Ret = GetInput(ePId_Input0)->GetOutput()->RenderTargetDesc;

	Ret.Reset();
	Ret.DebugName = TEXT("MotionBlur");
	Ret.AutoWritable = false;

	return Ret;
}
