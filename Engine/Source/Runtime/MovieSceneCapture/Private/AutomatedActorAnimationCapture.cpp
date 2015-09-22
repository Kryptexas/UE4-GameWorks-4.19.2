// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCapturePCH.h"
#include "ActorAnimationPlayer.h"
#include "AutomatedActorAnimationCapture.h"
#include "ErrorCodes.h"

UAutomatedActorAnimationCapture::UAutomatedActorAnimationCapture(const FObjectInitializer& Init)
	: Super(Init)
{
	PlaybackSettings.LoopCount = 0;
}

void UAutomatedActorAnimationCapture::Initialize(FViewport* InViewport)
{
	if (UActorAnimation* ActorAnimationAsset = Cast<UActorAnimation>(ActorAnimation.TryLoad()))
	{
		// Set up an instance of the asset for the current world, so we can resolve the bindings
		AnimationInstance = NewObject<UActorAnimationInstance>(this, "AnimationInstance");
		AnimationInstance->Initialize(ActorAnimationAsset, GWorld, false);

		// Set up an animation player that can play back the instance
		AnimationPlayback = NewObject<UActorAnimationPlayer>(this, "AnimationPlayer");
		AnimationPlayback->Initialize(AnimationInstance, GWorld, PlaybackSettings);

		// Immediately play/pause the animation. This ensures that the scene is set up, ready for playing.
		// If a preroll has been specified, this will allow temporal PP effects to settle down before capturing
		AnimationPlayback->Play();
		AnimationPlayback->Pause();

		PrerollTime = 0.f;
	}
	else
	{
		FPlatformMisc::RequestExit(FMovieSceneCaptureExitCodes::AssetNotFound);
	}

	CaptureStrategy = MakeShareable(new FFixedTimeStepCaptureStrategy(Settings.FrameRate));
	Super::Initialize(InViewport);
}

void UAutomatedActorAnimationCapture::CaptureFrame(float DeltaSeconds)
{
	if (AnimationPlayback)
	{
		if (PrerollTime.IsSet())
		{
			float& Value = PrerollTime.GetValue();
			Value += DeltaSeconds;
			if (Value >= PrerollAmount)
			{
				AnimationPlayback->Play();
				StartCapture();
				PrerollTime.Reset();
			}
		}
		else
		{
			Super::CaptureFrame(DeltaSeconds);
			AnimationPlayback->Update(DeltaSeconds);
			
			if (!AnimationPlayback->IsPlaying())
			{
				FPlatformMisc::RequestExit(0);
			}
		}
	}
}