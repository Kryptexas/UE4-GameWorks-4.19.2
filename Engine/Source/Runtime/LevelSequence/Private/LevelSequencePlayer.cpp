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
	, LevelSequenceInstance(nullptr)
	, bIsPlaying(false)
	, TimeCursorPosition(0.0f)
	, CurrentNumLoops(0)
{ }


/* ULevelSequencePlayer interface
 *****************************************************************************/

ULevelSequencePlayer* ULevelSequencePlayer::CreateLevelSequencePlayer(UObject* WorldContextObject, ULevelSequence* LevelSequence, FLevelSequencePlaybackSettings Settings)
{
	if (LevelSequence == nullptr)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
	check(World != nullptr);

	ULevelSequencePlayer* NewPlayer = NewObject<ULevelSequencePlayer>(GetTransientPackage(), NAME_None, RF_Transient);
	check(NewPlayer != nullptr);

	// Set up a new instance of the level sequence for the player
	ULevelSequenceInstance* Instance = NewObject<ULevelSequenceInstance>(NewPlayer);
	Instance->Initialize(LevelSequence, World, false);

	NewPlayer->Initialize(Instance, World, Settings);

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
	if ((LevelSequenceInstance == nullptr) || !World.IsValid())
	{
		return;
	}

	// @todo Sequencer playback: Should we recreate the instance every time?
	RootMovieSceneInstance = MakeShareable(new FMovieSceneSequenceInstance(*LevelSequenceInstance));

	// @odo Sequencer Should we spawn actors here?
	SpawnActorsForMovie(RootMovieSceneInstance.ToSharedRef());
	RootMovieSceneInstance->RefreshInstance(*this);

	bIsPlaying = true;

	if (RootMovieSceneInstance.IsValid())
	{
		RootMovieSceneInstance->Update(TimeCursorPosition, TimeCursorPosition, *this);
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
		RootMovieSceneInstance->Update(TimeCursorPosition, LastTimePosition, *this);
	}
}

