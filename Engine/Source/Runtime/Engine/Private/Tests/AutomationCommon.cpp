// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Slate.h"
#include "AutomationCommon.h"
#include "ImageUtils.h"
#include "UnrealEd.h"

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

bool UndoRedoCommand::Update()
{
	if (bUndo == true)
	{
		//Undo
		GEditor->UndoTransaction();
	}
	else
	{
		//Redo
		GEditor->RedoTransaction();
	}

	return true;
}