// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCapturePCH.h"
#include "MovieSceneCapture.h"
#include "MovieSceneCaptureEnvironment.h"


UMovieSceneCapture* FindMovieSceneCapture()
{
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.GameViewport && Context.GameViewport->Viewport)
		{
			UMovieSceneCapture* Capture = static_cast<UMovieSceneCapture*>(Context.GameViewport->Viewport->GetMovieSceneCapture());
			if (Capture)
			{
				return Capture;
			}
		}
	}

	return nullptr;
}

int32 UMovieSceneCaptureEnvironment::GetCaptureFrameNumber()
{
	UMovieSceneCapture* Capture = FindMovieSceneCapture();
	return Capture ? Capture->GetMetrics().Frame : 0;
}

float UMovieSceneCaptureEnvironment::GetCaptureElapsedTime()
{
	UMovieSceneCapture* Capture = FindMovieSceneCapture();
	return Capture ? Capture->GetMetrics().ElapsedSeconds : 0.f;
}