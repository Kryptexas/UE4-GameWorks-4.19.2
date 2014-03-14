// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PostProcessing.cpp: The center for all post processing activities.
=============================================================================*/

#include "RendererPrivate.h"
#include "ScenePrivate.h"
#include "PostProcessing.h"
#include "PostProcessAA.h"
#include "PostProcessTonemap.h"
#include "PostProcessMaterial.h"
#include "PostProcessInput.h"
#include "PostProcessWeightedSampleSum.h"
#include "PostProcessBloomSetup.h"
#include "PostProcessMobile.h"
#include "PostProcessDownsample.h"
#include "PostProcessHistogram.h"
#include "PostProcessHistogramReduce.h"
#include "PostProcessVisualizeHDR.h"
#include "PostProcessSelectionOutline.h"
#include "PostProcessGBufferHints.h"
#include "PostProcessVisualizeBuffer.h"
#include "PostProcessEyeAdaptation.h"
#include "PostProcessLensFlares.h"
#include "PostProcessLensBlur.h"
#include "PostProcessBokehDOF.h"
#include "PostProcessBokehDOFRecombine.h"
#include "PostProcessCombineLUTs.h"
#include "BatchedElements.h"
#include "ScreenRendering.h"
#include "PostProcessTemporalAA.h"
#include "PostProcessMotionBlur.h"
#include "PostProcessDOF.h"
#include "PostProcessUpscale.h"
#include "PostProcessHMD.h"
#include "PostProcessVisualizeComplexity.h"
#include "PostProcessCompositeEditorPrimitives.h"
#include "PostProcessPassThrough.h"
#include "PostProcessAmbientOcclusion.h"
#include "ScreenSpaceReflections.h"
#include "PostProcessTestImage.h"
#include "PostProcessSceneColorFringe.h"
#include "PostProcessSubsurface.h"
#include "PostProcessUIBlur.h"
#include "HighResScreenshot.h"

/** The global center for all post processing activities. */
FPostProcessing GPostProcessing;

static TAutoConsoleVariable<int32> CVarRenderTargetSwitchWorkaround(
	TEXT("r.RenderTargetSwitchWorkaround"),
	0,
	TEXT("Workaround needed on some mobile platforms to avoid a performance drop related to switching render targets.\n")
	TEXT("Only enabled on some hardware. This affects the bloom quality a bit. It runs slower than the normal code path but\n")
	TEXT("still faster as it avoids the many render target switches. (Default: 0)\n")
	TEXT("We want this enabled (1) on all 32 bit iOS devices (implemented through DeviceProfiles)."),
	ECVF_RenderThreadSafe);

IMPLEMENT_SHADER_TYPE(,FPostProcessVS,TEXT("PostProcessBloom"),TEXT("MainPostprocessCommonVS"),SF_Vertex);

// -------------------------------------------------------

FPostprocessContext::FPostprocessContext(class FRenderingCompositionGraph& InGraph, const FViewInfo& InView)
	: Graph(InGraph)
	, View(InView)
{
	SceneColor = Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessInput(GSceneRenderTargets.SceneColor));
	SceneDepth = Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessInput(GSceneRenderTargets.SceneDepthZ));
	GBufferA = Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessInput(GSceneRenderTargets.GBufferA));

	FinalOutput = FRenderingCompositeOutputRef(SceneColor);
}

static FRenderingCompositeOutputRef RenderHalfResBloomThreshold(FPostprocessContext& Context, FRenderingCompositeOutputRef SceneColorHalfRes, FRenderingCompositeOutputRef EyeAdaptation)
{
// todo: optimize later, the missing node causes some wrong behavior
//	if(Context.View.FinalPostProcessSettings.BloomIntensity <= 0.0f)
//	{
//		// this pass is not required
//		return FRenderingCompositeOutputRef();
//	}
	// bloom threshold
	FRenderingCompositePass* PostProcessBloomSetup = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessBloomSetup());
	PostProcessBloomSetup->SetInput(ePId_Input0, SceneColorHalfRes);
	PostProcessBloomSetup->SetInput(ePId_Input1, EyeAdaptation);

	return FRenderingCompositeOutputRef(PostProcessBloomSetup);
}


// 2 pass Gaussian blur using uni-linear filtering
static FRenderingCompositeOutputRef RenderGaussianBlur(
	FPostprocessContext& Context,
	EPostProcessRectSource::Type RectSource,
	const TCHAR* DebugNameX,
	const TCHAR* DebugNameY,
	const FRenderingCompositeOutputRef& Input,
	float SizeScale,
	FLinearColor Tint = FLinearColor::White,
	const FRenderingCompositeOutputRef Additive = FRenderingCompositeOutputRef())
{
	// Gaussian blur in x
	FRCPassPostProcessWeightedSampleSum* PostProcessBlurX = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessWeightedSampleSum(EFS_Horiz, EFCM_Weighted, SizeScale, RectSource, DebugNameX));
	PostProcessBlurX->SetInput(ePId_Input0, Input);

	// Gaussian blur in y
	FRCPassPostProcessWeightedSampleSum* PostProcessBlurY = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessWeightedSampleSum(EFS_Vert, EFCM_Weighted, SizeScale, RectSource, DebugNameY, Tint));
	PostProcessBlurY->SetInput(ePId_Input0, FRenderingCompositeOutputRef(PostProcessBlurX));
	PostProcessBlurY->SetInput(ePId_Input1, Additive);

	return FRenderingCompositeOutputRef(PostProcessBlurY);
}

// render one bloom pass and add another optional texture to it
static FRenderingCompositeOutputRef RenderBloom(
	FPostprocessContext& Context,
	const FRenderingCompositeOutputRef& PreviousBloom,
	float Size,
	FLinearColor Tint = FLinearColor::White,
	const FRenderingCompositeOutputRef Additive = FRenderingCompositeOutputRef())
{
	return RenderGaussianBlur(Context, EPostProcessRectSource::GBS_ViewRect, TEXT("BloomBlurX"), TEXT("BloomBlurY"), PreviousBloom, Size, Tint, Additive);
}


static FRenderingCompositeOutputRef CombineColorGradingLUTs(
	FPostprocessContext& Context)
{
	FRenderingCompositePass* Pass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessCombineLUTs());

	return FRenderingCompositeOutputRef(Pass);
}

static void AddTonemapper(
	FPostprocessContext& Context,
	const FRenderingCompositeOutputRef& BloomOutputCombined,
	const FRenderingCompositeOutputRef& EyeAdaptation,
	const FRenderingCompositeOutputRef& CombinedLUT)
{
	FRenderingCompositePass* PostProcessTonemap = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessTonemap());

	PostProcessTonemap->SetInput(ePId_Input0, Context.FinalOutput);
	PostProcessTonemap->SetInput(ePId_Input1, BloomOutputCombined);
	PostProcessTonemap->SetInput(ePId_Input2, EyeAdaptation);
	PostProcessTonemap->SetInput(ePId_Input3, CombinedLUT);

	Context.FinalOutput = FRenderingCompositeOutputRef(PostProcessTonemap);
}

static void AddSelectionOutline(FPostprocessContext& Context)
{
	FRenderingCompositePass* SelectionColorPass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessSelectionOutlineColor());
	SelectionColorPass->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Context.FinalOutput));

	FRenderingCompositePass* Node = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessSelectionOutline());
	Node->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Context.FinalOutput));
	Node->SetInput(ePId_Input1, FRenderingCompositeOutputRef(FRenderingCompositeOutputRef(SelectionColorPass)));

	Context.FinalOutput = FRenderingCompositeOutputRef(Node);
}

static void AddGammaOnlyTonemapper(FPostprocessContext& Context)
{
	FRenderingCompositePass* PostProcessTonemap = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessTonemap(true));

	PostProcessTonemap->SetInput(ePId_Input0, Context.FinalOutput);

	Context.FinalOutput = FRenderingCompositeOutputRef(PostProcessTonemap);
}

static void AddPostProcessAA(FPostprocessContext& Context)
{
	// console variable override
	static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.PostProcessAAQuality")); 

	uint32 Quality = FMath::Clamp(CVar->GetValueOnRenderThread(), 1, 6);

	FRenderingCompositePass* Node = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessAA(Quality));

	Node->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Context.FinalOutput));

	Context.FinalOutput = FRenderingCompositeOutputRef(Node);
}

static void AddSceneColorFringe(FPostprocessContext& Context)
{
	FRenderingCompositePass* Node = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessSceneColorFringe());

	Node->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Context.FinalOutput));

	Context.FinalOutput = FRenderingCompositeOutputRef(Node);
}

