// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneSequencePlayer.h"
#include "MovieScene.h"
#include "MovieSceneSequence.h"


bool FMovieSceneSequencePlaybackSettings::SerializeFromMismatchedTag( const FPropertyTag& Tag, FArchive& Ar )
{
	if (Tag.Type == NAME_StructProperty && Tag.StructName == "LevelSequencePlaybackSettings")
	{
		StaticStruct()->SerializeItem(Ar, this, nullptr);
		return true;
	}

	return false;
}

UMovieSceneSequencePlayer::UMovieSceneSequencePlayer(const FObjectInitializer& Init)
	: Super(Init)
	, bIsPlaying(false)
	, bReversePlayback(false)
	, bIsEvaluating(false)
	, Sequence(nullptr)
	, TimeCursorPosition(0.0f)
	, StartTime(0.f)
	, EndTime(0.f)
	, CurrentNumLoops(0)
{
}

EMovieScenePlayerStatus::Type UMovieSceneSequencePlayer::GetPlaybackStatus() const
{
	return bIsPlaying ? EMovieScenePlayerStatus::Playing : EMovieScenePlayerStatus::Stopped;
}

FMovieSceneSpawnRegister& UMovieSceneSequencePlayer::GetSpawnRegister()
{
	return SpawnRegister.IsValid() ? *SpawnRegister : IMovieScenePlayer::GetSpawnRegister();
}

void UMovieSceneSequencePlayer::ResolveBoundObjects(const FGuid& InBindingId, FMovieSceneSequenceID SequenceID, UMovieSceneSequence& InSequence, UObject* ResolutionContext, TArray<UObject*, TInlineAllocator<1>>& OutObjects) const
{
	bool bAllowDefault = PlaybackSettings.BindingOverrides ? PlaybackSettings.BindingOverrides->LocateBoundObjects(InBindingId, OutObjects) : true;

	if (bAllowDefault)
	{
		InSequence.LocateBoundObjects(InBindingId, ResolutionContext, OutObjects);
	}
}

void UMovieSceneSequencePlayer::Play()
{
	bReversePlayback = false;
	PlayInternal();
}

void UMovieSceneSequencePlayer::PlayReverse()
{
	bReversePlayback = true;
	PlayInternal();
}

void UMovieSceneSequencePlayer::ChangePlaybackDirection()
{
	bReversePlayback = !bReversePlayback;
	PlayInternal();
}

void UMovieSceneSequencePlayer::PlayLooping(int32 NumLoops)
{
	PlaybackSettings.LoopCount = NumLoops;
	PlayInternal();
}

void UMovieSceneSequencePlayer::PlayInternal()
{
	if (!bIsPlaying)
	{
		// Start playing
		StartPlayingNextTick();

		// Update now
		bPendingFirstUpdate = false;
		
		if (PlaybackSettings.bRestoreState)
		{
			PreAnimatedState.EnableGlobalCapture();
		}

		UMovieSceneSequence* MovieSceneSequence = RootTemplateInstance.GetSequence(MovieSceneSequenceID::Root);
		TOptional<float> FixedFrameInterval = MovieSceneSequence->GetMovieScene() ? MovieSceneSequence->GetMovieScene()->GetOptionalFixedFrameInterval() : TOptional<float>();

		FMovieSceneEvaluationRange Range = PlayPosition.JumpTo(GetSequencePosition(), FixedFrameInterval);
		UpdateMovieSceneInstance(Range);

		if (OnPlay.IsBound())
		{
			OnPlay.Broadcast();
		}
	}
}

void UMovieSceneSequencePlayer::StartPlayingNextTick()
{
	if (bIsPlaying || !Sequence || !CanPlay())
	{
		return;
	}

	// @todo Sequencer playback: Should we recreate the instance every time?
	// We must not recreate the instance since it holds stateful information (such as which objects it has spawned). Recreating the instance would break any 
	// @todo: Is this still the case now that eval state is stored (correctly) in the player?
	if (!RootTemplateInstance.IsValid())
	{
		RootTemplateInstance.Initialize(*Sequence, *this);
	}

	OnStartedPlaying();

	bPendingFirstUpdate = true;
	bIsPlaying = true;
}

void UMovieSceneSequencePlayer::Pause()
{
	if (bIsPlaying)
	{
		if (bIsEvaluating)
		{
			LatentActions.Add(ELatentAction::Pause);
			return;
		}

		bIsPlaying = false;

		// Evaluate the sequence at its current time, with a status of 'stopped' to ensure that animated state pauses correctly
		{
			bIsEvaluating = true;

			UMovieSceneSequence* MovieSceneSequence = RootTemplateInstance.GetSequence(MovieSceneSequenceID::Root);
			TOptional<float> FixedFrameInterval = MovieSceneSequence->GetMovieScene() ? MovieSceneSequence->GetMovieScene()->GetOptionalFixedFrameInterval() : TOptional<float>();

			FMovieSceneEvaluationRange Range = PlayPosition.JumpTo(GetSequencePosition(), FixedFrameInterval);

			const FMovieSceneContext Context(Range, EMovieScenePlayerStatus::Stopped);
			RootTemplateInstance.Evaluate(Context, *this);

			bIsEvaluating = false;
		}

		ApplyLatentActions();

		if (OnPause.IsBound())
		{
			OnPause.Broadcast();
		}
	}
}

void UMovieSceneSequencePlayer::Stop()
{
	if (bIsPlaying)
	{
		if (bIsEvaluating)
		{
			LatentActions.Add(ELatentAction::Stop);
			return;
		}

		bIsPlaying = false;
		TimeCursorPosition = PlaybackSettings.PlayRate < 0.f ? GetLength() : 0.f;
		CurrentNumLoops = 0;

		if (PlaybackSettings.bRestoreState)
		{
			RestorePreAnimatedState();
		}

		RootTemplateInstance.Finish(*this);

		OnStopped();

		if (OnStop.IsBound())
		{
			OnStop.Broadcast();
		}
	}
}

