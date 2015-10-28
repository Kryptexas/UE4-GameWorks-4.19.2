// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "LevelSequencePCH.h"
#include "LevelSequencePlayer.h"
#include "MovieScene.h"
#include "MovieSceneSequence.h"
#include "MovieSceneSequenceInstance.h"


struct FTickAnimationPlayers : public FTickableGameObject
{
	TArray<TWeakObjectPtr<ULevelSequencePlayer>> ActiveInstances;
	
	virtual bool IsTickable() const override
	{
		return ActiveInstances.Num() != 0;
	}

	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FTickAnimationPlayers, STATGROUP_Tickables);
	}
	
	virtual void Tick(float DeltaSeconds) override
	{
		for (int32 Index = 0; Index < ActiveInstances.Num();)
		{
			if (auto* Player = ActiveInstances[Index].Get())
			{
				Player->Update(DeltaSeconds);
				++Index;
			}
			else
			{
				ActiveInstances.RemoveAt(Index, 1, false);
			}
		}
	}
};

struct FAutoDestroyAnimationTicker
{
	FAutoDestroyAnimationTicker()
	{
		FCoreDelegates::OnPreExit.AddLambda([&]{
			Impl.Reset();
		});
	}
	
	void Add(ULevelSequencePlayer* Player)
	{
		if (!Impl.IsValid())
		{
			Impl.Reset(new FTickAnimationPlayers);
		}
		Impl->ActiveInstances.Add(Player);
	}

	TUniquePtr<FTickAnimationPlayers> Impl;
} GAnimationPlayerTicker;

/* ULevelSequencePlayer structors
 *****************************************************************************/

ULevelSequencePlayer::ULevelSequencePlayer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, LevelSequence(nullptr)
	, bIsPlaying(false)
	, TimeCursorPosition(0.0f)
	, CurrentNumLoops(0)
	, bAutoPlayNextFrame(false)
{ }


/* ULevelSequencePlayer interface
 *****************************************************************************/

ULevelSequencePlayer* ULevelSequencePlayer::CreateLevelSequencePlayer(UObject* WorldContextObject, ULevelSequence* InLevelSequence, FLevelSequencePlaybackSettings Settings)
{
	if (InLevelSequence == nullptr)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
	check(World != nullptr);

	ULevelSequencePlayer* NewPlayer = NewObject<ULevelSequencePlayer>(GetTransientPackage(), NAME_None, RF_Transient);
	check(NewPlayer != nullptr);

	NewPlayer->Initialize(InLevelSequence, World, Settings);

	// Automatically tick this player
	GAnimationPlayerTicker.Add(NewPlayer);

	return NewPlayer;
}


bool ULevelSequencePlayer::IsPlaying() const
{
	return bIsPlaying;
}


void ULevelSequencePlayer::Pause()
{
	bIsPlaying = false;
}

void ULevelSequencePlayer::Stop()
{
	bIsPlaying = false;
	TimeCursorPosition = PlaybackSettings.PlayRate < 0.f ? GetLength() : 0.f;
	CurrentNumLoops = 0;

	// todo: Trigger an event?
}

void ULevelSequencePlayer::Play()
{
	if ((LevelSequence == nullptr) || !World.IsValid())
	{
		return;
	}

	// @todo Sequencer playback: Should we recreate the instance every time?
	RootMovieSceneInstance = MakeShareable(new FMovieSceneSequenceInstance(*LevelSequence));
	RootMovieSceneInstance->RefreshInstance(*this);

	bIsPlaying = true;

	if (RootMovieSceneInstance.IsValid())
	{
		RootMovieSceneInstance->Update(TimeCursorPosition + SequenceStartOffset, TimeCursorPosition + SequenceStartOffset, *this);
	}
}

void ULevelSequencePlayer::PlayLooping(int32 NumLoops)
{
	PlaybackSettings.LoopCount = NumLoops;
	Play();
}

float ULevelSequencePlayer::GetPlaybackPosition() const
{
	return TimeCursorPosition;
}

void ULevelSequencePlayer::SetPlaybackPosition(float NewPlaybackPosition)
{
	float LastTimePosition = TimeCursorPosition;
	
	TimeCursorPosition = NewPlaybackPosition;
	OnCursorPositionChanged();

	if (RootMovieSceneInstance.IsValid())
	{
		RootMovieSceneInstance->Update(TimeCursorPosition + SequenceStartOffset, LastTimePosition + SequenceStartOffset, *this);
	}
}

