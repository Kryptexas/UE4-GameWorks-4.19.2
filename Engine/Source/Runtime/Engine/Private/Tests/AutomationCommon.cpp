// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "SlateBasics.h"
#include "AutomationCommon.h"
#include "ImageUtils.h"

///////////////////////////////////////////////////////////////////////
// Common Latent commands

namespace AutomationCommon
{
	/** These save a PNG and get sent over the network */
	static void SaveWindowAsScreenshot(TSharedRef<SWindow> Window, const FString& FileName)
	{
		TSharedRef<SWidget> WindowRef = Window;

		TArray<FColor> OutImageData;
		FIntVector OutImageSize;
		FSlateApplication::Get().TakeScreenshot(WindowRef,OutImageData,OutImageSize);

		FAutomationTestFramework::GetInstance().OnScreenshotCaptured().ExecuteIfBound(OutImageSize.X, OutImageSize.Y, OutImageData, FileName);
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
	AutomationCommon::SaveWindowAsScreenshot(FSlateApplication::Get().GetActiveTopLevelWindow().ToSharedRef(),ScreenshotName);
	return true;
}

bool FTakeEditorScreenshotCommand::Update()
{
	AutomationCommon::SaveWindowAsScreenshot(ScreenshotParameters.CurrentWindow.ToSharedRef(), ScreenshotParameters.ScreenshotName);
	return true;
}