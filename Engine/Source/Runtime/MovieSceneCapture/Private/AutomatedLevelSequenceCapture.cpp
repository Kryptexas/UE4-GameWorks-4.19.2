// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCapturePCH.h"
#include "LevelSequencePlayer.h"
#include "AutomatedLevelSequenceCapture.h"
#include "ErrorCodes.h"
#include "ActiveMovieSceneCaptures.h"

UAutomatedLevelSequenceCapture::UAutomatedLevelSequenceCapture(const FObjectInitializer& Init)
	: Super(Init)
{
	bStageSequence = false;
	bWasPlaying = false;
}

void UAutomatedLevelSequenceCapture::SetLevelSequenceAsset(FString AssetPath)
{
	LevelSequenceAsset = MoveTemp(AssetPath);
}

void UAutomatedLevelSequenceCapture::SetLevelSequenceActor(ALevelSequenceActor* InActor)
{
	LevelSequenceActor = InActor;
	LevelSequenceActorId = LevelSequenceActor.GetUniqueID().GetGuid();
}

void UAutomatedLevelSequenceCapture::Initialize(TWeakPtr<FSceneViewport> InViewport)
{
	// Ensure the LevelSequence is up to date with the LevelSequenceActorId (Level sequence is only there for a nice UI)
	LevelSequenceActor = FUniqueObjectGuid(LevelSequenceActorId);

	// Attempt to find the level sequence actor
	ALevelSequenceActor* Actor = LevelSequenceActor.Get();
	if (Actor)
	{
		// Ensure that the sequence cannot play by itself. This is safe because this code is called before BeginPlay()
		Actor->bAutoPlay = false;
	}
	
	PrerollTime = 0.f;
	CaptureStrategy = MakeShareable(new FFixedTimeStepCaptureStrategy(Settings.FrameRate));

	Super::Initialize(InViewport);
}

void UAutomatedLevelSequenceCapture::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ALevelSequenceActor* Actor = LevelSequenceActor.Get();

	if (!Actor)
	{
		ULevelSequence* Asset = Cast<ULevelSequence>(LevelSequenceAsset.TryLoad());
		if (Asset)
		{
			// Spawn a new actor
			Actor = GWorld->SpawnActor<ALevelSequenceActor>();
			Actor->SetSequence(Asset);
			// Ensure it doesn't loop (-1 is indefinite)
			Actor->PlaybackSettings.LoopCount = 0;
			
			LevelSequenceActor = Actor;
		}
		else
		{
			FPlatformMisc::RequestExit(FMovieSceneCaptureExitCodes::AssetNotFound);
		}
	}

	if (Actor && Actor->SequencePlayer)
	{
		// First off, check if we need to stage the sequence
		if (bStageSequence)
		{
			Actor->SequencePlayer->Play();
			Actor->SequencePlayer->Pause();
			bStageSequence = false;
		}

		// Then handle any preroll
		if (PrerollTime.IsSet())
		{
			float& Value = PrerollTime.GetValue();
			Value += DeltaSeconds;
			if (Value >= PrerollAmount)
			{
				Actor->SequencePlayer->Play();
				StartCapture();
				PrepareForScreenshot();

				PrerollTime.Reset();
			}
		}

		// If the sequence is playing, we need to capture the frame. Ensure we get the last frame if the sequence was stopped this frame.
		else if (Actor->SequencePlayer->IsPlaying() || bWasPlaying)
		{
			PrepareForScreenshot();
		}

		// Otherwise we can quit if we've processed all the frames
		else if (!bHasOutstandingFrame)
		{
			Close();
			FPlatformMisc::RequestExit(0);
		}

		bWasPlaying = Actor->SequencePlayer->IsPlaying();
	}
}
#if WITH_EDITOR
void UAutomatedLevelSequenceCapture::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	LevelSequenceActorId = LevelSequenceActor.GetUniqueID().GetGuid();
}
#endif