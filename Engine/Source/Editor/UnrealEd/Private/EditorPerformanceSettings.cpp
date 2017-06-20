// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Editor/EditorPerformanceSettings.h"

UEditorPerformanceSettings::UEditorPerformanceSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bShowFrameRateAndMemory(false)
	, bThrottleCPUWhenNotForeground(true)
	, bMonitorEditorPerformance(true)
{

}