float ULevelSequencePlayer::GetLength() const
{
	if (!LevelSequence)
	{
		return 0;
	}

	UMovieScene* MovieScene = LevelSequence->GetMovieScene();
	return MovieScene ? MovieScene->GetPlaybackRange().Size<float>() : 0;
}

float ULevelSequencePlayer::GetPlayRate() const
{
	return PlaybackSettings.PlayRate;
}

void ULevelSequencePlayer::SetPlayRate(float PlayRate)
{
	PlaybackSettings.PlayRate = PlayRate;
}


void ULevelSequencePlayer::OnCursorPositionChanged()
{
	float Length = GetLength();

	// Handle looping or stopping
	if (TimeCursorPosition >= Length || TimeCursorPosition < 0)
	{
		if (PlaybackSettings.LoopCount < 0 || CurrentNumLoops < PlaybackSettings.LoopCount)
		{
			++CurrentNumLoops;
			const float Overplay = FMath::Fmod(TimeCursorPosition, Length);
			TimeCursorPosition = Overplay < 0 ? Length + Overplay : Overplay;
		}
		else
		{
			// Stop playing without modifying the playback position
			// @todo: trigger an event?
			bIsPlaying = false;
			CurrentNumLoops = 0;
		}
	}
}

/* ULevelSequencePlayer implementation
 *****************************************************************************/

void ULevelSequencePlayer::Initialize(ULevelSequence* InLevelSequence, UWorld* InWorld, const FLevelSequencePlaybackSettings& Settings)
{
	LevelSequence = InLevelSequence;
	LevelSequence->BindToContext(InWorld);

	SequenceStartOffset = LevelSequence->GetMovieScene()->GetPlaybackRange().GetLowerBoundValue();

	World = InWorld;
	PlaybackSettings = Settings;

	// Ensure everything is set up, ready for playback
	Stop();
}


/* IMovieScenePlayer interface
 *****************************************************************************/


void ULevelSequencePlayer::GetRuntimeObjects(TSharedRef<FMovieSceneSequenceInstance> MovieSceneInstance, const FGuid& ObjectId, TArray<UObject*>& OutObjects) const
{
	if (UObject* FoundObject = LevelSequence->FindObject(ObjectId))
	{
		OutObjects.Add(FoundObject);
	}
}


void ULevelSequencePlayer::UpdateCameraCut(UObject* ObjectToViewThrough, bool bNewCameraCut) const
{
}

void ULevelSequencePlayer::SetViewportSettings(const TMap<FViewportClient*, EMovieSceneViewportParams>& ViewportParamsMap)
{
}

void ULevelSequencePlayer::GetViewportSettings(TMap<FViewportClient*, EMovieSceneViewportParams>& ViewportParamsMap) const
{
}

EMovieScenePlayerStatus::Type ULevelSequencePlayer::GetPlaybackStatus() const
{
	return bIsPlaying ? EMovieScenePlayerStatus::Playing : EMovieScenePlayerStatus::Stopped;
}

void ULevelSequencePlayer::AddOrUpdateMovieSceneInstance(UMovieSceneSection& MovieSceneSection, TSharedRef<FMovieSceneSequenceInstance> InstanceToAdd)
{
}

void ULevelSequencePlayer::RemoveMovieSceneInstance(UMovieSceneSection& MovieSceneSection, TSharedRef<FMovieSceneSequenceInstance> InstanceToRemove)
{
}

TSharedRef<FMovieSceneSequenceInstance> ULevelSequencePlayer::GetRootMovieSceneSequenceInstance() const
{
	return RootMovieSceneInstance.ToSharedRef();
}

void ULevelSequencePlayer::Update(const float DeltaSeconds)
{
	if (bAutoPlayNextFrame)
	{
		Play();
		bAutoPlayNextFrame = false;
		return;
	}

	float LastTimePosition = TimeCursorPosition;

	if (bIsPlaying)
	{
		TimeCursorPosition += DeltaSeconds * PlaybackSettings.PlayRate;
		OnCursorPositionChanged();
	}

	if(RootMovieSceneInstance.IsValid())
	{
		RootMovieSceneInstance->Update(TimeCursorPosition + SequenceStartOffset, LastTimePosition + SequenceStartOffset, *this);
	}
}

void ULevelSequencePlayer::AutoPlayNextFrame()
{
	bAutoPlayNextFrame = true;
}