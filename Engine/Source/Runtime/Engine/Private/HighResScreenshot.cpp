// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "EnginePrivate.h"
#include "HighResScreenshot.h"

FHighResScreenshotConfig& GetHighResScreenshotConfig()
{
	static FHighResScreenshotConfig Instance;
	return Instance;
}

const float FHighResScreenshotConfig::MinResolutionMultipler = 1.0f;
const float FHighResScreenshotConfig::MaxResolutionMultipler = 10.0f;

FHighResScreenshotConfig::FHighResScreenshotConfig()
	: ResolutionMultiplier(FHighResScreenshotConfig::MinResolutionMultipler)
	, ResolutionMultiplierScale(0.0f)
	, bMaskEnabled(false)
	, bDumpBufferVisualizationTargets(false)
	, TargetViewport(NULL)

{
	ChangeViewport(NULL);
}

void FHighResScreenshotConfig::ChangeViewport(FViewport* InViewport)
{
	if (TargetViewport != NULL)
	{
		// Force an invalidate on the old viewport to make sure we clear away the capture region effect
		TargetViewport->Invalidate();
		TargetViewport = NULL;
	}

	UnscaledCaptureRegion = FIntRect(0, 0, -1, -1);
	CaptureRegion = UnscaledCaptureRegion;
	bMaskEnabled = false;
	bDumpBufferVisualizationTargets = false;
	ResolutionMultiplier = 1.0f;
	TargetViewport = InViewport;
}

bool FHighResScreenshotConfig::ParseConsoleCommand(const FString& InCmd, FOutputDevice& Ar)
{
	GScreenshotResolutionX = 0;
	GScreenshotResolutionY = 0;
	ResolutionMultiplier = 1.0f;

	if( GetHighResScreenShotInput(*InCmd, Ar, GScreenshotResolutionX, GScreenshotResolutionY, ResolutionMultiplier, CaptureRegion, bMaskEnabled) )
	{
		GScreenshotResolutionX *= ResolutionMultiplier;
		GScreenshotResolutionY *= ResolutionMultiplier;
		GIsHighResScreenshot = true;
		GScreenMessagesRestoreState = GAreScreenMessagesEnabled;
		GAreScreenMessagesEnabled = false;

		return true;
	}

	return false;
}

bool FHighResScreenshotConfig::MergeMaskIntoAlpha(TArray<FColor>& InBitmap)
{
	bool bWritten = false;

	if (bMaskEnabled)
	{
		// If this is a high resolution screenshot and we are using the masking feature,
		// Get the results of the mask rendering pass and insert into the alpha channel of the screenshot.
		TArray<FColor>* MaskArray = FScreenshotRequest::GetHighresScreenshotMaskColorArray();
		check(MaskArray->Num() == InBitmap.Num());
		for (int32 i = 0; i < MaskArray->Num(); ++i)
		{
			InBitmap[i].A = (*MaskArray)[i].R;
		}

		bWritten = true;
	}

	return bWritten;
}