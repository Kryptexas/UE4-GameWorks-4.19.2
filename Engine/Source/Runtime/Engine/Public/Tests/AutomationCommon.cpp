// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "AutomationCommon.h"

///////////////////////////////////////////////////////////////////////
// Common Latent commands which are used across test type. I.e. Engine, Network, etc...

static void SaveWindowAsScreenshot(TSharedRef<SWindow> Window, const FString& FileName)
{
	TSharedRef<SWidget> WindowRef = Window;

	TArray<FColor> OutImageData;
	FIntVector OutImageSize;
	FSlateApplication::Get().TakeScreenshot(WindowRef,OutImageData,OutImageSize);

	FFileHelper::CreateBitmap(*FileName, OutImageSize.X, OutImageSize.Y, OutImageData.GetTypedData());
}

static void SaveEditorWindowsAsScreenshots(const FString& BaseFileName)
{
	TArray<TSharedRef<SWindow> > ChildWindows;
	FSlateApplication::Get().GetAllVisibleWindowsOrdered(ChildWindows);

	for( int32 i = 0; i < ChildWindows.Num(); ++i )
	{
		const FString BMPFileName = FString::Printf(TEXT("%s%02i.bmp"), *BaseFileName, i+1);
		SaveWindowAsScreenshot(ChildWindows[i],BMPFileName);
	}
}

bool FWaitLatentCommand::Update()
{
	float NewTime = FPlatformTime::Seconds();
	if (NewTime - StartTime >= Duration)
	{
		return true;
	}
	return false;
}

bool FTakeActiveEditorScreenshotCommand::Update()
{
	SaveWindowAsScreenshot(FSlateApplication::Get().GetActiveTopLevelWindow().ToSharedRef(),ScreenshotName);
	return true;
}

bool FTakeEditorScreenshotCommand::Update()
{
	SaveWindowAsScreenshot(ScreenshotParameters.CurrentWindow.ToSharedRef(), ScreenshotParameters.ScreenshotName);
	return true;
}
