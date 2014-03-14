// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	GPUBenchmark.cpp: GPUBenchmark to compute performance index to set video options automatically
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "SceneFilterRendering.h"
#include "GPUBenchmark.h"

static uint32 GBenchmarkResolution = 512;

DEFINE_LOG_CATEGORY_STATIC(LogSynthBenchmark, Log, All);


// todo: get rid of global
FRenderQueryPool GTimerQueryPool(RQT_AbsoluteTime);

/** Encapsulates the post processing down sample pixel shader. */
template <uint32 Method>
class FPostProcessBenchmarkPS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessBenchmarkPS, Global);

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3);
	}

	static void ModifyCompilationEnvironment(EShaderPlatform Platform, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Platform, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("METHOD"), Method);
	}

	/** Default constructor. */
	FPostProcessBenchmarkPS() {}

public:
	FShaderResourceParameter InputTexture;
	FShaderResourceParameter InputTextureSampler;

	/** Initialization constructor. */
	FPostProcessBenchmarkPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
		InputTexture.Bind(Initializer.ParameterMap,TEXT("InputTexture"));
		InputTextureSampler.Bind(Initializer.ParameterMap,TEXT("InputTextureSampler"));
	}

	// FShader interface.
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);
		Ar << InputTexture << InputTextureSampler;
		return bShaderHasOutdatedParameters;
	}

	void SetParameters(const FSceneView& View, TRefCountPtr<IPooledRenderTarget>& Src)
	{
		const FPixelShaderRHIParamRef ShaderRHI = GetPixelShader();

		FGlobalShader::SetParameters(ShaderRHI, View);

		SetTextureParameter(ShaderRHI, InputTexture, InputTextureSampler, TStaticSamplerState<>::GetRHI(), Src->GetRenderTargetItem().ShaderResourceTexture);
	}

	static const TCHAR* GetSourceFilename()
	{
		return TEXT("GPUBenchmark");
	}

	static const TCHAR* GetFunctionName()
	{
		return TEXT("MainPS");
	}
};

// #define avoids a lot of code duplication
#define VARIATION1(A) typedef FPostProcessBenchmarkPS<A> FPostProcessBenchmarkPS##A; \
	IMPLEMENT_SHADER_TYPE2(FPostProcessBenchmarkPS##A, SF_Pixel);

VARIATION1(0)			VARIATION1(1)			VARIATION1(2)			VARIATION1(3)			VARIATION1(4)
#undef VARIATION1


/** Encapsulates the post processing down sample vertex shader. */
class FPostProcessBenchmarkVS : public FGlobalShader
{
	DECLARE_SHADER_TYPE(FPostProcessBenchmarkVS,Global);
public:

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return true;
	}

	/** Default constructor. */
	FPostProcessBenchmarkVS() {}
	
	/** Initialization constructor. */
	FPostProcessBenchmarkVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer):
		FGlobalShader(Initializer)
	{
	}

	/** Serializer */
	virtual bool Serialize(FArchive& Ar)
	{
		bool bShaderHasOutdatedParameters = FGlobalShader::Serialize(Ar);

		return bShaderHasOutdatedParameters;
	}

	void SetParameters(const FSceneView& View)
	{
		const FVertexShaderRHIParamRef ShaderRHI = GetVertexShader();
		
		FGlobalShader::SetParameters(ShaderRHI, View);
	}
};

IMPLEMENT_SHADER_TYPE(,FPostProcessBenchmarkVS,TEXT("GPUBenchmark"),TEXT("MainBenchmarkVS"),SF_Vertex);


