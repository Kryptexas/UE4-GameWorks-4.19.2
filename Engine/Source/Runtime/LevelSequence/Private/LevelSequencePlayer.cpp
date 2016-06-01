// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "LevelSequencePCH.h"
#include "LevelSequencePlayer.h"
#include "MovieScene.h"
#include "MovieSceneSubSection.h"
#include "MovieSceneSequence.h"
#include "MovieSceneSequenceInstance.h"
#include "MovieSceneTrack.h"
#include "LevelSequenceSpawnRegister.h"
#include "Engine/LevelStreaming.h"


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
	, LastCursorPosition(0.0f)
	, StartTime(0.f)
	, EndTime(0.f)
	, CurrentNumLoops(0)
	, bHasCleanedUpSequence(false)
{
	SpawnRegister = MakeShareable(new FLevelSequenceSpawnRegister);
}


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

	SetTickPrerequisites(false);

	// todo: Trigger an event?
}

void GetDescendantSequences(UMovieSceneSequence* InSequence, TArray<UMovieSceneSequence*> & InSequences)
{
	if (InSequence == nullptr)
	{
		return;
	}

	InSequences.Add(InSequence);
	
	UMovieScene* MovieScene = InSequence->GetMovieScene();
	if (MovieScene == nullptr)
	{
		return;
	}

	for (auto MasterTrack : MovieScene->GetMasterTracks())
	{
		for (auto Section : MasterTrack->GetAllSections())
		{
			UMovieSceneSubSection* SubSection = Cast<UMovieSceneSubSection>(Section);
			if (SubSection != nullptr)
			{
				UMovieSceneSequence* SubSequence = SubSection->GetSequence();
				if (SubSequence != nullptr)
				{
					GetDescendantSequences(SubSequence, InSequences);
				}
			}
		}
	}
}

void ULevelSequencePlayer::SetTickPrerequisites(bool bAddTickPrerequisites)
{
	AActor* LevelSequenceActor = Cast<AActor>(GetOuter());
	if (LevelSequenceActor == nullptr)
	{
		return;
	}

	TArray<UMovieSceneSequence*> AllSequences;
	GetDescendantSequences(LevelSequence, AllSequences);

	for (auto Sequence : AllSequences)
	{
		UMovieScene* MovieScene = Sequence->GetMovieScene();
		if (MovieScene == nullptr)
		{
			continue;
		}

		TArray<AActor*> ControlledActors;

		for (int32 PossessableCount = 0; PossessableCount < MovieScene->GetPossessableCount(); ++PossessableCount)
		{
			FMovieScenePossessable& Possessable = MovieScene->GetPossessable(PossessableCount);
				
			UObject* PossessableObject = Sequence->FindPossessableObject(Possessable.GetGuid(), this);
			if (PossessableObject != nullptr)
			{
				AActor* PossessableActor = Cast<AActor>(PossessableObject);

				if (PossessableActor != nullptr)
				{
					ControlledActors.Add(PossessableActor);
				}
			}
		}

		for (int32 SpawnableCount = 0; SpawnableCount < MovieScene->GetSpawnableCount(); ++ SpawnableCount)
		{
			FMovieSceneSpawnable& Spawnable = MovieScene->GetSpawnable(SpawnableCount);

			UObject* SpawnableObject = Spawnable.GetObjectTemplate();
			if (SpawnableObject != nullptr)
			{
				AActor* SpawnableActor = Cast<AActor>(SpawnableObject);

				if (SpawnableActor != nullptr)
				{
					ControlledActors.Add(SpawnableActor);
				}
			}
		}


		for (AActor* ControlledActor : ControlledActors)
		{
			USceneComponent* RootComponent = ControlledActor->GetRootComponent();
			if (RootComponent)
			{
				if (bAddTickPrerequisites)
				{
					RootComponent->PrimaryComponentTick.AddPrerequisite(LevelSequenceActor, LevelSequenceActor->PrimaryActorTick);
				}
				else
				{
					RootComponent->PrimaryComponentTick.RemovePrerequisite(LevelSequenceActor, LevelSequenceActor->PrimaryActorTick);
				}
			}

			if (bAddTickPrerequisites)
			{
				ControlledActor->PrimaryActorTick.AddPrerequisite(LevelSequenceActor, LevelSequenceActor->PrimaryActorTick);
			}
			else
			{
				ControlledActor->PrimaryActorTick.RemovePrerequisite(LevelSequenceActor, LevelSequenceActor->PrimaryActorTick);
			}
		}
	}
}

