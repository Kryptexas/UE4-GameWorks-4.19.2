// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCapturePCH.h"
#include "CompositionGraphCaptureProtocol.h"
#include "SceneViewExtension.h"
#include "BufferVisualizationData.h"
#include "SceneViewport.h"

struct FSceneViewExtension : ISceneViewExtension
{
	FSceneViewExtension(const TArray<FString>& InRenderPasses, bool bInCaptureFramesInHDR, UMaterialInterface* InPostProcessingMaterial)
		: RenderPasses(InRenderPasses)
	{
		PostProcessingMaterial = InPostProcessingMaterial;
		bCaptureFramesInHDR = bInCaptureFramesInHDR;

		CVarDumpFrames = IConsoleManager::Get().FindConsoleVariable(TEXT("r.BufferVisualizationDumpFrames"));
		CVarDumpFramesAsHDR = IConsoleManager::Get().FindConsoleVariable(TEXT("r.BufferVisualizationDumpFramesAsHDR"));

		Disable();
	}

	virtual ~FSceneViewExtension()
	{
		Disable();
	}

	bool IsEnabled() const
	{
		return bNeedsCapture;
	}

	void Enable(FString&& InFilename)
	{
		OutputFilename = MoveTemp(InFilename);

		bNeedsCapture = true;
		RestoreDumpHDR = CVarDumpFramesAsHDR->GetInt();
		CVarDumpFramesAsHDR->Set(bCaptureFramesInHDR);
		CVarDumpFrames->Set(1);
	}

	void Disable()
	{
		if (bNeedsCapture)
		{
			bNeedsCapture = false;
			CVarDumpFramesAsHDR->Set(RestoreDumpHDR);
			CVarDumpFrames->Set(0);
		}
	}

	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView)
	{
		if (!bNeedsCapture)
		{
			return;
		}

		InView.FinalPostProcessSettings.bBufferVisualizationDumpRequired = true;
		InView.FinalPostProcessSettings.BufferVisualizationOverviewMaterials.Empty();
		InView.FinalPostProcessSettings.BufferVisualizationDumpBaseFilename = MoveTemp(OutputFilename);

		struct FIterator
		{
			FFinalPostProcessSettings& FinalPostProcessSettings;
			const TArray<FString>& RenderPasses;

			FIterator(FFinalPostProcessSettings& InFinalPostProcessSettings, const TArray<FString>& InRenderPasses)
				: FinalPostProcessSettings(InFinalPostProcessSettings), RenderPasses(InRenderPasses)
			{}

			void ProcessValue(const FString& InName, UMaterial* Material, const FText&)
			{
				if (!RenderPasses.Num() || RenderPasses.Contains(InName))
				{
					FinalPostProcessSettings.BufferVisualizationOverviewMaterials.Add(Material);
				}
			}
		} Iterator(InView.FinalPostProcessSettings, RenderPasses);
		GetBufferVisualizationData().IterateOverAvailableMaterials(Iterator);

		if (PostProcessingMaterial)
		{
			FWeightedBlendable Blendable(1.f, PostProcessingMaterial);
			PostProcessingMaterial->OverrideBlendableSettings(InView, 1.f);
		}
		
		// Ensure we're rendering at full size
		InView.ViewRect = InView.UnscaledViewRect;

		bNeedsCapture = false;
	}

	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) {}
	virtual void PreRenderViewFamily_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneViewFamily& InViewFamily) {}
	virtual void PreRenderView_RenderThread(FRHICommandListImmediate& RHICmdList, FSceneView& InView) {}

private:
	const TArray<FString>& RenderPasses;

	bool bCaptureFramesInHDR;

	UMaterialInterface* PostProcessingMaterial;

	bool bNeedsCapture;
	FString OutputFilename;

	IConsoleVariable* CVarDumpFrames;
	IConsoleVariable* CVarDumpFramesAsHDR;

	int32 RestoreDumpHDR;
};

bool FCompositionGraphCaptureProtocol::Initialize(const FCaptureProtocolInitSettings& InSettings, const ICaptureProtocolHost& Host)
{
	SceneViewport = InSettings.SceneViewport;

	bool bCaptureFramesInHDR = false;

	UMaterialInterface* PostProcessingMaterial = nullptr;
	UCompositionGraphCaptureSettings* ProtocolSettings = CastChecked<UCompositionGraphCaptureSettings>(InSettings.ProtocolSettings);
	if (ProtocolSettings)
	{
		RenderPasses = ProtocolSettings->IncludeRenderPasses.Value;
		bCaptureFramesInHDR = ProtocolSettings->bCaptureFramesInHDR;
		PostProcessingMaterial = Cast<UMaterialInterface>(ProtocolSettings->PostProcessingMaterial.TryLoad());
	}

	ViewExtension = MakeShareable(new FSceneViewExtension(RenderPasses, bCaptureFramesInHDR, PostProcessingMaterial));

	return true;
}

void FCompositionGraphCaptureProtocol::Finalize()
{
	ViewExtension->Disable();
	GEngine->ViewExtensions.Remove(ViewExtension);
}

void FCompositionGraphCaptureProtocol::CaptureFrame(const FFrameMetrics& FrameMetrics, const ICaptureProtocolHost& Host)
{
	ViewExtension->Enable(Host.GenerateFilename(FrameMetrics, TEXT("")));

	GEngine->ViewExtensions.AddUnique(ViewExtension);
}

bool FCompositionGraphCaptureProtocol::HasFinishedProcessing() const
{
	return !ViewExtension->IsEnabled();
}

void FCompositionGraphCaptureProtocol::Tick()
{
	if (!ViewExtension->IsEnabled())
	{
		ViewExtension->Disable();
		GEngine->ViewExtensions.Remove(ViewExtension);
	}
}