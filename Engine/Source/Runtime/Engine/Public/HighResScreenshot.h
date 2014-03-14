// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

struct ENGINE_API FHighResScreenshotConfig
{
	static const float MinResolutionMultipler;
	static const float MaxResolutionMultipler;

	FIntRect UnscaledCaptureRegion;
	FIntRect CaptureRegion;
	float ResolutionMultiplier;
	float ResolutionMultiplierScale;
	bool bMaskEnabled;
	bool bDumpBufferVisualizationTargets;
	class FViewport* TargetViewport;
	bool bDisplayCaptureRegion;

	FHighResScreenshotConfig();

	/** Point the screenshot UI at a different viewport **/
	void ChangeViewport(FViewport* InViewport);

	/** Parse screenshot parameters from the supplied console command line **/
	bool ParseConsoleCommand(const FString& InCmd, FOutputDevice& Ar);

	/** Utility function for merging the mask buffer into the alpha channel of the supplied bitmap, if masking is enabled.
	  * Returns true if the mask was written, and false otherwise.
	**/
	bool MergeMaskIntoAlpha(TArray<FColor>& InBitmap);
};

ENGINE_API FHighResScreenshotConfig& GetHighResScreenshotConfig();