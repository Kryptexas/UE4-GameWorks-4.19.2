// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCapturePCH.h"

#include "LevelSequencePlayer.h"
#include "AutomatedLevelSequenceCapture.h"
#include "ErrorCodes.h"
#include "ActiveMovieSceneCaptures.h"

UAutomatedLevelSequenceCapture::UAutomatedLevelSequenceCapture(const FObjectInitializer& Init)
	: Super(Init)
{
#if WITH_EDITORONLY_DATA == 0
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		checkf(false, TEXT("Automated level sequence captures can only be used in editor builds."));
	}
#else
	bStageSequence = false;
#endif
}

#if WITH_EDITORONLY_DATA

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
	// Apply command-line overrides
	{
		FString LevelSequenceAssetPath;
		if( FParse::Value( FCommandLine::Get(), TEXT( "-LevelSequence=" ), LevelSequenceAssetPath ) )
		{
			LevelSequenceAsset.SetPath( LevelSequenceAssetPath );
			LevelSequenceActorId = FGuid();
		}

		float PrerollOverride;
		if( FParse::Value( FCommandLine::Get(), TEXT( "-MoviePrerollSeconds=" ), PrerollOverride ) )
		{
			PrerollAmount = PrerollOverride;
		}

		bool bStageSequenceOverride;
		if( FParse::Bool( FCommandLine::Get(), TEXT( "-MoviePreStage=" ), bStageSequenceOverride ) )
		{
			bStageSequence = bStageSequenceOverride;
		}
	}

	// Ensure the LevelSequence is up to date with the LevelSequenceActorId (Level sequence is only there for a nice UI)
	LevelSequenceActor = FUniqueObjectGuid(LevelSequenceActorId);

	ALevelSequenceActor* Actor = LevelSequenceActor.Get();

	// If we don't have a valid actor, attempt to find a level sequence actor in the world that references this asset
	if( Actor == nullptr )
	{
		if( LevelSequenceAsset.IsValid() )
		{
			ULevelSequence* Asset = Cast<ULevelSequence>( LevelSequenceAsset.TryLoad() );
			if( Asset != nullptr )
			{
				for( auto It = TActorIterator<ALevelSequenceActor>( InViewport.Pin()->GetClient()->GetWorld() ); It; ++It )
				{
					if( It->LevelSequence == LevelSequenceAsset )
					{
						// Found it!
						Actor = *It;
						this->LevelSequenceActor = Actor;

						break;
					}
				}
			}
		}
	}

	if (Actor)
	{
		// Make sure we're not playing yet (in case AutoPlay was called from BeginPlay)
		if( Actor->SequencePlayer != nullptr && Actor->SequencePlayer->IsPlaying() )
		{
			Actor->SequencePlayer->Stop();
		}
		Actor->bAutoPlay = false;

		// Bind to the event so we know when to capture a frame
		if( Actor->SequencePlayer != nullptr )
		{
			OnPlayerUpdatedBinding = Actor->SequencePlayer->OnSequenceUpdated().AddUObject( this, &UAutomatedLevelSequenceCapture::SequenceUpdated );
		}
	}
	
	PrerollTime = 0.f;
	CaptureStrategy = MakeShareable(new FFixedTimeStepCaptureStrategy(Settings.FrameRate));

	Super::Initialize(InViewport);
}


void UAutomatedLevelSequenceCapture::SetupFrameRange()
{
	ALevelSequenceActor* Actor = LevelSequenceActor.Get();
	if( Actor )
	{
		ULevelSequence* LevelSequence = Cast<ULevelSequence>( Actor->LevelSequence.TryLoad() );
		if( LevelSequence != nullptr )
		{
			UMovieScene* MovieScene = LevelSequence->GetMovieScene();
			if( MovieScene != nullptr )
			{
				// We always add 1 to the number of frames we want to capture, because we want to capture both the start and end frames (which if the play range is 0, would still yield a single frame)
				const int32 SequenceStartFrame = FMath::RoundToInt( MovieScene->GetPlaybackRange().GetLowerBoundValue() * Settings.FrameRate );
				const int32 SequenceEndFrame = FMath::Max( SequenceStartFrame, FMath::RoundToInt( MovieScene->GetPlaybackRange().GetUpperBoundValue() * Settings.FrameRate ) );

				// Default to playing back the sequence's stored playback range
				int32 PlaybackStartFrame = SequenceStartFrame;
				int32 PlaybackEndFrame = SequenceEndFrame;

				if( Settings.bUseCustomStartFrame )
				{
					PlaybackStartFrame = Settings.bUseRelativeFrameNumbers ? ( SequenceStartFrame + Settings.StartFrame ) : Settings.StartFrame;
				}

				if( Settings.bUseCustomEndFrame )
				{
					PlaybackEndFrame = FMath::Max( PlaybackStartFrame, Settings.bUseRelativeFrameNumbers ? ( SequenceEndFrame + Settings.EndFrame ) : Settings.EndFrame );
				}

				this->FrameCount = ( PlaybackEndFrame - PlaybackStartFrame ) + 1;

				if( !Settings.bUseRelativeFrameNumbers )
				{
					this->FrameNumberOffset = PlaybackStartFrame;
				}

				// Override the movie scene's playback range
				Actor->SequencePlayer->SetPlaybackRange(
					(float)PlaybackStartFrame / (float)Settings.FrameRate,
					(float)PlaybackEndFrame / (float)Settings.FrameRate );
			}
		}
	}
}


void UAutomatedLevelSequenceCapture::Tick(float DeltaSeconds)
{
	const bool bAnyFramesToCapture = OutstandingFrameCount > 0;
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

				PrerollTime.Reset();
			}
		}

		if( !PrerollTime.IsSet() )
		{
			if( bAnyFramesToCapture && OutstandingFrameCount == 0 )
			{
				// If we hit this, then we've rendered out the last frame and can stop!
				StopCapture();
			}
		}
	}
}

void UAutomatedLevelSequenceCapture::OnCaptureStopped()
{
	ALevelSequenceActor* Actor = LevelSequenceActor.Get();
	if( Actor != nullptr )
	{
		if( Actor->SequencePlayer != nullptr )
		{
			Actor->SequencePlayer->OnSequenceUpdated().Remove( OnPlayerUpdatedBinding );
		}
	}

	Close();
	FPlatformMisc::RequestExit(0);
}

void UAutomatedLevelSequenceCapture::SequenceUpdated(const ULevelSequencePlayer& Player, float CurrentTime, float PreviousTime)
{
	if( bCapturing )
	{
		// Save the previous rendered frame
		CaptureFrame( LastFrameDelta );

		// Get ready to capture the next rendered frame
		PrepareForScreenshot();
	}
}

void UAutomatedLevelSequenceCapture::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	LevelSequenceActorId = LevelSequenceActor.GetUniqueID().GetGuid();
}

#endif