void ULevelSequencePlayer::Play()
{
	// Start playing
	StartPlayingNextTick();

	// Update now
	bPendingFirstUpdate = false;
	UpdateMovieSceneInstance(TimeCursorPosition, TimeCursorPosition);
}

void ULevelSequencePlayer::PlayLooping(int32 NumLoops)
{
	PlaybackSettings.LoopCount = NumLoops;
	Play();
}

void ULevelSequencePlayer::StartPlayingNextTick()
{
	if ((LevelSequence == nullptr) || !World.IsValid())
	{
		return;
	}

	// @todo Sequencer playback: Should we recreate the instance every time?
	// We must not recreate the instance since it holds stateful information (such as which objects it has spawned). Recreating the instance would break any 
	if (!RootMovieSceneInstance.IsValid())
	{
		RootMovieSceneInstance = MakeShareable(new FMovieSceneSequenceInstance(*LevelSequence));
		RootMovieSceneInstance->RefreshInstance(*this);
	}

	SetTickPrerequisites(true);

	bPendingFirstUpdate = true;
	bIsPlaying = true;
	bHasCleanedUpSequence = false;
}

float ULevelSequencePlayer::GetPlaybackPosition() const
{
	return TimeCursorPosition;
}

void ULevelSequencePlayer::SetPlaybackPosition(float NewPlaybackPosition)
{
	UpdateTimeCursorPosition(NewPlaybackPosition);
	UpdateMovieSceneInstance(TimeCursorPosition, LastCursorPosition);
}

float ULevelSequencePlayer::GetLength() const
{
	return EndTime - StartTime;
}

float ULevelSequencePlayer::GetPlayRate() const
{
	return PlaybackSettings.PlayRate;
}

void ULevelSequencePlayer::SetPlayRate(float PlayRate)
{
	PlaybackSettings.PlayRate = PlayRate;
}

void ULevelSequencePlayer::SetPlaybackRange( const float NewStartTime, const float NewEndTime )
{
	StartTime = NewStartTime;
	EndTime = FMath::Max(NewEndTime, StartTime);

	TimeCursorPosition = FMath::Clamp(TimeCursorPosition, 0.f, GetLength());
}

void ULevelSequencePlayer::UpdateTimeCursorPosition(float NewPosition)
{
	float Length = GetLength();

	if (bPendingFirstUpdate)
	{
		NewPosition = TimeCursorPosition;
		bPendingFirstUpdate = false;
	}

	if ((NewPosition >= Length) || (NewPosition < 0))
	{
		// loop playback
		if (PlaybackSettings.LoopCount < 0 || CurrentNumLoops < PlaybackSettings.LoopCount)
		{
			++CurrentNumLoops;
			const float Overplay = FMath::Fmod(NewPosition, Length);
			TimeCursorPosition = Overplay < 0 ? Length + Overplay : Overplay;
			LastCursorPosition = TimeCursorPosition;

			SpawnRegister->ForgetExternallyOwnedSpawnedObjects(*this);

			return;
		}

		// stop playback
		bIsPlaying = false;
		CurrentNumLoops = 0;
	}

	LastCursorPosition = TimeCursorPosition;
	TimeCursorPosition = NewPosition;
}

/* ULevelSequencePlayer implementation
 *****************************************************************************/

void ULevelSequencePlayer::Initialize(ULevelSequence* InLevelSequence, UWorld* InWorld, const FLevelSequencePlaybackSettings& Settings)
{
	LevelSequence = InLevelSequence;

	World = InWorld;
	PlaybackSettings = Settings;

	if (UMovieScene* MovieScene = LevelSequence->GetMovieScene())
	{
		TRange<float> PlaybackRange = MovieScene->GetPlaybackRange();
		SetPlaybackRange(PlaybackRange.GetLowerBoundValue(), PlaybackRange.GetUpperBoundValue());
	}

	// Ensure everything is set up, ready for playback
	Stop();
}


/* IMovieScenePlayer interface
 *****************************************************************************/

void ULevelSequencePlayer::GetRuntimeObjects(TSharedRef<FMovieSceneSequenceInstance> MovieSceneInstance, const FGuid& ObjectId, TArray<TWeakObjectPtr<UObject>>& OutObjects) const
{
	UObject* FoundObject = MovieSceneInstance->FindObject(ObjectId, *this);
	if (FoundObject)
	{
		OutObjects.Add(FoundObject);
	}
}