static FRenderingCompositeOutputRef AddPostProcessEyeAdaptation(FPostprocessContext& Context, FRenderingCompositeOutputRef& Histogram)
{
	FRenderingCompositePass* Node = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessEyeAdaptation());

	Node->SetInput(ePId_Input0, Histogram);
	return FRenderingCompositeOutputRef(Node);
}

static void AddPostProcessDepthOfFieldBokeh(FPostprocessContext& Context, FRenderingCompositeOutputRef& SeparateTranslucency)
{
	// downsample, mask out the in focus part, depth in alpha
	FRenderingCompositePass* DOFSetup = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessBokehDOFSetup());
	DOFSetup->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Context.FinalOutput));
	DOFSetup->SetInput(ePId_Input1, FRenderingCompositeOutputRef(Context.SceneDepth));

	FSceneViewState* ViewState = (FSceneViewState*)Context.View.State;
	
	FRenderingCompositePass* DOFInputPass = DOFSetup;
	if( Context.View.FinalPostProcessSettings.AntiAliasingMethod == AAM_TemporalAA && ViewState )
	{
		FRenderingCompositePass* HistoryInput;
		if( ViewState->DOFHistoryRT && ViewState->bBokehDOFHistory && !Context.View.bCameraCut )
		{
			HistoryInput = Context.Graph.RegisterPass( new(FMemStack::Get()) FRCPassPostProcessInput( ViewState->DOFHistoryRT ) );
		}
		else
		{
			// No history so use current as history
			HistoryInput = DOFSetup;
		}

		FRenderingCompositePass* NodeTemporalAA = Context.Graph.RegisterPass( new(FMemStack::Get()) FRCPassPostProcessDOFTemporalAA );
		NodeTemporalAA->SetInput( ePId_Input0, DOFSetup );
		NodeTemporalAA->SetInput( ePId_Input1, FRenderingCompositeOutputRef( HistoryInput ) );
		NodeTemporalAA->SetInput( ePId_Input2, FRenderingCompositeOutputRef( HistoryInput ) );
		//NodeTemporalAA->SetInput( ePId_Input3, VelocityInput );

		DOFInputPass = NodeTemporalAA;
		ViewState->bBokehDOFHistory = true;
	}

	FRenderingCompositePass* NodeBlurred = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessBokehDOF());
	NodeBlurred->SetInput(ePId_Input0, DOFInputPass);
	NodeBlurred->SetInput(ePId_Input1, Context.SceneColor);
	NodeBlurred->SetInput(ePId_Input2, Context.SceneDepth);

	FRenderingCompositePass* NodeRecombined = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessBokehDOFRecombine());
	NodeRecombined->SetInput(ePId_Input0, Context.FinalOutput);
	NodeRecombined->SetInput(ePId_Input1, NodeBlurred);
	NodeRecombined->SetInput(ePId_Input2, SeparateTranslucency);

	Context.FinalOutput = FRenderingCompositeOutputRef(NodeRecombined);
}


static void AddPostProcessDepthOfFieldGaussian(FPostprocessContext& Context)
{
	float FarSize = Context.View.FinalPostProcessSettings.DepthOfFieldFarBlurSize;
	float NearSize = Context.View.FinalPostProcessSettings.DepthOfFieldNearBlurSize;
	
	bool bNearBlurEnabled = NearSize >= GetCachedScalabilityCVars().GaussianDOFNearThreshold;

	if(FarSize <= 0 && NearSize <= 0)
	{
		return;
	}

	if(Context.View.Family->EngineShowFlags.VisualizeDOF)
	{
		// no need for this pass
		return;
	}

	FRenderingCompositePass* DOFSetup = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessDOFSetup(bNearBlurEnabled));
	DOFSetup->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Context.FinalOutput));
	// We need the depth to create the near and far mask
	DOFSetup->SetInput(ePId_Input1, FRenderingCompositeOutputRef(Context.SceneDepth));

	FSceneViewState* ViewState = (FSceneViewState*)Context.View.State;
	
	FRenderingCompositePass* DOFInputPass = DOFSetup;
	if( Context.View.FinalPostProcessSettings.AntiAliasingMethod == AAM_TemporalAA && ViewState )
	{
		FRenderingCompositePass* HistoryInput;
		if( ViewState->DOFHistoryRT && !ViewState->bBokehDOFHistory && !Context.View.bCameraCut )
		{
			HistoryInput = Context.Graph.RegisterPass( new(FMemStack::Get()) FRCPassPostProcessInput( ViewState->DOFHistoryRT ) );
		}
		else
		{
			// No history so use current as history
			HistoryInput = DOFSetup;
		}

		FRenderingCompositePass* NodeTemporalAA = Context.Graph.RegisterPass( new(FMemStack::Get()) FRCPassPostProcessDOFTemporalAA );
		NodeTemporalAA->SetInput( ePId_Input0, DOFSetup );
		NodeTemporalAA->SetInput( ePId_Input1, FRenderingCompositeOutputRef( HistoryInput ) );
		NodeTemporalAA->SetInput( ePId_Input2, FRenderingCompositeOutputRef( HistoryInput ) );
		//NodeTemporalAA->SetInput( ePId_Input3, VelocityInput );

		DOFInputPass = NodeTemporalAA;
		ViewState->bBokehDOFHistory = false;
	}

	FRenderingCompositeOutputRef Far(DOFInputPass, ePId_Output0);
	FRenderingCompositeOutputRef Near; // Don't need to bind a dummy here as we use a different permutation which doesn't read from this input when near DOF is disabled

	// far
	if(FarSize >= 0.01f)
	{
		Far = RenderGaussianBlur(Context, EPostProcessRectSource::GBS_ViewRect, TEXT("FarDOFBlurX"), TEXT("FarDOFBlurY"), FRenderingCompositeOutputRef(DOFInputPass, ePId_Output0), FarSize);
	}

	// near
	if(bNearBlurEnabled)
	{
		Near = RenderGaussianBlur(Context, EPostProcessRectSource::GBS_ViewRect, TEXT("NearDOFBlurX"), TEXT("NearDOFBlurY"), FRenderingCompositeOutputRef(DOFSetup, ePId_Output1), NearSize);
	}

	FRenderingCompositePass* NodeRecombined = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessDOFRecombine(bNearBlurEnabled));
	NodeRecombined->SetInput(ePId_Input0, Context.FinalOutput);
	NodeRecombined->SetInput(ePId_Input1, Far);
	NodeRecombined->SetInput(ePId_Input2, Near);

	Context.FinalOutput = FRenderingCompositeOutputRef(NodeRecombined);
}

static FRenderingCompositeOutputRef AddPostProcessHistogram(FPostprocessContext& Context, FRenderingCompositeOutputRef& HalfResScene)
{
	FRenderingCompositePass* NodeHistogram = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessHistogram());

	NodeHistogram->SetInput(ePId_Input0, HalfResScene);

	FRenderingCompositePass* NodeHistogramReduce = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessHistogramReduce());

	NodeHistogramReduce->SetInput(ePId_Input0, NodeHistogram);

	return FRenderingCompositeOutputRef(NodeHistogramReduce);
}

