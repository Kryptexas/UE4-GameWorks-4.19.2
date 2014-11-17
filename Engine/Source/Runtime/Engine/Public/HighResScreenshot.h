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
	TWeakPtr<FSceneViewport> TargetViewport;
	bool bDisplayCaptureRegion;
	bool bCaptureHDR;

	TSharedPtr<class IImageWrapper> ImageCompressorLDR;
	TSharedPtr<class IImageWrapper> ImageCompressorHDR;

	FHighResScreenshotConfig();

	/** Initialize the image wrapper modules (required for SaveImage) **/
	void Init();

	/** Point the screenshot UI at a different viewport **/
	void ChangeViewport(TWeakPtr<FSceneViewport> InViewport);

	/** Parse screenshot parameters from the supplied console command line **/
	bool ParseConsoleCommand(const FString& InCmd, FOutputDevice& Ar);

	/** Utility function for merging the mask buffer into the alpha channel of the supplied bitmap, if masking is enabled.
	  * Returns true if the mask was written, and false otherwise.
	**/
	bool MergeMaskIntoAlpha(TArray<FColor>& InBitmap);

	/** Enable/disable HDR capable captures **/
	void SetHDRCapture(bool bCaptureHDRIN);

	/** Save to image file **/
	template<typename TPixelType>
	ENGINE_API FString SaveImage(const FString& File, const TArray<TPixelType>& Bitmap, const FIntPoint& BitmapSize, EPixelFormat SourceFormat) const;
};

ENGINE_API FHighResScreenshotConfig& GetHighResScreenshotConfig();