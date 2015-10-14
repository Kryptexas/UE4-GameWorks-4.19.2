// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCapturePCH.h"
#include "LevelCapture.h"

void ULevelCapture::Initialize(FViewport* InViewport)
{
	Super::Initialize(InViewport);

	CaptureStrategy = MakeShareable(new FFixedTimeStepCaptureStrategy(Settings.FrameRate));
	StartCapture();
}