void ULevelSequencePlayer::UpdateCameraCut(UObject* CameraObject, UObject* UnlockIfCameraObject, bool bJumpCut) const
{
	// skip missing player controller
	APlayerController* PC = World->GetGameInstance()->GetFirstLocalPlayerController();

	if (PC == nullptr)
	{
		return;
	}

	// skip same view target
	AActor* ViewTarget = PC->GetViewTarget();

	// save the last view target so that it can be restored when the camera object is null
	if (!LastViewTarget.IsValid())
	{
		LastViewTarget = ViewTarget;
	}

	if (CameraObject == ViewTarget)
	{
		if ( bJumpCut && PC->PlayerCameraManager )
		{
			PC->PlayerCameraManager->bGameCameraCutThisFrame = true;
		}
		return;
	}

	// skip unlocking if the current view target differs
	AActor* UnlockIfCameraActor = Cast<AActor>(UnlockIfCameraObject);

	if ((CameraObject == nullptr) && (UnlockIfCameraActor != ViewTarget))
	{
		return;
	}

	// override the player controller's view target
	AActor* CameraActor = Cast<AActor>(CameraObject);

	// if the camera object is null, use the last view target so that it is restored to the state before the sequence takes control
	if (CameraActor == nullptr)
	{
		CameraActor = LastViewTarget.Get();
	}

	FViewTargetTransitionParams TransitionParams;
	PC->SetViewTarget(CameraActor, TransitionParams);

	if (PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->bClientSimulatingViewTarget = (CameraActor != nullptr);
		PC->PlayerCameraManager->bGameCameraCutThisFrame = true;
	}
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

void ULevelSequencePlayer::SetPlaybackStatus(EMovieScenePlayerStatus::Type InPlaybackStatus)
{
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

UObject* ULevelSequencePlayer::GetPlaybackContext() const
{
	return World.Get();
}

TArray<UObject*> ULevelSequencePlayer::GetEventContexts() const
{
	TArray<UObject*> EventContexts;
	if (World.IsValid())
	{
		if (World->GetLevelScriptActor())
		{
			EventContexts.Add(World->GetLevelScriptActor());
		}

		for (ULevelStreaming* StreamingLevel : World->StreamingLevels)
		{
			if (StreamingLevel->GetLevelScriptActor())
			{
				EventContexts.Add(StreamingLevel->GetLevelScriptActor());
			}
		}
	}
	return EventContexts;
}

void ULevelSequencePlayer::Update(const float DeltaSeconds)
{
	if (bIsPlaying)
	{
		UpdateTimeCursorPosition(TimeCursorPosition + DeltaSeconds * PlaybackSettings.PlayRate);
		UpdateMovieSceneInstance(TimeCursorPosition, LastCursorPosition);
	}
	else if (!bHasCleanedUpSequence && TimeCursorPosition >= GetLength())
	{
		UpdateMovieSceneInstance(TimeCursorPosition, LastCursorPosition);

		bHasCleanedUpSequence = true;
		
		SpawnRegister->ForgetExternallyOwnedSpawnedObjects(*this);
		SpawnRegister->CleanUp(*this);
	}
}

void ULevelSequencePlayer::UpdateMovieSceneInstance(float CurrentPosition, float PreviousPosition)
{
	if(RootMovieSceneInstance.IsValid())
	{
		float Position = CurrentPosition + StartTime;
		float LastPosition = PreviousPosition + StartTime;
		UMovieSceneSequence* MovieSceneSequence = RootMovieSceneInstance->GetSequence();
		if (MovieSceneSequence != nullptr && 
			MovieSceneSequence->GetMovieScene()->GetForceFixedFrameIntervalPlayback() &&
			MovieSceneSequence->GetMovieScene()->GetFixedFrameInterval() > 0 )
		{
			float FixedFrameInterval = MovieSceneSequence->GetMovieScene()->GetFixedFrameInterval();
			Position = FMath::RoundToInt( Position / FixedFrameInterval ) * FixedFrameInterval;
			LastPosition = FMath::RoundToInt( LastPosition / FixedFrameInterval ) * FixedFrameInterval;
		}
		EMovieSceneUpdateData UpdateData(Position, LastPosition);
		RootMovieSceneInstance->Update(UpdateData, *this);
#if WITH_EDITOR
		OnLevelSequencePlayerUpdate.Broadcast(*this, CurrentPosition, PreviousPosition);
#endif
	}
}

IMovieSceneSpawnRegister& ULevelSequencePlayer::GetSpawnRegister()
{
	return *SpawnRegister;
}