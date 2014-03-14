// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessHistogram.cpp: Post processing histogram implementation.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "PostProcessHistogram.h"
#include "PostProcessing.h"
#include "PostProcessEyeAdaptation.h"


/** Encapsulates the post processing histogram compute shader. */
class FPostProcessHistogramCS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessHistogramCS, Global);

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEX"), FRCPassPostProcessHistogram::ThreadGroupSizeX);
		OutEnvironment.SetDefine(TEXT("THREADGROUP_SIZEY"), FRCPassPostProcessHistogram::ThreadGroupSizeY);
		OutEnvironment.SetDefine(TEXT("LOOP_SIZEX"), FRCPassPostProcessHistogram::LoopCountX);
		OutEnvironment.SetDefine(TEXT("LOOP_SIZEY"), FRCPassPostProcessHistogram::LoopCountY);
		OutEnvironment.SetDefine(TEXT("HISTOGRAM_SIZE"), FRCPassPostProcessHistogram::HistogramSize);
		OutEnvironment.CompilerFlags.Add( CFLAG_StandardOptimization );
	}

	/** Default constructor. */
	FPostProcessHistogramCS() {}

public:
	FPostProcessPassParameters PostprocessParameter;
	FShaderResourceParameter HistogramRWTexture;
	FShaderParameter HistogramParameters;
	FShaderParameter ThreadGroupCount;
	FShaderParameter LeftTopOffset;
	FShaderParameter EyeAdaptationParams;

	/** Initialization constructor. */
	FPostProcessHistogramCS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		PostprocessParameter.Bind(Initializer.ParameterMap);
		HistogramRWTexture.Bind(Initializer.ParameterMap, TEXT("HistogramRWTexture"));
		HistogramParameters.Bind(Initializer.ParameterMap, TEXT("HistogramParameters"));
		ThreadGroupCount.Bind(Initializer.ParameterMap, TEXT("ThreadGroupCount"));
		LeftTopOffset.Bind(Initializer.ParameterMap, TEXT("LeftTopOffset"));
		EyeAdaptationParams.Bind(Initializer.ParameterMap, TEXT("EyeAdaptationParams"));
	}

	void SetCS(const FRenderingCompositePassContext& Context, FIntPoint ThreadGroupCountValue, FIntPoint LeftTopOffsetValue, FIntPoint GatherExtent)
	{
		const FComputeShaderRHIParamRef ShaderRHI = GetComputeShader();

		FGlobalShader::SetParameters(ShaderRHI, Context.View);

		PostprocessParameter.SetCS(ShaderRHI, Context, TStaticSamplerState<SF_Point,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI());

		SetShaderValue(ShaderRHI, ThreadGroupCount, ThreadGroupCountValue);
		SetShaderValue(ShaderRHI, LeftTopOffset, LeftTopOffsetValue);

		FVector4 HistogramParametersValue(GatherExtent.X, GatherExtent.Y, 0, 0);
		SetShaderValue(ShaderRHI, HistogramParameters, HistogramParametersValue);

		{
			FVector4 Temp[3];

			FRCPassPostProcessEyeAdaptation::ComputeEyeAdaptationParamsValue(Context.View, Temp);
			SetShaderValueArray(ShaderRHI, EyeAdaptationParams, Temp, 3);
		}
	}
	
	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << PostprocessParameter << HistogramRWTexture << HistogramParameters << ThreadGroupCount << LeftTopOffset << EyeAdaptationParams;
		return bShaderHasOutdatedParameters;
	}
};

IMPLEMENT_SHADER_TYPE(,FPostProcessHistogramCS,TEXT("PostProcessHistogram"),TEXT("MainCS"),SF_Compute);

void FRCPassPostProcessHistogram::Process(FRenderingCompositePassContext& Context)
{
	SCOPED_DRAW_EVENT(PostProcessHistogram, DEC_SCENE_ITEMS);
	const FPooledRenderTargetDesc* InputDesc = GetInputDesc(ePId_Input0);

	if(!InputDesc)
	{
		// input is not hooked up correctly
		return;
	}

	const FSceneView& View = Context.View;
	const FSceneViewFamily& ViewFamily = *(View.Family);
	
	FIntPoint SrcSize = InputDesc->Extent;
	FIntRect DestRect = View.ViewRect;

	const FSceneRenderTargetItem& DestRenderTarget = PassOutputs[0].RequestSurface(Context);

	TShaderMapRef<FPostProcessHistogramCS> ComputeShader(GetGlobalShaderMap());

	RHISetComputeShader(ComputeShader->GetComputeShader());
	RHISetRenderTarget(FTextureRHIRef(), FTextureRHIRef());	
	

	// set destination
	check(DestRenderTarget.UAV);
	RHISetUAVParameter(ComputeShader->GetComputeShader(), ComputeShader->HistogramRWTexture.GetBaseIndex(), DestRenderTarget.UAV);

	// we currently assume the input is half res, one full res pixel less to avoid getting bilinear filtered input
	FIntPoint GatherExtent = (DestRect.Size() - FIntPoint(1, 1)) / 2;

	FIntPoint ThreadGroupCountValue = ComputeThreadGroupCount(GatherExtent);

	
	ComputeShader->SetCS(Context, ThreadGroupCountValue, (DestRect.Min + FIntPoint(1, 1)) / 2, GatherExtent);
	
	DispatchComputeShader(*ComputeShader, ThreadGroupCountValue.X, ThreadGroupCountValue.Y, 1);

	// un-set destination
	RHISetUAVParameter(ComputeShader->GetComputeShader(), ComputeShader->HistogramRWTexture.GetBaseIndex(), NULL);

	RHICopyToResolveTarget(DestRenderTarget.TargetableTexture, DestRenderTarget.ShaderResourceTexture, false, FResolveParams());
}

FIntPoint FRCPassPostProcessHistogram::ComputeThreadGroupCount(FIntPoint PixelExtent)
{
	uint32 TexelPerThreadGroupX = ThreadGroupSizeX * LoopCountX;
	uint32 TexelPerThreadGroupY = ThreadGroupSizeY * LoopCountY;

	uint32 ThreadGroupCountX = (PixelExtent.X + TexelPerThreadGroupX - 1) / TexelPerThreadGroupX;
	uint32 ThreadGroupCountY = (PixelExtent.Y + TexelPerThreadGroupY - 1) / TexelPerThreadGroupY;

	return FIntPoint(ThreadGroupCountX, ThreadGroupCountY);
}

FPooledRenderTargetDesc FRCPassPostProcessHistogram::ComputeOutputDesc(EPassOutputId InPassOutputId) const
{
	FPooledRenderTargetDesc UnmodifiedRet = PassInputs[0].GetOutput()->RenderTargetDesc;

	UnmodifiedRet.Reset();
	FIntPoint PixelExtent = UnmodifiedRet.Extent;

	FIntPoint ThreadGroupCount = ComputeThreadGroupCount(PixelExtent);

	// each ThreadGroup outputs one histogram
	FIntPoint NewSize = FIntPoint(HistogramTexelCount, ThreadGroupCount.X * ThreadGroupCount.Y);

	// format can be optimized later
	FPooledRenderTargetDesc Ret(FPooledRenderTargetDesc::Create2DDesc(NewSize, PF_FloatRGBA, TexCreate_None, TexCreate_RenderTargetable | TexCreate_UAV, false));

	Ret.DebugName = TEXT("Histogram");

	return Ret;
}