float ULevelSequencePlayer::GetLength() const
{
	if (!LevelSequenceInstance)
	{
		return 0;
	}

	UMovieScene* MovieScene = LevelSequenceInstance->GetMovieScene();
	return MovieScene ? MovieScene->GetTimeRange().Size<float>() : 0;
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
	if (TimeCursorPosition > Length || TimeCursorPosition < 0)
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

void ULevelSequencePlayer::Initialize(ULevelSequenceInstance* InLevelSequenceInstance, UWorld* InWorld, const FLevelSequencePlaybackSettings& Settings)
{
	LevelSequenceInstance = InLevelSequenceInstance;

	World = InWorld;
	PlaybackSettings = Settings;

	// Ensure everything is set up, ready for playback
	Stop();
}


/* IMovieScenePlayer interface
 *****************************************************************************/

void ULevelSequencePlayer::SpawnActorsForMovie(TSharedRef<FMovieSceneSequenceInstance> MovieSceneInstance)
{
	UWorld* WorldPtr = World.Get();

	if (WorldPtr == nullptr)
	{
		return;
	}

	UMovieScene* MovieScene = MovieSceneInstance->GetSequence()->GetMovieScene();

	if (MovieScene == nullptr)
	{
		return;
	}

	TArray<FSpawnedActorInfo>* FoundSpawnedActors = InstanceToSpawnedActorMap.Find(MovieSceneInstance);

	if (FoundSpawnedActors != nullptr)
	{
		// Remove existing spawned actors for this movie
		DestroyActorsForMovie( MovieSceneInstance );
	}

	TArray<FSpawnedActorInfo>& SpawnedActorList = InstanceToSpawnedActorMap.Add(MovieSceneInstance, TArray<FSpawnedActorInfo>());

	for (auto SpawnableIndex = 0; SpawnableIndex < MovieScene->GetSpawnableCount(); ++SpawnableIndex)
	{
		auto& Spawnable = MovieScene->GetSpawnable(SpawnableIndex);
		UClass* GeneratedClass = Spawnable.GetClass();
		
		if ((GeneratedClass == nullptr) || !GeneratedClass->IsChildOf(AActor::StaticClass()))
		{
			continue;
		}

		AActor* ActorCDO = CastChecked<AActor>(GeneratedClass->ClassDefaultObject);
		const FVector SpawnLocation = ActorCDO->GetRootComponent()->RelativeLocation;
		const FRotator SpawnRotation = ActorCDO->GetRootComponent()->RelativeRotation;

		FActorSpawnParameters SpawnInfo;
		{
			SpawnInfo.ObjectFlags = RF_NoFlags;
		}

		AActor* NewActor = WorldPtr->SpawnActor(GeneratedClass, &SpawnLocation, &SpawnRotation, SpawnInfo);

		if (NewActor)
		{	
			// Actor was spawned OK!
			FSpawnedActorInfo NewInfo;
			{
				NewInfo.RuntimeGuid = Spawnable.GetGuid();
				NewInfo.SpawnedActor = NewActor;
			}
						
			SpawnedActorList.Add(NewInfo);
		}
	}
}


void ULevelSequencePlayer::DestroyActorsForMovie(TSharedRef<FMovieSceneSequenceInstance> MovieSceneInstance)
{
	UWorld* WorldPtr = World.Get();

	if (WorldPtr == nullptr)
	{
		return;
	}

	TArray<FSpawnedActorInfo>* SpawnedActors = InstanceToSpawnedActorMap.Find(MovieSceneInstance);

	if (SpawnedActors == nullptr)
	{
		return;
	}

	TArray<FSpawnedActorInfo>& SpawnedActorsRef = *SpawnedActors;

	for(int32 ActorIndex = 0; ActorIndex < SpawnedActors->Num(); ++ActorIndex)
	{
		AActor* Actor = SpawnedActorsRef[ActorIndex].SpawnedActor.Get();
		
		if (Actor != nullptr)
		{
			// @todo Sequencer figure this out.  Defaults to false.
			const bool bNetForce = false;

			// At runtime, level modification is not needed
			const bool bShouldModifyLevel = false;
			Actor->GetWorld()->DestroyActor(Actor, bNetForce, bShouldModifyLevel);
		}
	}

	InstanceToSpawnedActorMap.Remove(MovieSceneInstance);
}


void ULevelSequencePlayer::GetRuntimeObjects(TSharedRef<FMovieSceneSequenceInstance> MovieSceneInstance, const FGuid& ObjectId, TArray<UObject*>& OutObjects) const
{
	// @todo sequencer runtime: Add support for individually spawning actors on demand when first requested?
	//    This may be important to reduce the up-front hitch when spawning actors for the entire movie, but
	//    may introduce smaller hitches during playback.  Needs experimentation.

	const TArray<FSpawnedActorInfo>* SpawnedActors = InstanceToSpawnedActorMap.Find(MovieSceneInstance);

	if ((SpawnedActors != nullptr) && (SpawnedActors->Num() > 0))
	{
		// we have a spawned actor for this handle
		const TArray<FSpawnedActorInfo>& SpawnedActorInfoRef = *SpawnedActors;

		for (int32 SpawnedActorIndex = 0; SpawnedActorIndex < SpawnedActorInfoRef.Num(); ++SpawnedActorIndex)
		{
			const FSpawnedActorInfo ActorInfo = SpawnedActorInfoRef[SpawnedActorIndex];

			if ((ActorInfo.RuntimeGuid == ObjectId) && ActorInfo.SpawnedActor.IsValid())
			{
				OutObjects.Add(ActorInfo.SpawnedActor.Get());
			}
		}
	}
	else
	{
		// otherwise, check whether we have one or more possessed actors that are mapped to this handle
		UObject* FoundObject = LevelSequenceInstance->FindObject(ObjectId);

		if (FoundObject != nullptr)
		{
			OutObjects.Add(FoundObject);
		}
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
	SpawnActorsForMovie( InstanceToAdd );
}


void ULevelSequencePlayer::RemoveMovieSceneInstance(UMovieSceneSection& MovieSceneSection, TSharedRef<FMovieSceneSequenceInstance> InstanceToRemove)
{
	const bool bDestroyAll = true;
	DestroyActorsForMovie( InstanceToRemove );
}

TSharedRef<FMovieSceneSequenceInstance> ULevelSequencePlayer::GetRootMovieSceneSequenceInstance() const
{
	return RootMovieSceneInstance.ToSharedRef();
}

void ULevelSequencePlayer::Update(const float DeltaSeconds)
{
	float LastTimePosition = TimeCursorPosition;

	if (bIsPlaying)
	{
		TimeCursorPosition += DeltaSeconds * PlaybackSettings.PlayRate;
		OnCursorPositionChanged();
	}

	if(RootMovieSceneInstance.IsValid())
	{
		RootMovieSceneInstance->Update(TimeCursorPosition, LastTimePosition, *this);
	}
}