static FRenderingCompositeOutputRef AddBloom(FPostprocessContext Context, FRenderingCompositeOutputRef PostProcessDownsample0)
{
	// Quality level to bloom stages table. Note: 0 is omitted, ensure element count tallys with the range documented with 'r.BloomQuality' definition.
	const static uint32 BloomQualityStages[] =
	{
		3,// Q1
		3,// Q2
		4,// Q3
		4,// Q4
		5,// Q5
	};

	int32 BloomQuality;
	{
		// console variable override
		static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.BloomQuality"));
		BloomQuality = FMath::Clamp(CVar->GetValueOnRenderThread(), 0, (int32)ARRAY_COUNT(BloomQualityStages));
	}

	// Perform down sample. Used by both bloom and lens flares.
	static const int32 DownSampleStages = 5;
	FRenderingCompositeOutputRef PostProcessDownsamples[DownSampleStages] = {PostProcessDownsample0};
	for (int i = 1; i < DownSampleStages; i++)
	{
		static const TCHAR* PassLabels[] =
			{NULL, TEXT("BloomDownsample1"), TEXT("BloomDownsample2"), TEXT("BloomDownsample3"), TEXT("BloomDownsample4")};
		checkAtCompileTime(ARRAY_COUNT(PassLabels) == DownSampleStages, PassLabel_count_must_equal_DownSampleStages);
		FRenderingCompositePass* Pass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessDownsample(PF_Unknown, 1, EPostProcessRectSource::GBS_ViewRect, PassLabels[i]));
		Pass->SetInput(ePId_Input0, PostProcessDownsamples[i - 1]);
		PostProcessDownsamples[i] = FRenderingCompositeOutputRef(Pass);
	}

	FRenderingCompositeOutputRef BloomOutput;
	if (BloomQuality == 0)
	{
		// No bloom, provide substitute source for lens flare.
		BloomOutput = PostProcessDownsamples[0];
	}
	else
	{
		// Perform bloom blur + accumulate.
		struct FBloomStage
		{
			float BloomSize;
			const FLinearColor* Tint;
		};
		const FFinalPostProcessSettings& Settings = Context.View.FinalPostProcessSettings;
		FBloomStage BloomStages[] =
		{
			{ Settings.Bloom5Size, &Settings.Bloom5Tint},
			{ Settings.Bloom4Size, &Settings.Bloom4Tint},
			{ Settings.Bloom3Size, &Settings.Bloom3Tint},
			{ Settings.Bloom2Size, &Settings.Bloom2Tint},
			{ Settings.Bloom1Size, &Settings.Bloom1Tint},
		};
		static const uint32 NumBloomStages = ARRAY_COUNT(BloomStages);

		const uint32 BloomStageCount = BloomQualityStages[BloomQuality - 1];
		check(BloomStageCount <= NumBloomStages);
		float TintScale = 1.0f / NumBloomStages;
		for (uint32 i = 0, SourceIndex = NumBloomStages - 1; i < BloomStageCount; i++, SourceIndex--)
		{
			FBloomStage& Op = BloomStages[i];
			BloomOutput = RenderBloom(Context, PostProcessDownsamples[SourceIndex], Op.BloomSize, (*Op.Tint) * TintScale, BloomOutput);
		}
	}

	// Lens Flares
	FLinearColor LensFlareHDRColor = Context.View.FinalPostProcessSettings.LensFlareTint * Context.View.FinalPostProcessSettings.LensFlareIntensity;
	static const int32 MaxLensFlareQuality = 3;
	int32 LensFlareQuality;
	{
		// console variable override
		static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.LensFlareQuality"));
		LensFlareQuality = FMath::Clamp(CVar->GetValueOnRenderThread(), 0, MaxLensFlareQuality);
	}

	if (!LensFlareHDRColor.IsAlmostBlack() && LensFlareQuality > 0)
	{
		float PercentKernelSize = Context.View.FinalPostProcessSettings.LensFlareBokehSize;

		bool bLensBlur = PercentKernelSize > 0.3f;

		FRenderingCompositePass* PostProcessFlares = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessLensFlares(bLensBlur ? 2.0f : 1.0f));

		PostProcessFlares->SetInput(ePId_Input0, BloomOutput);

		FRenderingCompositeOutputRef LensFlareInput = PostProcessDownsamples[MaxLensFlareQuality - LensFlareQuality];

		if (bLensBlur)
		{
			float Threshold = Context.View.FinalPostProcessSettings.LensFlareThreshold;

			FRenderingCompositePass* PostProcessLensBlur = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessLensBlur(PercentKernelSize, Threshold));
			PostProcessLensBlur->SetInput(ePId_Input0, LensFlareInput);
			PostProcessFlares->SetInput(ePId_Input1, FRenderingCompositeOutputRef(PostProcessLensBlur));
		}
		else
		{
			// fast: no blurring or blurring shared from bloom
			PostProcessFlares->SetInput(ePId_Input1, LensFlareInput);
		}

		BloomOutput = FRenderingCompositeOutputRef(PostProcessFlares);
	}

	return BloomOutput;
}

static void AddTemporalAA( FPostprocessContext& Context, FRenderingCompositeOutputRef& VelocityInput )
{
	check(VelocityInput.IsValid());

	FSceneViewState* ViewState = (FSceneViewState*)Context.View.State;

	FRenderingCompositePass* HistoryInput;
	if( ViewState && ViewState->TemporalAAHistoryRT && !Context.View.bCameraCut )
	{
		HistoryInput = Context.Graph.RegisterPass( new(FMemStack::Get()) FRCPassPostProcessInput( ViewState->TemporalAAHistoryRT ) );
	}
	else
	{
		// No history so use current as history
		HistoryInput = Context.Graph.RegisterPass( new(FMemStack::Get()) FRCPassPostProcessInput( GSceneRenderTargets.SceneColor ) );
	}

	FRenderingCompositePass* TemporalAAPass = Context.Graph.RegisterPass( new(FMemStack::Get()) FRCPassPostProcessTemporalAA );
	TemporalAAPass->SetInput( ePId_Input0, Context.FinalOutput );
	TemporalAAPass->SetInput( ePId_Input1, FRenderingCompositeOutputRef( HistoryInput ) );
	TemporalAAPass->SetInput( ePId_Input2, FRenderingCompositeOutputRef( HistoryInput ) );
	TemporalAAPass->SetInput( ePId_Input3, VelocityInput );
	Context.FinalOutput = FRenderingCompositeOutputRef( TemporalAAPass );
}

FPostProcessMaterialNode* IteratePostProcessMaterialNodes(const FFinalPostProcessSettings& Dest, EBlendableLocation InLocation, FBlendableEntry*& Iterator)
{
	for(;;)
	{
		FPostProcessMaterialNode* DataPtr = Dest.BlendableManager.IterateBlendables<FPostProcessMaterialNode>(Iterator);

		if(!DataPtr || DataPtr->Location == InLocation)
		{
			return DataPtr;
		}
	}
}

static void AddPostProcessMaterial(FPostprocessContext& Context, EBlendableLocation InLocation, FRenderingCompositeOutputRef SeparateTranslucency)
{
	if(!Context.View.Family->EngineShowFlags.PostProcessing || !Context.View.Family->EngineShowFlags.PostProcessMaterial)
	{
		return;
	}

	FBlendableEntry* Iterator = 0;

	const uint32 MAX_PPMATERIALNODES = 10;

	FPostProcessMaterialNode* PPNodes[MAX_PPMATERIALNODES];

	uint32 PPNodeCount = 0;
	for(;PPNodeCount < MAX_PPMATERIALNODES; ++PPNodeCount)
	{
		FPostProcessMaterialNode* Data = IteratePostProcessMaterialNodes(Context.View.FinalPostProcessSettings, InLocation, Iterator);

		if(!Data)
		{
			break;
		}

		check(Data->MID);

		PPNodes[PPNodeCount] = Data;
	}
	
	::Sort(PPNodes,PPNodeCount, FPostProcessMaterialNode::FCompare());

	for(uint32 i = 0; i < PPNodeCount; ++i)
	{
		FPostProcessMaterialNode* Data = PPNodes[i];

		FRenderingCompositePass* Node = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessMaterial(Data->MID));
		Node->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Context.FinalOutput));

		// We are binding separate translucency here because the post process SceneTexture node can reference 
		// the separate translucency buffers through ePId_Input1. 
		// TODO: Check if material actually uses this texture and only bind if needed.
		Node->SetInput(ePId_Input1, SeparateTranslucency);
		Context.FinalOutput = FRenderingCompositeOutputRef(Node);
	}
}

static void AddHighResScreenshotMask(FPostprocessContext& Context, FRenderingCompositeOutputRef& SeparateTranslucencyInput)
{
	if (Context.View.Family->EngineShowFlags.HighResScreenshotMask != 0)
	{
		check(Context.View.FinalPostProcessSettings.HighResScreenshotMaterial);

		FRenderingCompositeOutputRef Input = Context.FinalOutput;

		FRenderingCompositePass* CompositePass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessMaterial(Context.View.FinalPostProcessSettings.HighResScreenshotMaterial));
		CompositePass->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Input));
		Context.FinalOutput = FRenderingCompositeOutputRef(CompositePass);

		if (GIsHighResScreenshot)
		{
			check(Context.View.FinalPostProcessSettings.HighResScreenshotMaskMaterial); 

			FRenderingCompositePass* MaskPass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessMaterial(Context.View.FinalPostProcessSettings.HighResScreenshotMaskMaterial));
			MaskPass->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Input));
			CompositePass->AddDependency(MaskPass);

			FString BaseFilename = FString(Context.View.FinalPostProcessSettings.BufferVisualizationDumpBaseFilename);
			MaskPass->SetOutputColorArray(ePId_Output0, FScreenshotRequest::GetHighresScreenshotMaskColorArray());
		}
	}

	// Draw the capture region if a material was supplied
	if (Context.View.FinalPostProcessSettings.HighResScreenshotCaptureRegionMaterial)
	{
		FRenderingCompositePass* CaptureRegionVisualizationPass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessMaterial(Context.View.FinalPostProcessSettings.HighResScreenshotCaptureRegionMaterial));
		CaptureRegionVisualizationPass->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Context.FinalOutput));
		Context.FinalOutput = FRenderingCompositeOutputRef(CaptureRegionVisualizationPass);
	}
}

