// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "FunctionalTestingPrivatePCH.h"
#include "AutomationBlueprintFunctionLibrary.h"
#include "TakeScreenshotAfterTimeLatentAction.h"

FTakeScreenshotAfterTimeLatentAction::FTakeScreenshotAfterTimeLatentAction(const FLatentActionInfo& LatentInfo, const FString& InScreenshotName, FAutomationScreenshotOptions InOptions)
	: ExecutionFunction(LatentInfo.ExecutionFunction)
	, OutputLink(LatentInfo.Linkage)
	, CallbackTarget(LatentInfo.CallbackTarget)
	, DelegateHandle()
	, ScreenshotName(InScreenshotName)
	, SecondsRemaining(InOptions.Delay)
	, IssuedScreenshotCapture(false)
	, TakenScreenshot(false)
	, Options(InOptions)
{
}

FTakeScreenshotAfterTimeLatentAction::~FTakeScreenshotAfterTimeLatentAction()
{
	GEngine->GameViewport->OnScreenshotCaptured().RemoveAll(this);
}

void FTakeScreenshotAfterTimeLatentAction::OnScreenshotTaken(int32 InSizeX, int32 InSizeY, const TArray<FColor>& InImageData)
{
	TakenScreenshot = true;
}

void FTakeScreenshotAfterTimeLatentAction::UpdateOperation(FLatentResponse& Response)
{
	if ( !TakenScreenshot )
	{
		if ( !IssuedScreenshotCapture )
		{
			SecondsRemaining -= Response.ElapsedTime();
			if ( SecondsRemaining <= 0.0f )
			{
				DelegateHandle = GEngine->GameViewport->OnScreenshotCaptured().AddRaw(this, &FTakeScreenshotAfterTimeLatentAction::OnScreenshotTaken);

				if ( UAutomationBlueprintFunctionLibrary::TakeAutomationScreenshotInternal(ScreenshotName, Options) )
				{
					IssuedScreenshotCapture = true;
				}
				else
				{
					//TODO LOG FAILED SCREENSHOT
					TakenScreenshot = true;
				}
			}
		}
	}
	else
	{
		GEngine->GameViewport->OnScreenshotCaptured().RemoveAll(this);
		Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
	}
}

#if WITH_EDITOR
FString FTakeScreenshotAfterTimeLatentAction::GetDescription() const
{
	return FString::Printf(TEXT("Take screenshot named %s after %f seconds"), *ScreenshotName, SecondsRemaining);
}
#endif