template <uint32 Method>
void RunBenchmarkShader(const FSceneView& View, TRefCountPtr<IPooledRenderTarget>& Src, uint32 Count)
{
	TShaderMapRef<FPostProcessBenchmarkVS> VertexShader(GetGlobalShaderMap());
	TShaderMapRef<FPostProcessBenchmarkPS<Method> > PixelShader(GetGlobalShaderMap());

	static FGlobalBoundShaderState BoundShaderState;

	SetGlobalBoundShaderState(BoundShaderState, GFilterVertexDeclaration.VertexDeclarationRHI, *VertexShader, *PixelShader);

	PixelShader->SetParameters(View, Src);
	VertexShader->SetParameters(View);

	for(uint32 i = 0; i < Count; ++i)
	{
		DrawRectangle(
			0, 0,
			GBenchmarkResolution, GBenchmarkResolution,
			0, 0,
			GBenchmarkResolution, GBenchmarkResolution,
			FIntPoint(GBenchmarkResolution, GBenchmarkResolution),
			FIntPoint(GBenchmarkResolution, GBenchmarkResolution),
			EDRF_Default);
	}
}

void RunBenchmarkShader(const FSceneView& View, uint32 MethodId, TRefCountPtr<IPooledRenderTarget>& Src, uint32 Count)
{
	SCOPED_DRAW_EVENTF(Benchmark, DEC_SCENE_ITEMS, TEXT("Benchmark Method:%d"), MethodId);

	switch(MethodId)
	{
		case 0: RunBenchmarkShader<0>(View, Src, Count); return;
		case 1: RunBenchmarkShader<1>(View, Src, Count); return;
		case 2: RunBenchmarkShader<2>(View, Src, Count); return;
		case 3: RunBenchmarkShader<3>(View, Src, Count); return;
		case 4: RunBenchmarkShader<4>(View, Src, Count); return;

		default:
			check(0);
	}
}

// Many Benchmark timings stored in an array to allow to extract a good value dropping outliers
// We need to get rid of the bad samples.
class FTimingSeries
{
public:
	// @param ArraySize
	void Init(uint32 ArraySize)
	{
		check(ArraySize > 0);

		TimingValues.AddZeroed(ArraySize);
	}

	//
	void SetEntry(uint32 Index, float TimingValue)
	{
		check(Index < (uint32)TimingValues.Num());

		TimingValues[Index] = TimingValue;
	}

	//
	float GetEntry(uint32 Index) const
	{
		check(Index < (uint32)TimingValues.Num());

		return TimingValues[Index];
	}

	// @param OutConfidence
	float ComputeValue(float& OutConfidence) const
	{
		float Ret = 0.0f;

		TArray<float> SortedValues;
		{
			// a lot of values in the beginning are wrong, we cut off some part (1/4 of the samples area)
			uint32 StartIndex = TimingValues.Num() / 3;

			for(uint32 Index = StartIndex; Index < (uint32)TimingValues.Num(); ++Index)
			{
				SortedValues.Add(TimingValues[Index]);
			}
			SortedValues.Sort();
		}

		OutConfidence = 0.0f;
		
		uint32 Passes = 10;

		// slow but simple
		for(uint32 Pass = 0; Pass < Passes; ++Pass)
		{
			// 0..1, 0 not included
			float Alpha = (Pass + 1) / (float)Passes;

			int32 MidIndex = SortedValues.Num() / 2;
			int32 FromIndex = (int32)FMath::Lerp(MidIndex, 0, Alpha); 
			int32 ToIndex = (int32)FMath::Lerp(MidIndex, SortedValues.Num(), Alpha); 

			float Delta = 0.0f;
			float Confidence = 0.0f;

			float TimingValue = ComputeTimingFromSortedRange(FromIndex, ToIndex, SortedValues, Delta, Confidence);

			// aim for 5% delta and some samples
			if(Pass > 0 && Delta > TimingValue * 0.5f)
			{
				// it gets worse, take the best one we had so far
				break;
			}

			OutConfidence = Confidence;
			Ret = TimingValue;
		}

		return Ret;
	}

private:
	// @param FromIndex within SortedValues
	// @param ToIndex within SortedValues
	// @param OutDelta +/- 
	// @param OutConfidence 0..100 0=not at all, 100=fully, meaning how many samples are considered useful
	// @return TimingValue, smaller is better
	static float ComputeTimingFromSortedRange(int32 FromIndex, int32 ToIndex, const TArray<float>& SortedValues, float& OutDelta, float& OutConfidence)
	{
		float ValidSum = 0;
		uint32 ValidCount = 0;
		float Min = FLT_MAX;
		float Max = -FLT_MAX;
		{
			for(int32 Index = FromIndex; Index < ToIndex; ++Index)
			{
				float Value = SortedValues[Index];

				Min = FMath::Min(Min, Value);
				Max = FMath::Max(Max, Value);

				ValidSum += Value;
				++ValidCount;
			}
		}

		if(ValidCount)
		{
			OutDelta = (Max - Min) * 0.5f;

			OutConfidence = 100.0f * ValidCount / (float)SortedValues.Num();

			return ValidSum / ValidCount;
		}
		else
		{
			OutDelta = 0.0f;
			OutConfidence = 0.0f;
			return 0.0f;
		}
	}