static void AddGBufferVisualization(FPostprocessContext& Context, FRenderingCompositeOutputRef& SeparateTranslucencyInput)
{
	static const auto CVarDumpFrames = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.BufferVisualizationDumpFrames"));
	bool bVisualizationEnabled = Context.View.Family->EngineShowFlags.VisualizeBuffer;
	bool bOverviewModeEnabled = bVisualizationEnabled && (Context.View.CurrentBufferVisualizationMode == NAME_None);
	bool bHighResBufferVisualizationDumpRequried = GIsHighResScreenshot && GetHighResScreenshotConfig().bDumpBufferVisualizationTargets;
	bool bDumpFrames = Context.View.FinalPostProcessSettings.bBufferVisualizationDumpRequired && (CVarDumpFrames->GetValueOnRenderThread() || bHighResBufferVisualizationDumpRequried);

	FString BaseFilename;

	if (bDumpFrames)
	{
		BaseFilename = FString(Context.View.FinalPostProcessSettings.BufferVisualizationDumpBaseFilename);
	}
	
	if (bDumpFrames || bVisualizationEnabled)
	{	
		FRenderingCompositeOutputRef IncomingStage = Context.FinalOutput;

		if (bDumpFrames || bOverviewModeEnabled)
		{
			FRenderingCompositePass* CompositePass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessVisualizeBuffer());
			CompositePass->SetInput(ePId_Input0, FRenderingCompositeOutputRef(IncomingStage));
			Context.FinalOutput = FRenderingCompositeOutputRef(CompositePass);

			// Loop over materials, creating stages for generation and downsampling of the tiles.			
			for (TArray<UMaterialInterface*>::TConstIterator It = Context.View.FinalPostProcessSettings.BufferVisualizationOverviewMaterials.CreateConstIterator(); It; ++It)
			{
				if (*It)
				{
					// Apply requested material
					FRenderingCompositePass* MaterialPass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessMaterial(*It));
					MaterialPass->SetInput(ePId_Input0, FRenderingCompositeOutputRef(IncomingStage));
					MaterialPass->SetInput(ePId_Input1, FRenderingCompositeOutputRef(SeparateTranslucencyInput));

					if (BaseFilename.Len())
					{
						FString MaterialFilename = BaseFilename + TEXT("_") + (*It)->GetName();
						MaterialPass->SetOutputDumpFilename(ePId_Output0, *MaterialFilename);
					}

					// If the overview mode is activated, downsample the material pass to quarter size
					if (bOverviewModeEnabled)
					{
						// Down-sample to 1/2 size
						FRenderingCompositePass* HalfSize = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessDownsample(PF_Unknown, 0, EPostProcessRectSource::GBS_ViewRect, TEXT("MaterialHalfSize")));
						HalfSize->SetInput(ePId_Input0, FRenderingCompositeOutputRef(MaterialPass));

						// Down-sample to 1/4 size
						FRenderingCompositePass* QuarterSize = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessDownsample(PF_Unknown, 0, EPostProcessRectSource::GBS_ViewRect, TEXT("MaterialQuarterSize")));
						QuarterSize->SetInput(ePId_Input0, FRenderingCompositeOutputRef(HalfSize));

						// Mark the quarter size target as the dependency for the composite pass
						((FRCPassPostProcessVisualizeBuffer*)CompositePass)->AddVisualizationBuffer(FRenderingCompositeOutputRef(QuarterSize), (*It)->GetName());
					}
					else
					{
						// We are just dumping the frames, so the material pass is the dependency of the composite
						CompositePass->AddDependency(MaterialPass);
					}
				}
				else
				{
					if (bOverviewModeEnabled)
					{
						((FRCPassPostProcessVisualizeBuffer*)CompositePass)->AddVisualizationBuffer(FRenderingCompositeOutputRef(), FString());
					}
				}
			}
		}
		else if (bVisualizationEnabled && !bOverviewModeEnabled)
		{
			// Apply requested material to the full screen
			UMaterial* Material = GetBufferVisualizationData().GetMaterial(Context.View.CurrentBufferVisualizationMode);
			check(Material);
			FRenderingCompositePass* MaterialPass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessMaterial(Material));
			MaterialPass->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Context.FinalOutput));
			MaterialPass->SetInput(ePId_Input1, FRenderingCompositeOutputRef(SeparateTranslucencyInput));
			Context.FinalOutput = FRenderingCompositeOutputRef(MaterialPass);
		}
	}
}

static void AddUIBlur(FPostprocessContext& Context)
{
	if (Context.View.UIBlurOverrideRectangles.Num() == 0)
	{
		return;
	}
	FRenderingCompositePass* HalfResPass = Context.Graph.RegisterPass(new FRCPassPostProcessDownsample(PF_B8G8R8A8, 1, EPostProcessRectSource::GBS_ViewRect, TEXT("SceneColorHalfRes")));
	HalfResPass->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Context.FinalOutput));

	FRenderingCompositePass* QuarterResPass = Context.Graph.RegisterPass(new FRCPassPostProcessDownsample(PF_B8G8R8A8, 1, EPostProcessRectSource::GBS_ViewRect, TEXT("SceneColorQuarterRes")));
	QuarterResPass->SetInput(ePId_Input0, FRenderingCompositeOutputRef(HalfResPass));

	static auto* ICVar = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("UI.BlurRadius"));
	float BlurRadius = ICVar->GetValueOnRenderThread();

	FRenderingCompositeOutputRef PostProcessBlur = QuarterResPass;
	if (BlurRadius > 0)
	{
		PostProcessBlur = RenderGaussianBlur(Context, EPostProcessRectSource::GBS_ViewRect, TEXT("UIBlurX"), TEXT("UIBlurY"), QuarterResPass, BlurRadius);
	}

	// Apply blurry content to the selected areas
	FRenderingCompositePass* PostProcessUIBlur = Context.Graph.RegisterPass(new FRCPassPostProcessUIBlur(*Context.FinalOutput.GetOutput()));
	PostProcessUIBlur->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Context.FinalOutput));
	PostProcessUIBlur->SetInput(ePId_Input1, PostProcessBlur);

	Context.FinalOutput = FRenderingCompositeOutputRef(PostProcessUIBlur);
}

// could be moved into the graph
// allows for Framebuffer blending optimization with the composition graph
void OverrideRenderTarget(FRenderingCompositeOutputRef It, TRefCountPtr<IPooledRenderTarget>& RT, FPooledRenderTargetDesc& Desc)
{
	for(;;)
	{
		It.GetOutput()->PooledRenderTarget = RT;
		It.GetOutput()->RenderTargetDesc = Desc;

		if(!It.GetPass()->FrameBufferBlendingWithInput0())
		{
			break;
		}

		It = *It.GetPass()->GetInput(ePId_Input0);
	}
}

