// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "EnginePrivate.h"
#include "HighResScreenshot.h"
#include "Slate/SceneViewport.h"
#include "ImageWrapper.h"

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
{
	ChangeViewport(TWeakPtr<FSceneViewport>());
	SetHDRCapture(false);
}

void FHighResScreenshotConfig::Init()
{
	IImageWrapperModule* ImageWrapperModule = FModuleManager::LoadModulePtr<IImageWrapperModule>(FName("ImageWrapper"));
	if (ImageWrapperModule != nullptr)
	{
		ImageCompressorLDR = ImageWrapperModule->CreateImageWrapper(EImageFormat::PNG);
		ImageCompressorHDR = ImageWrapperModule->CreateImageWrapper(EImageFormat::EXR);
	}
}

void FHighResScreenshotConfig::ChangeViewport(TWeakPtr<FSceneViewport> InViewport)
{
	if (FSceneViewport* Viewport = TargetViewport.Pin().Get())
	{
		// Force an invalidate on the old viewport to make sure we clear away the capture region effect
		Viewport->Invalidate();
	}

	UnscaledCaptureRegion = FIntRect(0, 0, 0, 0);
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

void FHighResScreenshotConfig::SetHDRCapture(bool bCaptureHDRIN)
{
	bCaptureHDR = bCaptureHDRIN;
}

template<typename TPixelType>
FString FHighResScreenshotConfig::SaveImage(const FString& File, const TArray<TPixelType>& Bitmap, const FIntPoint& BitmapSize, EPixelFormat SourceFormat) const
{
	static_assert(ARE_TYPES_EQUAL(TPixelType, FFloat16Color) || ARE_TYPES_EQUAL(TPixelType, FColor), "Source format must be either FColor or FFloat16Color");
	check(SourceFormat == PF_FloatRGBA || SourceFormat == PF_B8G8R8A8 || SourceFormat == PF_R8G8B8A8);
	const int32 x = BitmapSize.X;
	const int32 y = BitmapSize.Y;
	check(Bitmap.Num() == x * y);

	static const auto CVarDumpFramesAsHDR = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.BufferVisualizationDumpFramesAsHDR"));
	bool bIsWritingHDRImage = (bCaptureHDR || CVarDumpFramesAsHDR->GetValueOnRenderThread()) && SourceFormat == PF_FloatRGBA;

	IFileManager* FileManager = &IFileManager::Get();
	const size_t BitsPerPixel = (sizeof(TPixelType) / 4) * 8;
	const ERGBFormat::Type SourceChannelLayout = SourceFormat == PF_B8G8R8A8 ? ERGBFormat::BGRA : ERGBFormat::RGBA;

	TSharedPtr<class IImageWrapper> ImageCompressor = bIsWritingHDRImage ? ImageCompressorHDR : ImageCompressorLDR;
	FString Filename = File + (bIsWritingHDRImage ? TEXT(".exr") : TEXT(".png"));

	if (ImageCompressor.IsValid() && ImageCompressor->SetRaw((void*)&Bitmap[0], sizeof(TPixelType)* x * y, x, y, SourceChannelLayout, BitsPerPixel))
	{
		FArchive* Ar = FileManager->CreateFileWriter(Filename.GetCharArray().GetData());
		if (Ar != nullptr)
		{
			const TArray<uint8>& CompressedData = ImageCompressor->GetCompressed();
			int32 CompressedSize = CompressedData.Num();
			Ar->Serialize((void*)CompressedData.GetTypedData(), CompressedSize);
			delete Ar;
		}
		else
		{
			Filename = FString("Failed to open for writing: ") + Filename;
		}
	}
	return Filename;
}

template ENGINE_API FString FHighResScreenshotConfig::SaveImage<FColor>(const FString& File, const TArray<FColor>& Bitmap, const FIntPoint& BitmapSize, EPixelFormat SourceFormat) const;
template ENGINE_API FString FHighResScreenshotConfig::SaveImage<FFloat16Color>(const FString& File, const TArray<FFloat16Color>& Bitmap, const FIntPoint& BitmapSize, EPixelFormat SourceFormat) const;