	TArray<float> TimingValues;
};

#if !UE_BUILD_SHIPPING
// for debugging only
class FBenchmarkGraph
{
public:
	FBenchmarkGraph(uint32 InWidth, uint32 InHeight, const TCHAR* InFilePath)
		: FilePath(InFilePath)
		, Height(InHeight)
		, Width(InWidth)
	{
		Data.AddZeroed(Width * Height);
	}


	// @param x 0..Width-1
	// @param y 0..Height-1
	void DrawPixel(int32 X, int32 Y, FColor Color = FColor::White)
	{
		if((uint32)X < Width && (uint32)Y < Height)
		{
			Data[X + Y * Width] = Color;
		}
	}

	// @param x 0..Width-1
	// @param Value 0..1
	void DrawBar(int32 X, float Value)
	{
		Value = FMath::Clamp(Value, 0.0f, 1.0f);

		uint32 StartY = (uint32)((1.0f - Value) * Height);
		for(uint32 Y = StartY; Y < Height; ++Y)
		{
			bool bBetterThanReference = Y < (Height / 2);
			// better than reference system: green, otherwise red
			DrawPixel(X, Y, bBetterThanReference ? FColor::Green : FColor::Red);
		}
	}

	void Save() const
	{
		FFileHelper::CreateBitmap(*FilePath, Width, Height, Data.GetData());
	}

private:
	TArray<FColor> Data;
	FString FilePath;
	uint32 Height;
	uint32 Width;
};
#endif