void FPostProcessing::Process(const FViewInfo& View, TRefCountPtr<IPooledRenderTarget>& VelocityRT)
{
	QUICK_SCOPE_CYCLE_COUNTER( STAT_PostProcessing_Process );

	check(IsInRenderingThread());

	bool bSimpleDynamicLighting = IsSimpleDynamicLightingEnabled();


	// This page: https://udn.epicgames.com/Three/RenderingOverview#Rendering%20state%20defaults 
	// describes what state a pass can expect and to what state it need to be set back.

	// All post processing is happening on the render thread side. All passes can access FinalPostProcessSettings and all
	// view settings. Those are copies for the RT then never get access by the main thread again.
	// Pointers to other structures might be unsafe to touch.


	// so that the passes can register themselves to the graph
	{
		FMemMark Mark(FMemStack::Get());
		FRenderingCompositePassContext CompositeContext(View);

		FPostprocessContext Context(CompositeContext.Graph, View);
		
		// not always valid
		FRenderingCompositeOutputRef HDRColor; 
		// not always valid
		FRenderingCompositeOutputRef Histogram;
		// not always valid
		FRenderingCompositeOutputRef EyeAdaptation;
		// not always valid
		FRenderingCompositeOutputRef SeparateTranslucency;

		bool bAllowTonemapper = true;
		EStereoscopicPass StereoPass = View.StereoPass;

		FSceneViewState* ViewState = (FSceneViewState*)Context.View.State;

		{
			if(ViewState && ViewState->SeparateTranslucencyRT)
			{
				FRenderingCompositePass* NodeSeparateTranslucency = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessInput(ViewState->SeparateTranslucencyRT));

				SeparateTranslucency = FRenderingCompositeOutputRef(NodeSeparateTranslucency);
				// the node keeps another reference so the RT will not be release too early
				ViewState->FreeSeparateTranslucency();

				check(!ViewState->SeparateTranslucencyRT);
			}
		}
		
		// add the passes we want to add to the graph (commenting a line means the pass is not inserted into the graph) ---------

		if( View.Family->EngineShowFlags.PostProcessing && GRHIFeatureLevel >= ERHIFeatureLevel::SM4 )
		{
			// Screen Space Subsurface Scattering
			{
				static const auto ICVar = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.SSSSS"));
				float Radius = ICVar->GetValueOnRenderThread();

				if(Radius > 0 && !bSimpleDynamicLighting)
				{
					FRenderingCompositePass* PassSetup = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessSubsurfaceSetup());
					PassSetup->SetInput(ePId_Input0, Context.FinalOutput);

					FRenderingCompositePass* Pass0 = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessSubsurface(0, Radius));
					Pass0->SetInput(ePId_Input0, PassSetup);

					FRenderingCompositePass* Pass1 = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessSubsurface(1, Radius));
					Pass1->SetInput(ePId_Input0, Pass0);
					Pass1->SetInput(ePId_Input1, Context.FinalOutput);
					Context.FinalOutput = FRenderingCompositeOutputRef(Pass1);
				}
			}

			FRenderingCompositeOutputRef VelocityInput;
			if(VelocityRT)
			{
				VelocityInput = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessInputSingleUse(VelocityRT));
				// the graph keeps a reference until it's no longer needed
				VelocityRT.SafeRelease();
			}
			
			if( Context.View.FinalPostProcessSettings.AntiAliasingMethod != AAM_TemporalAA && ViewState )			
			{
				if(ViewState->DOFHistoryRT)
				{
					ViewState->DOFHistoryRT.SafeRelease();
				}
				if(ViewState->TemporalAAHistoryRT)
				{
					ViewState->TemporalAAHistoryRT.SafeRelease();
				}
			}

			AddPostProcessMaterial(Context, BL_BeforeTranslucency, SeparateTranslucency);

			static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.DepthOfFieldQuality"));
			check(CVar)
			bool bDepthOfField = View.Family->EngineShowFlags.DepthOfField && CVar->GetValueOnRenderThread() > 0;

			if(bDepthOfField && View.FinalPostProcessSettings.DepthOfFieldMethod != DOFM_BokehDOF)
			{
				AddPostProcessDepthOfFieldGaussian(Context);
			}

			bool bBokehDOF = bDepthOfField
				&& View.FinalPostProcessSettings.DepthOfFieldScale > 0
				&& View.FinalPostProcessSettings.DepthOfFieldMethod == DOFM_BokehDOF
				&& !Context.View.Family->EngineShowFlags.VisualizeDOF;

			if(bBokehDOF)
			{
				AddPostProcessDepthOfFieldBokeh(Context, SeparateTranslucency);
			}
			else 
			{
				if(SeparateTranslucency.IsValid())
				{
					// separate translucency is done here or in AddPostProcessDepthOfFieldBokeh()
					FRenderingCompositePass* NodeRecombined = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessBokehDOFRecombine());
					NodeRecombined->SetInput(ePId_Input0, Context.FinalOutput);
					NodeRecombined->SetInput(ePId_Input2, SeparateTranslucency);

					Context.FinalOutput = FRenderingCompositeOutputRef(NodeRecombined);
				}
			}

			AddPostProcessMaterial(Context, BL_BeforeTonemapping, SeparateTranslucency);

			EAntiAliasingMethod AntiAliasingMethod = Context.View.FinalPostProcessSettings.AntiAliasingMethod;

			if( AntiAliasingMethod == AAM_TemporalAA && ViewState)
			{
				if(VelocityInput.IsValid())
				{
					AddTemporalAA( Context, VelocityInput );
				}
				else
				{
					// black is how we clear the velocity buffer so this means no velocity
					FRenderingCompositePass* NoVelocity = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessInput(GSystemTextures.BlackDummy));
					FRenderingCompositeOutputRef NoVelocityRef(NoVelocity);
					AddTemporalAA( Context, NoVelocityRef );
				}
			}

			if(IsMotionBlurEnabled(View) && VelocityInput.IsValid())
			{
				// Motionblur is enabled
				FRenderingCompositeOutputRef SoftEdgeVelocity;

				FRenderingCompositeOutputRef MotionBlurHalfVelocity;
				FRenderingCompositeOutputRef MotionBlurColorDepth;

				// down sample screen space velocity and extend outside of borders to allows soft edge motion blur
				{
					// Down sample and prepare for soft masked blurring
					FRenderingCompositePass* HalfResVelocity = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessMotionBlurSetup());
					HalfResVelocity->SetInput(ePId_Input0, VelocityInput);
					HalfResVelocity->SetInput(ePId_Input1, FRenderingCompositeOutputRef(Context.FinalOutput));
					// only for Depth of Field we need the depth in the alpha channel
					HalfResVelocity->SetInput(ePId_Input2, FRenderingCompositeOutputRef(Context.SceneDepth));
					
					MotionBlurHalfVelocity = FRenderingCompositeOutputRef(HalfResVelocity, ePId_Output0);
					MotionBlurColorDepth = FRenderingCompositeOutputRef(HalfResVelocity, ePId_Output1);

					FRenderingCompositePass* QuarterResVelocity = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessDownsample(PF_Unknown, 0, EPostProcessRectSource::GBS_ViewRect, TEXT("QuarterResVelocity")));
					QuarterResVelocity->SetInput(ePId_Input0, HalfResVelocity);

					SoftEdgeVelocity = FRenderingCompositeOutputRef(QuarterResVelocity);

					float MotionBlurSoftEdgeSize;
					{
						static const auto ICVar = IConsoleManager::Get().FindTConsoleVariableDataFloat(TEXT("r.MotionBlurSoftEdgeSize"));

						MotionBlurSoftEdgeSize = ICVar->GetValueOnRenderThread();
					}

					if(MotionBlurSoftEdgeSize > 0.01f)
					{
						SoftEdgeVelocity = RenderGaussianBlur(Context, EPostProcessRectSource::GBS_ViewRect, TEXT("VelocityBlurX"), TEXT("VelocityBlurY"), QuarterResVelocity, MotionBlurSoftEdgeSize);
					}
				}

				// doing the actual motion blur sampling, alpha to mask out the blurred areas
				FRenderingCompositePass* MotionBlurPass;

				if(View.Family->EngineShowFlags.VisualizeMotionBlur)
				{
					MotionBlurPass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessVisualizeMotionBlur());
					bAllowTonemapper = false;
				}
				else
				{
					int32 MotionBlurQuality = GetMotionBlurQualityFromCVar();

					MotionBlurPass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessMotionBlur(MotionBlurQuality));
				}

				MotionBlurPass->SetInput(ePId_Input0, MotionBlurColorDepth);

				if(VelocityInput.IsValid())
				{
					// blurred screen space velocity for soft masked motion blur
					MotionBlurPass->SetInput(ePId_Input1, SoftEdgeVelocity);
					// screen space velocity input from per object velocity rendering
					MotionBlurPass->SetInput(ePId_Input2, MotionBlurHalfVelocity);
				}

				FRenderingCompositePass* MotionBlurRecombinePass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessMotionBlurRecombine());
				MotionBlurRecombinePass->SetInput(ePId_Input0, Context.FinalOutput);
				MotionBlurRecombinePass->SetInput(ePId_Input1, FRenderingCompositeOutputRef(MotionBlurPass));
				Context.FinalOutput = FRenderingCompositeOutputRef(MotionBlurRecombinePass);
			}

			// down sample Scene color from full to half res
			FRenderingCompositeOutputRef SceneColorHalfRes;
			{
				// doesn't have to be as high quality as the Scene color
				FRenderingCompositePass* HalfResPass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessDownsample(PF_FloatRGB, 1, EPostProcessRectSource::GBS_ViewRect, TEXT("SceneColorHalfRes")));
				HalfResPass->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Context.FinalOutput));

				SceneColorHalfRes = FRenderingCompositeOutputRef(HalfResPass);
			}

			{
				bool bHistogramNeeded = View.Family->EngineShowFlags.VisualizeHDR;

				if(View.Family->EngineShowFlags.EyeAdaptation
					&& View.FinalPostProcessSettings.AutoExposureMinBrightness < View.FinalPostProcessSettings.AutoExposureMaxBrightness
					&& !View.bIsSceneCapture) // Eye adaption is not available for scene captures.
				{
					bHistogramNeeded = true;
				}

				if(!bAllowTonemapper)
				{
					bHistogramNeeded = false;
				}

				if(!GIsHighResScreenshot && bHistogramNeeded && GRHIFeatureLevel == ERHIFeatureLevel::SM5 && StereoPass != eSSP_RIGHT_EYE)
				{
					Histogram = AddPostProcessHistogram(Context, SceneColorHalfRes);
				}
			}

			// some views don't have a state (thumbnail rendering)
			if(!GIsHighResScreenshot && View.State && (StereoPass != eSSP_RIGHT_EYE) && !View.bIsSceneCapture)
			{
				// we always add eye adaptation, if the engine show flag is disabled we set the ExposureScale in the texture to a fixed value
				EyeAdaptation = AddPostProcessEyeAdaptation(Context, Histogram);
			}

			// optional
			FRenderingCompositeOutputRef BloomOutputCombined;

			if(View.Family->EngineShowFlags.Bloom)
			{
				FRenderingCompositeOutputRef HalfResBloomThreshold = RenderHalfResBloomThreshold(Context, SceneColorHalfRes, EyeAdaptation);

				BloomOutputCombined = AddBloom(Context, HalfResBloomThreshold);
			}

			HDRColor = Context.FinalOutput;

			if(bAllowTonemapper && GRHIFeatureLevel >= ERHIFeatureLevel::SM4)
			{
				FRenderingCompositeOutputRef CombinedLUT;

				CombinedLUT = CombineColorGradingLUTs(Context);

				AddTonemapper(Context, BloomOutputCombined, EyeAdaptation, CombinedLUT);
			}

			if(AntiAliasingMethod == AAM_FXAA || (AntiAliasingMethod == AAM_TemporalAA && View.bCameraCut))
			{
				AddPostProcessAA(Context);
			}
			
			{
				static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.SceneColorFringeQuality")); 

				int32 FringeQuality = CVar->GetValueOnRenderThread();

				if(View.Family->EngineShowFlags.SceneColorFringe
					&& View.Family->EngineShowFlags.CameraImperfections
					&& View.FinalPostProcessSettings.SceneFringeIntensity > 0.01f
					&& FringeQuality > 0)
				{
					AddSceneColorFringe(Context);
				}
			}

			if(bDepthOfField && Context.View.Family->EngineShowFlags.VisualizeDOF)
			{
				FRenderingCompositePass* VisualizeNode = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessVisualizeDOF());
				VisualizeNode->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Context.FinalOutput));

				// PassThrough is needed to upscale the half res texture
				FPooledRenderTargetDesc Desc = GSceneRenderTargets.SceneColor->GetDesc();
				Desc.Format = PF_B8G8R8A8;
				FRenderingCompositePass* NullPass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessPassThrough(Desc));
				NullPass->SetInput(ePId_Input0, FRenderingCompositeOutputRef(VisualizeNode));
				Context.FinalOutput = FRenderingCompositeOutputRef(NullPass);
			}
		}
		else
		{
			if(ViewState)
			{
				check(!ViewState->SeparateTranslucencyRT);
			}

			AddGammaOnlyTonemapper(Context);
		}
		
		if(View.Family->EngineShowFlags.StationaryLightOverlap)
		{
			FRenderingCompositePass* Node = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessVisualizeComplexity(GEngine->StationaryLightOverlapColors));
			Node->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Context.FinalOutput));
			Context.FinalOutput = FRenderingCompositeOutputRef(Node);
		}

		if(View.Family->EngineShowFlags.ShaderComplexity)
		{
			FRenderingCompositePass* Node = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessVisualizeComplexity(GEngine->ShaderComplexityColors));
			Node->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Context.FinalOutput));
			Context.FinalOutput = FRenderingCompositeOutputRef(Node);
		}


		// Show the selection outline if it is in the editor and we arent in wireframe 
		// If the engine is in demo mode and game view is on we also do not show the selection outline
		if ( GIsEditor && View.Family->EngineShowFlags.SelectionOutline && !(View.Family->EngineShowFlags.Wireframe) && ( !GIsDemoMode || ( GIsDemoMode && !View.Family->EngineShowFlags.Game ) ) )
		{
			// Selection outline is after bloom, but before AA
			AddSelectionOutline(Context);
		}

		// Composite editor primitives if we had any to draw and compositing is enabled
		if( FDeferredShadingSceneRenderer::ShouldCompositeEditorPrimitives(View) )
		{
			FRenderingCompositePass* Node = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessCompositeEditorPrimitives());
			Node->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Context.FinalOutput));
			//Node->SetInput(ePId_Input1, FRenderingCompositeOutputRef(Context.SceneDepth));
			Context.FinalOutput = FRenderingCompositeOutputRef(Node);
		}

		if(View.Family->EngineShowFlags.GBufferHints)
		{
			FRenderingCompositePass* Node = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessGBufferHints());
			Node->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Context.FinalOutput));
			// Ideally without lighting as we want the emissive, we should do that later.
			Node->SetInput(ePId_Input1, FRenderingCompositeOutputRef(Context.SceneColor));
			Context.FinalOutput = FRenderingCompositeOutputRef(Node);
		}
		
		AddPostProcessMaterial(Context, BL_AfterTonemapping, SeparateTranslucency);

		AddGBufferVisualization(Context, SeparateTranslucency);

		if (GEngine->HMDDevice.IsValid() && GEngine->IsStereoscopic3D() && View.Family->EngineShowFlags.HMDDistortion)
		{
			FRenderingCompositePass* Node = Context.Graph.RegisterPass(new FRCPassPostProcessHMD());
			Node->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Context.FinalOutput));
			Context.FinalOutput = FRenderingCompositeOutputRef(Node);
		}

		if(View.Family->EngineShowFlags.VisualizeHDR && GRHIFeatureLevel >= ERHIFeatureLevel::SM5)
		{
			FRenderingCompositePass* Node = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessVisualizeHDR());
			Node->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Context.FinalOutput));
			Node->SetInput(ePId_Input1, Histogram);
			Node->SetInput(ePId_Input2, HDRColor);
			Context.FinalOutput = FRenderingCompositeOutputRef(Node);
		}

		AddUIBlur(Context);

		if(View.Family->EngineShowFlags.TestImage && GRHIFeatureLevel >= ERHIFeatureLevel::SM5)
		{
			FRenderingCompositePass* Node = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessTestImage());
			Node->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Context.FinalOutput));
			Context.FinalOutput = FRenderingCompositeOutputRef(Node);
		}

		AddHighResScreenshotMask(Context, SeparateTranslucency);

		if((View.UnscaledViewRect != View.ViewRect) && (!(View.Family->EngineShowFlags.HMDDistortion) || !(GEngine->HMDDevice.IsValid() && GEngine->IsStereoscopic3D())))
		{
			static const auto CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.UpsampleQuality")); 
			int32 UpsampleMethod = CVar->GetValueOnRenderThread();
			UpsampleMethod = FMath::Clamp(UpsampleMethod, 0, 3);
			FRenderingCompositePass* Node = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessUpscale(UpsampleMethod));
			Node->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Context.FinalOutput)); // Bilinear sampling.
			Node->SetInput(ePId_Input1, FRenderingCompositeOutputRef(Context.FinalOutput)); // Point sampling.
			Context.FinalOutput = FRenderingCompositeOutputRef(Node);
		}

		// The graph setup should be finished before this line ----------------------------------------

		{
			// currently created on the heap each frame but View.Family->RenderTarget could keep this object and all would be cleaner
			TRefCountPtr<IPooledRenderTarget> Temp;
			FSceneRenderTargetItem Item;
			Item.TargetableTexture = (FTextureRHIRef&)View.Family->RenderTarget->GetRenderTargetTexture();
			Item.ShaderResourceTexture = (FTextureRHIRef&)View.Family->RenderTarget->GetRenderTargetTexture();

			FPooledRenderTargetDesc Desc;

			Desc.Extent = View.Family->RenderTarget->GetSizeXY();
			// todo: this should come from View.Family->RenderTarget
			Desc.Format = PF_B8G8R8A8;
			Desc.NumMips = 1;

			GRenderTargetPool.CreateUntrackedElement(Desc, Temp, Item);

			OverrideRenderTarget(Context.FinalOutput, Temp, Desc);

			// you can add multiple dependencies
			CompositeContext.Root->AddDependency(Context.FinalOutput);

			CompositeContext.Process(TEXT("PostProcessing"));
		}
	}
}