float UMovieSceneSequencePlayer::GetPlaybackPosition() const
{
	return TimeCursorPosition;
}

void UMovieSceneSequencePlayer::SetPlaybackPosition(float NewPlaybackPosition)
{
	UpdateTimeCursorPosition(NewPlaybackPosition);
}

bool UMovieSceneSequencePlayer::IsPlaying() const
{
	return bIsPlaying;
}

float UMovieSceneSequencePlayer::GetLength() const
{
	return EndTime - StartTime;
}

float UMovieSceneSequencePlayer::GetPlayRate() const
{
	return PlaybackSettings.PlayRate;
}

void UMovieSceneSequencePlayer::SetPlayRate(float PlayRate)
{
	PlaybackSettings.PlayRate = PlayRate;
}

void UMovieSceneSequencePlayer::SetPlaybackRange( const float NewStartTime, const float NewEndTime )
{
	StartTime = NewStartTime;
	EndTime = FMath::Max(NewEndTime, StartTime);

	TimeCursorPosition = FMath::Clamp(TimeCursorPosition, 0.f, GetLength());
}

bool UMovieSceneSequencePlayer::ShouldStopOrLoop(float NewPosition) const
{
	bool bShouldStopOrLoop = false;
	if (!bReversePlayback)
	{
		bShouldStopOrLoop = NewPosition >= GetLength();
	}
	else
	{
		bShouldStopOrLoop = NewPosition < 0.f;
	}

	return bShouldStopOrLoop;
}

void UMovieSceneSequencePlayer::Initialize(UMovieSceneSequence* InSequence, const FMovieSceneSequencePlaybackSettings& InSettings)
{
	check(InSequence);
	
	Sequence = InSequence;
	PlaybackSettings = InSettings;

	if (UMovieScene* MovieScene = Sequence->GetMovieScene())
	{
		TRange<float> PlaybackRange = MovieScene->GetPlaybackRange();
		SetPlaybackRange(PlaybackRange.GetLowerBoundValue(), PlaybackRange.GetUpperBoundValue());
	}

	TimeCursorPosition = PlaybackSettings.bRandomStartTime ? FMath::FRand() * 0.99f * GetLength() : FMath::Clamp(PlaybackSettings.StartTime, 0.f, GetLength());

	RootTemplateInstance.Initialize(*Sequence, *this);

	// Ensure everything is set up, ready for playback
	Stop();
}

void UMovieSceneSequencePlayer::Update(const float DeltaSeconds)
{
	if (bIsPlaying)
	{
		float PlayRate = bReversePlayback ? -PlaybackSettings.PlayRate : PlaybackSettings.PlayRate;
		UpdateTimeCursorPosition(TimeCursorPosition + DeltaSeconds * PlayRate);
	}
}

void UMovieSceneSequencePlayer::UpdateTimeCursorPosition(float NewPosition)
{
	float Length = GetLength();

	UMovieSceneSequence* MovieSceneSequence = RootTemplateInstance.GetSequence(MovieSceneSequenceID::Root);
	TOptional<float> FixedFrameInterval = MovieSceneSequence->GetMovieScene() ? MovieSceneSequence->GetMovieScene()->GetOptionalFixedFrameInterval() : TOptional<float>();

	if (bPendingFirstUpdate)
	{
		NewPosition = TimeCursorPosition;
		bPendingFirstUpdate = false;
	}

	if (ShouldStopOrLoop(NewPosition))
	{
		// loop playback
		if (PlaybackSettings.LoopCount < 0 || CurrentNumLoops < PlaybackSettings.LoopCount)
		{
			++CurrentNumLoops;
			const float Overplay = FMath::Fmod(NewPosition, Length);
			TimeCursorPosition = Overplay < 0 ? Length + Overplay : Overplay;
			
			PlayPosition.Reset(Overplay < 0 ? Length : 0.f);
			FMovieSceneEvaluationRange Range = PlayPosition.PlayTo(GetSequencePosition(), FixedFrameInterval);

			if (SpawnRegister.IsValid())
			{
				SpawnRegister->ForgetExternallyOwnedSpawnedObjects(State, *this);
			}

			UpdateMovieSceneInstance(Range);

			OnLooped();
			return;
		}

		// stop playback
		Stop();
		return;
	}

	TimeCursorPosition = NewPosition;

	FMovieSceneEvaluationRange Range = PlayPosition.PlayTo(NewPosition + StartTime, FixedFrameInterval);
	UpdateMovieSceneInstance(Range);
}

void UMovieSceneSequencePlayer::UpdateMovieSceneInstance(FMovieSceneEvaluationRange InRange)
{
	bIsEvaluating = true;

	const FMovieSceneContext Context(InRange, GetPlaybackStatus());
	RootTemplateInstance.Evaluate(Context, *this);

#if WITH_EDITOR
	OnMovieSceneSequencePlayerUpdate.Broadcast(*this, Context.GetTime(), Context.GetPreviousTime());
#endif
	bIsEvaluating = false;

	ApplyLatentActions();
}

void UMovieSceneSequencePlayer::ApplyLatentActions()
{
	// Swap to a stack array to ensure no reentrancy if we evaluate during a pause, for instance
	TArray<ELatentAction> TheseActions;
	Swap(TheseActions, LatentActions);

	for (ELatentAction LatentAction : TheseActions)
	{
		switch(LatentAction)
		{
		case ELatentAction::Stop:	Stop(); break;
		case ELatentAction::Pause:	Pause(); break;
		}
	}
}