void RendererGPUBenchmark(FSynthBenchmarkResults& InOut, const FSceneView& View, uint32 WorkScale, bool bDebugOut)
{
	check(IsInRenderingThread());
	
	// two RT to ping pong so we force the GPU to flush it's pipeline
	TRefCountPtr<IPooledRenderTarget> RTItems[3];
	{
		FPooledRenderTargetDesc Desc(FPooledRenderTargetDesc::Create2DDesc(FIntPoint(GBenchmarkResolution, GBenchmarkResolution), PF_B8G8R8A8, TexCreate_None, TexCreate_RenderTargetable | TexCreate_ShaderResource, false));
		GRenderTargetPool.FindFreeElement(Desc, RTItems[0], TEXT("Benchmark0"));
		GRenderTargetPool.FindFreeElement(Desc, RTItems[1], TEXT("Benchmark1"));

		Desc.Extent = FIntPoint(1, 1);
		Desc.Flags = TexCreate_CPUReadback;	// needs TexCreate_ResolveTargetable?
		Desc.TargetableFlags = TexCreate_None;

		GRenderTargetPool.FindFreeElement(Desc, RTItems[2], TEXT("BenchmarkReadback"));
	}

	// set the state
	RHISetBlendState(TStaticBlendState<>::GetRHI());
	RHISetRasterizerState(TStaticRasterizerState<>::GetRHI());
	RHISetDepthStencilState(TStaticDepthStencilState<false,CF_Always>::GetRHI());

	{
		// larger number means more accuracy but slower, some slower GPUs might timeout with a number to large
		const uint32 IterationCount = 70;
		const uint32 MethodCount = ARRAY_COUNT(InOut.GPUStats);

		// 0 / 1
		uint32 DestRTIndex = 0;

		const uint32 TimerSampleCount = IterationCount * MethodCount + 1;

		static FRenderQueryRHIRef TimerQueries[TimerSampleCount];
		static uint32 PassCount[IterationCount];

		for(uint32  i = 0; i < TimerSampleCount; ++i)
		{
			TimerQueries[i] = GTimerQueryPool.AllocateQuery();
		}

		if(!TimerQueries[0])
		{
			UE_LOG(LogSynthBenchmark, Warning, TEXT("GPU driver does not support timer queries."));
		}

		// TimingValues are in Seconds per GPixel
		FTimingSeries TimingSeries[MethodCount];
		
		for(uint32 MethodIterator = 0; MethodIterator < MethodCount; ++MethodIterator)
		{
			TimingSeries[MethodIterator].Init(IterationCount);
		}

		check(MethodCount == 5);
		InOut.GPUStats[0] = FSynthBenchmarkStat(TEXT("ALUHeavyNoise"), 1.0f / 4.601f, TEXT("s/GigaPix"));
		InOut.GPUStats[1] = FSynthBenchmarkStat(TEXT("TexHeavy"), 1.0f / 7.447f, TEXT("s/GigaPix"));
		InOut.GPUStats[2] = FSynthBenchmarkStat(TEXT("DepTexHeavy"), 1.0f / 3.847f, TEXT("s/GigaPix"));
		InOut.GPUStats[3] = FSynthBenchmarkStat(TEXT("FillOnly"), 1.0f / 25.463f, TEXT("s/GigaPix"));
		InOut.GPUStats[4] = FSynthBenchmarkStat(TEXT("Bandwidth"), 1.0f / 1.072f, TEXT("s/GigaPix"));

		// e.g. on NV670: Method3 (mostly fill rate )-> 26GP/s (seems realistic)
		// reference: http://en.wikipedia.org/wiki/Comparison_of_Nvidia_graphics_processing_units theoretical: 29.3G/s

		RHIEndRenderQuery(TimerQueries[0]);

		// multiple iterations to see how trust able the values are
		for(uint32 Iteration = 0; Iteration < IterationCount; ++Iteration)
		{
			for(uint32 MethodIterator = 0; MethodIterator < MethodCount; ++MethodIterator)
			{
				// alternate between forward and backward (should give the same number)
				//			uint32 MethodId = (Iteration % 2) ? MethodIterator : (MethodCount - 1 - MethodIterator);
				uint32 MethodId = MethodIterator;

				uint32 QueryIndex = 1 + Iteration * MethodCount + MethodId;

				// 0 / 1
				const uint32 SrcRTIndex = 1 - DestRTIndex;

				GRenderTargetPool.VisualizeTexture.SetCheckPoint(RTItems[DestRTIndex]);

				RHISetRenderTarget(RTItems[DestRTIndex]->GetRenderTargetItem().TargetableTexture, FTextureRHIRef());	

				// decide how much work we do in this pass
				PassCount[Iteration] = (Iteration / 10 + 1) * WorkScale;

				RunBenchmarkShader(View, MethodId, RTItems[SrcRTIndex], PassCount[Iteration]);

				RHICopyToResolveTarget(RTItems[DestRTIndex]->GetRenderTargetItem().TargetableTexture, RTItems[DestRTIndex]->GetRenderTargetItem().ShaderResourceTexture, false, FResolveParams());

				/*if(bGPUCPUSync)
				{
					// more consistent timing but strangely much faster to the level that is unrealistic

					FResolveParams Param;

					Param.Rect = FResolveRect(0, 0, 1, 1);
					RHICopyToResolveTarget(
						RTItems[DestRTIndex]->GetRenderTargetItem().TargetableTexture,
						RTItems[2]->GetRenderTargetItem().ShaderResourceTexture,
						false,
						Param);

					void* Data = 0;
					int Width = 0;
					int Height = 0;

					RHIMapStagingSurface(RTItems[2]->GetRenderTargetItem().ShaderResourceTexture, Data, Width, Height);
					RHIUnmapStagingSurface(RTItems[2]->GetRenderTargetItem().ShaderResourceTexture);
				}*/

				RHIEndRenderQuery(TimerQueries[QueryIndex]);

				// ping pong
				DestRTIndex = 1 - DestRTIndex;
			}
		}

		{
			uint64 OldAbsTime = 0;
			RHIGetRenderQueryResult(TimerQueries[0], OldAbsTime, true);
			GTimerQueryPool.ReleaseQuery(TimerQueries[0]);

#if !UE_BUILD_SHIPPING
			FBenchmarkGraph BenchmarkGraph(IterationCount, IterationCount, *(FPaths::ScreenShotDir() + TEXT("GPUSynthBenchmarkGraph.bmp")));
#endif

			for(uint32 Iteration = 0; Iteration < IterationCount; ++Iteration)
			{
				uint32 Results[MethodCount];

				for(uint32 MethodId = 0; MethodId < MethodCount; ++MethodId)
				{
					uint32 QueryIndex = 1 + Iteration * MethodCount + MethodId;

					uint64 AbsTime;
					RHIGetRenderQueryResult(TimerQueries[QueryIndex], AbsTime, true);
					GTimerQueryPool.ReleaseQuery(TimerQueries[QueryIndex]);

					Results[MethodId] = AbsTime - OldAbsTime;
					OldAbsTime = AbsTime;
				}

				double SamplesInGPix = PassCount[Iteration] * GBenchmarkResolution * GBenchmarkResolution / 1000000000.0;

				for(uint32 MethodId = 0; MethodId < MethodCount; ++MethodId)
				{
					double TimeInSec = Results[MethodId] / 1000000.0;
					double TimingValue = TimeInSec / SamplesInGPix;

					// TimingValue in Seconds per GPixel
					TimingSeries[MethodId].SetEntry(Iteration, (float)TimingValue);
				}

#if !UE_BUILD_SHIPPING
				{
					// This is for debugging and we don't want to change the output but we still use "InOut".
					// That shouldn't hurt, as we override the values after that anyway.

					for(uint32 MethodId = 0; MethodId < MethodCount; ++MethodId)
					{
						InOut.GPUStats[MethodId].SetMeasuredTime(TimingSeries[MethodId].GetEntry(Iteration));
					}

					float LocalGPUIndex = InOut.ComputeGPUPerfIndex();

					// * 0.01 to get it in 0..1 range
					// * 0.5f to have 100 is the middle
					BenchmarkGraph.DrawBar(Iteration, LocalGPUIndex * 0.01f * 0.5f);
				}
#endif
			}

			for(uint32 MethodId = 0; MethodId < MethodCount; ++MethodId)
			{
				float Confidence = 0.0f;
				
				float TimingValue = TimingSeries[MethodId].ComputeValue(Confidence);

				if(Confidence > 0)
				{
					InOut.GPUStats[MethodId].SetMeasuredTime(TimingValue, Confidence);
				}

				UE_LOG(LogSynthBenchmark, Display, TEXT("         ... %.3f GigaPix/s, Confidence=%.0f%% '%s'"),
					1.0f / InOut.GPUStats[MethodId].GetMeasuredTime(), Confidence, InOut.GPUStats[MethodId].GetDesc());
			}

			UE_LOG(LogSynthBenchmark, Display, TEXT(""));
			
#if !UE_BUILD_SHIPPING
			if(bDebugOut)
			{
				BenchmarkGraph.Save();
			}
#endif
		}
	}
}