void FPostProcessing::ProcessES2( const FViewInfo& View, bool bUsedFramebufferFetch )
{
	check(IsInRenderingThread());

	// This page: https://udn.epicgames.com/Three/RenderingOverview#Rendering%20state%20defaults 
	// describes what state a pass can expect and to what state it need to be set back.

	// All post processing is happening on the render thread side. All passes can access FinalPostProcessSettings and all
	// view settings. Those are copies for the RT then never get access by the main thread again.
	// Pointers to other structures might be unsafe to touch.


	// so that the passes can register themselves to the graph
	{
		FMemMark Mark(FMemStack::Get());
		FRenderingCompositePassContext CompositeContext(View);

		FPostprocessContext Context(CompositeContext.Graph, View);
		FRenderingCompositeOutputRef BloomOutput;
		FRenderingCompositeOutputRef DofOutput;

		bool bUseAa = View.FinalPostProcessSettings.AntiAliasingMethod == AAM_TemporalAA;

		// AA with Mobile32bpp mode requires this outside of bUsePost.
		if(bUseAa)
		{
			// Handle pointer swap for double buffering.
			FSceneViewState* ViewState = (FSceneViewState*)View.State;
			if(ViewState)
			{
				// Note that this drops references to the render targets from two frames ago. This
				// causes them to be added back to the pool where we can grab them again.
				ViewState->MobileAaBloomSunVignette1 = ViewState->MobileAaBloomSunVignette0;
				ViewState->MobileAaColor1 = ViewState->MobileAaColor0;
			}
		}

		// add the passes we want to add to the graph (commenting a line means the pass is not inserted into the graph) ---------
		if( View.Family->EngineShowFlags.PostProcessing )
		{
			bool bUseSun = View.bLightShaftUse;
			bool bUseDof = View.FinalPostProcessSettings.DepthOfFieldScale > 0.0f;
			bool bUseBloom = View.FinalPostProcessSettings.BloomIntensity > 0.0f;
			bool bUseVignette = View.FinalPostProcessSettings.VignetteIntensity > 0.0f;

			bool bWorkaround = CVarRenderTargetSwitchWorkaround.GetValueOnRenderThread() != 0;

			// This is a workaround to avoid a performance cliff when using many render targets. 
			bool bUseBloomSmall = bUseBloom && !bUseSun && !bUseDof && bWorkaround;

			bool bUsePost = bUseSun | bUseDof | bUseBloom | bUseVignette;

			// Post is only supported on ES2 devices with FP16.
			bUsePost &= GSupportsRenderTargetFormat_PF_FloatRGBA;
			bUsePost &= IsMobileHDR();

			FIntPoint PrePostSourceViewportSize = View.ViewRect.Size();

			if(bUsePost)
			{
				// Skip this pass if the pass was done prior before resolve.
				if((!bUsedFramebufferFetch) && (bUseSun || bUseDof))
				{
					// Convert depth to {circle of confusion, sun shaft intensity} before resolve.
					FRenderingCompositePass* PostProcessSunMask = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessSunMaskES2(PrePostSourceViewportSize, false));
					PostProcessSunMask->SetInput(ePId_Input0, Context.FinalOutput);
					Context.FinalOutput = FRenderingCompositeOutputRef(PostProcessSunMask);
				}

				FRenderingCompositeOutputRef PostProcessBloomSetup;
				if(bUseSun || bUseDof || bUseBloom)
				{
					if(bUseBloomSmall)
					{
						FRenderingCompositePass* Pass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessBloomSetupSmallES2(PrePostSourceViewportSize, bUsedFramebufferFetch));
						Pass->SetInput(ePId_Input0, Context.FinalOutput);
						PostProcessBloomSetup = FRenderingCompositeOutputRef(Pass);
					}
					else
					{
						FRenderingCompositePass* Pass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessBloomSetupES2(PrePostSourceViewportSize, bUsedFramebufferFetch));
						Pass->SetInput(ePId_Input0, Context.FinalOutput);
						PostProcessBloomSetup = FRenderingCompositeOutputRef(Pass);
					}
				}

				if(bUseDof) 
				{
					// Near dilation circle of confusion size.
					// Samples at 1/16 area, writes to 1/16 area.
					FRenderingCompositeOutputRef PostProcessNear;
					{
						FRenderingCompositePass* Pass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessDofNearES2(PrePostSourceViewportSize));
						Pass->SetInput(ePId_Input0, PostProcessBloomSetup);
						PostProcessNear = FRenderingCompositeOutputRef(Pass);
					}

					// DOF downsample pass.
					// Samples at full resolution, writes to 1/4 area.
					FRenderingCompositeOutputRef PostProcessDofDown;
					{
						FRenderingCompositePass* Pass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessDofDownES2(PrePostSourceViewportSize, bUsedFramebufferFetch));
						Pass->SetInput(ePId_Input0, Context.FinalOutput);
						Pass->SetInput(ePId_Input1, PostProcessNear);
						PostProcessDofDown = FRenderingCompositeOutputRef(Pass);
					}

					// DOF blur pass.
					// Samples at 1/4 area, writes to 1/4 area.
					FRenderingCompositeOutputRef PostProcessDofBlur;
					{
						FRenderingCompositePass* Pass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessDofBlurES2(PrePostSourceViewportSize));
						Pass->SetInput(ePId_Input0, PostProcessDofDown);
						Pass->SetInput(ePId_Input1, PostProcessNear);
						PostProcessDofBlur = FRenderingCompositeOutputRef(Pass);
						DofOutput = PostProcessDofBlur;
					}
				}

				// Bloom.
				FRenderingCompositeOutputRef PostProcessDownsample2;
				FRenderingCompositeOutputRef PostProcessDownsample3;
				FRenderingCompositeOutputRef PostProcessDownsample4;
				FRenderingCompositeOutputRef PostProcessDownsample5;
				FRenderingCompositeOutputRef PostProcessUpsample4;
				FRenderingCompositeOutputRef PostProcessUpsample3;
				FRenderingCompositeOutputRef PostProcessUpsample2;

				if(bUseBloomSmall)
				{
					float DownScale = 0.66f * 4.0f;
					// Downsample by 2
					{
						FRenderingCompositePass* Pass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessBloomDownES2(PrePostSourceViewportSize/4, DownScale * 2.0f));
						Pass->SetInput(ePId_Input0, PostProcessBloomSetup);
						PostProcessDownsample2 = FRenderingCompositeOutputRef(Pass);
					}
				}

				if(bUseBloom && (!bUseBloomSmall))
				{
					float DownScale = 0.66f * 4.0f;
					// Downsample by 2
					{
						FRenderingCompositePass* Pass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessBloomDownES2(PrePostSourceViewportSize/4, DownScale));
						Pass->SetInput(ePId_Input0, PostProcessBloomSetup);
						PostProcessDownsample2 = FRenderingCompositeOutputRef(Pass);
					}

					// Downsample by 2
					{
						FRenderingCompositePass* Pass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessBloomDownES2(PrePostSourceViewportSize/8, DownScale));
						Pass->SetInput(ePId_Input0, PostProcessDownsample2);
						PostProcessDownsample3 = FRenderingCompositeOutputRef(Pass);
					}

					// Downsample by 2
					{
						FRenderingCompositePass* Pass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessBloomDownES2(PrePostSourceViewportSize/16, DownScale));
						Pass->SetInput(ePId_Input0, PostProcessDownsample3);
						PostProcessDownsample4 = FRenderingCompositeOutputRef(Pass);
					}

					// Downsample by 2
					{
						FRenderingCompositePass* Pass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessBloomDownES2(PrePostSourceViewportSize/32, DownScale));
						Pass->SetInput(ePId_Input0, PostProcessDownsample4);
						PostProcessDownsample5 = FRenderingCompositeOutputRef(Pass);
					}

					const FFinalPostProcessSettings& Settings = Context.View.FinalPostProcessSettings;

					float UpScale = 0.66f * 2.0f;
					// Upsample by 2
					{
						FVector4 TintA = FVector4(Settings.Bloom4Tint.R, Settings.Bloom4Tint.G, Settings.Bloom4Tint.B, 0.0f);
						FVector4 TintB = FVector4(Settings.Bloom5Tint.R, Settings.Bloom5Tint.G, Settings.Bloom5Tint.B, 0.0f);
						TintA *= View.FinalPostProcessSettings.BloomIntensity;
						TintB *= View.FinalPostProcessSettings.BloomIntensity;
						FRenderingCompositePass* Pass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessBloomUpES2(PrePostSourceViewportSize/32, FVector2D(UpScale, UpScale), TintA, TintB));
						Pass->SetInput(ePId_Input0, PostProcessDownsample4);
						Pass->SetInput(ePId_Input1, PostProcessDownsample5);
						PostProcessUpsample4 = FRenderingCompositeOutputRef(Pass);
					}

					// Upsample by 2
					{
						FVector4 TintA = FVector4(Settings.Bloom3Tint.R, Settings.Bloom3Tint.G, Settings.Bloom3Tint.B, 0.0f);
						TintA *= View.FinalPostProcessSettings.BloomIntensity;
						FVector4 TintB = FVector4(1.0f, 1.0f, 1.0f, 0.0f);
						FRenderingCompositePass* Pass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessBloomUpES2(PrePostSourceViewportSize/16, FVector2D(UpScale, UpScale), TintA, TintB));
						Pass->SetInput(ePId_Input0, PostProcessDownsample3);
						Pass->SetInput(ePId_Input1, PostProcessUpsample4);
						PostProcessUpsample3 = FRenderingCompositeOutputRef(Pass);
					}

					// Upsample by 2
					{
						FVector4 TintA = FVector4(Settings.Bloom2Tint.R, Settings.Bloom2Tint.G, Settings.Bloom2Tint.B, 0.0f);
						TintA *= View.FinalPostProcessSettings.BloomIntensity;
						// Scaling Bloom2 by extra factor to match filter area difference between PC default and mobile.
						TintA *= 0.5;
						FVector4 TintB = FVector4(1.0f, 1.0f, 1.0f, 0.0f);
						FRenderingCompositePass* Pass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessBloomUpES2(PrePostSourceViewportSize/8, FVector2D(UpScale, UpScale), TintA, TintB));
						Pass->SetInput(ePId_Input0, PostProcessDownsample2);
						Pass->SetInput(ePId_Input1, PostProcessUpsample3);
						PostProcessUpsample2 = FRenderingCompositeOutputRef(Pass);
					}
				}

				FRenderingCompositeOutputRef PostProcessSunBlur;
				if(bUseSun)
				{
					// Sunshaft depth blur using downsampled alpha.
					FRenderingCompositeOutputRef PostProcessSunAlpha;
					{
						FRenderingCompositePass* Pass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessSunAlphaES2(PrePostSourceViewportSize));
						Pass->SetInput(ePId_Input0, PostProcessBloomSetup);
						PostProcessSunAlpha = FRenderingCompositeOutputRef(Pass);
					}

					// Sunshaft blur number two.
					{
						FRenderingCompositePass* Pass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessSunBlurES2(PrePostSourceViewportSize));
						Pass->SetInput(ePId_Input0, PostProcessSunAlpha);
						PostProcessSunBlur = FRenderingCompositeOutputRef(Pass);
					}
				}

				if(bUseSun | bUseVignette | bUseBloom)
				{
					FRenderingCompositeOutputRef PostProcessSunMerge;
					if(bUseBloomSmall) 
					{
						FRenderingCompositePass* Pass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessSunMergeSmallES2(PrePostSourceViewportSize));
						Pass->SetInput(ePId_Input0, PostProcessBloomSetup);
						Pass->SetInput(ePId_Input1, PostProcessDownsample2);
						PostProcessSunMerge = FRenderingCompositeOutputRef(Pass);
						BloomOutput = PostProcessSunMerge;
					}
					else
					{
						FRenderingCompositePass* Pass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessSunMergeES2(PrePostSourceViewportSize));
						if(bUseSun)
						{
							Pass->SetInput(ePId_Input0, PostProcessSunBlur);
						}
						if(bUseBloom)
						{
							Pass->SetInput(ePId_Input1, PostProcessBloomSetup);
							Pass->SetInput(ePId_Input2, PostProcessUpsample2);
						}
						PostProcessSunMerge = FRenderingCompositeOutputRef(Pass);
						BloomOutput = PostProcessSunMerge;
					}

					// Mobile temporal AA requires a composite of two of these frames.
					if(bUseAa && (bUseBloom || bUseSun))
					{
						FSceneViewState* ViewState = (FSceneViewState*)View.State;
						FRenderingCompositeOutputRef PostProcessSunMerge2;
						if(ViewState && ViewState->MobileAaBloomSunVignette1)
						{
							FRenderingCompositePass* History;
							History = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessInput(ViewState->MobileAaBloomSunVignette1));
							PostProcessSunMerge2 = FRenderingCompositeOutputRef(History);
						}
						else
						{
							PostProcessSunMerge2 = PostProcessSunMerge;
						}

						FRenderingCompositeOutputRef PostProcessSunAvg;
						{
							FRenderingCompositePass* Pass = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessSunAvgES2(PrePostSourceViewportSize));
							Pass->SetInput(ePId_Input0, PostProcessSunMerge);
							Pass->SetInput(ePId_Input1, PostProcessSunMerge2);
							PostProcessSunAvg = FRenderingCompositeOutputRef(Pass);
						}
						BloomOutput = PostProcessSunAvg;
					}
				}
			}
		}

		// Must run to blit to back buffer even if post processing is off.
		FRenderingCompositePass* PostProcessTonemap = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessTonemapES2(bUsedFramebufferFetch));
		PostProcessTonemap->SetInput(ePId_Input0, Context.FinalOutput);
		PostProcessTonemap->SetInput(ePId_Input1, BloomOutput);
		PostProcessTonemap->SetInput(ePId_Input2, DofOutput);
		Context.FinalOutput = FRenderingCompositeOutputRef(PostProcessTonemap);

		if(bUseAa && View.Family->EngineShowFlags.PostProcessing)
		{
			// Double buffer post output.
			FSceneViewState* ViewState = (FSceneViewState*)View.State;

			FRenderingCompositeOutputRef PostProcessPrior = Context.FinalOutput;
			if(ViewState && ViewState->MobileAaColor1)
			{
				FRenderingCompositePass* History;
				History = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessInput(ViewState->MobileAaColor1));
				PostProcessPrior = FRenderingCompositeOutputRef(History);
			}

			// Mobile temporal AA is done after tonemapping.
			FIntPoint PrePostSourceViewportSize = View.ViewRect.Size();
			FRenderingCompositePass* PostProcessAa = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessAaES2(PrePostSourceViewportSize));
			PostProcessAa->SetInput(ePId_Input0, Context.FinalOutput);
			PostProcessAa->SetInput(ePId_Input1, PostProcessPrior);
			Context.FinalOutput = FRenderingCompositeOutputRef(PostProcessAa);
		}

		if(View.Family->EngineShowFlags.ShaderComplexity)
		{
			FRenderingCompositePass* Node = Context.Graph.RegisterPass(new(FMemStack::Get()) FRCPassPostProcessVisualizeComplexity(GEngine->ShaderComplexityColors));
			Node->SetInput(ePId_Input0, FRenderingCompositeOutputRef(Context.FinalOutput));
			Context.FinalOutput = FRenderingCompositeOutputRef(Node);
		}

		// The graph setup should be finished before this line ----------------------------------------

		{
			// currently created on the heap each frame but View.Family->RenderTarget could keep this object and all would be cleaner
			TRefCountPtr<IPooledRenderTarget> Temp;
			FSceneRenderTargetItem Item;
			Item.TargetableTexture = (FTextureRHIRef&)View.Family->RenderTarget->GetRenderTargetTexture();
			Item.ShaderResourceTexture = (FTextureRHIRef&)View.Family->RenderTarget->GetRenderTargetTexture();

			FPooledRenderTargetDesc Desc;

			Desc.Extent = View.Family->RenderTarget->GetSizeXY();
			// todo: this should come from View.Family->RenderTarget
			Desc.Format = PF_B8G8R8A8;
			Desc.NumMips = 1;

			GRenderTargetPool.CreateUntrackedElement(Desc, Temp, Item);

			OverrideRenderTarget(Context.FinalOutput, Temp, Desc);

			// you can add multiple dependencies
			CompositeContext.Root->AddDependency(Context.FinalOutput);

			CompositeContext.Process(TEXT("PostProcessingES2"));
		}
	}
}
