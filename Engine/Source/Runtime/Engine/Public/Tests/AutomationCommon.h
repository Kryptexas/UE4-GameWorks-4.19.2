// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EnginePrivate.h"

///////////////////////////////////////////////////////////////////////
// Common Latent commands which are used across test type. I.e. Engine, Network, etc...

DEFINE_LOG_CATEGORY_STATIC(LogEditorAutomationTests, Log, All);

/**
 * Parameters to the Latent Automation command FTakeEditorScreenshotCommand
 */
struct WindowScreenshotParameters
{
	FString ScreenshotName;
	TSharedPtr<SWindow> CurrentWindow;
};

/**
 * Wait for the given amount of time
 */
DEFINE_ENGINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FWaitLatentCommand, float, Duration);

/**
 * Take a screenshot of the active window
 */
DEFINE_ENGINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FTakeActiveEditorScreenshotCommand, FString, ScreenshotName);

/**
 * Take a screenshot of the active window
 */
DEFINE_ENGINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FTakeEditorScreenshotCommand, WindowScreenshotParameters, ScreenshotParameters);
