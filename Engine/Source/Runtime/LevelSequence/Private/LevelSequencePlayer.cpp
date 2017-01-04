// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "LevelSequencePlayer.h"
#include "GameFramework/Actor.h"
#include "MovieScene.h"
#include "Misc/CoreDelegates.h"
#include "EngineGlobals.h"
#include "Camera/PlayerCameraManager.h"
#include "UObject/Package.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "Tickable.h"
#include "Engine/LevelScriptActor.h"
#include "MovieSceneCommonHelpers.h"
#include "Sections/MovieSceneSubSection.h"
#include "LevelSequenceSpawnRegister.h"
#include "Engine/Engine.h"
#include "Engine/LevelStreaming.h"
#include "Tracks/MovieSceneCinematicShotTrack.h"
#include "Sections/MovieSceneCinematicShotSection.h"
#include "LevelSequenceActor.h"


/* ULevelSequencePlayer structors
 *****************************************************************************/

ULevelSequencePlayer::ULevelSequencePlayer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SpawnRegister = MakeShareable(new FLevelSequenceSpawnRegister);
}


/* ULevelSequencePlayer interface
 *****************************************************************************/

ULevelSequencePlayer* ULevelSequencePlayer::CreateLevelSequencePlayer(UObject* WorldContextObject, ULevelSequence* InLevelSequence, FMovieSceneSequencePlaybackSettings Settings)
{
	if (InLevelSequence == nullptr)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
	check(World != nullptr);

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.ObjectFlags |= RF_Transient;
	SpawnParams.bAllowDuringConstructionScript = true;
	ALevelSequenceActor* Actor = World->SpawnActor<ALevelSequenceActor>(SpawnParams);

	Actor->PlaybackSettings = Settings;
	Actor->LevelSequence = InLevelSequence;

	Actor->InitializePlayer();

	return Actor->SequencePlayer;
}

void ULevelSequencePlayer::SetTickPrerequisites(bool bAddTickPrerequisites)
{
	// @todo: this should happen procedurally, rather than up front. The current approach will not scale well.
	SetTickPrerequisites(MovieSceneSequenceID::Root, Sequence, bAddTickPrerequisites);
	for (auto& Pair : RootTemplateInstance.GetHierarchy().AllSubSequenceData())
	{
		SetTickPrerequisites(Pair.Key, Pair.Value.Sequence, bAddTickPrerequisites);
	}
}
void ULevelSequencePlayer::SetTickPrerequisites(FMovieSceneSequenceID SequenceID, UMovieSceneSequence* InSequence, bool bAddTickPrerequisites)
{
	AActor* LevelSequenceActor = Cast<AActor>(GetOuter());
	if (LevelSequenceActor == nullptr)
	{
		return;
	}

	UMovieScene* MovieScene = InSequence ? InSequence->GetMovieScene() : nullptr;
	if (!MovieScene)
	{
		return;
	}

	TArray<AActor*> ControlledActors;

	for (int32 PossessableCount = 0; PossessableCount < MovieScene->GetPossessableCount(); ++PossessableCount)
	{
		FMovieScenePossessable& Possessable = MovieScene->GetPossessable(PossessableCount);

		for (TWeakObjectPtr<> PossessableObject : FindBoundObjects(Possessable.GetGuid(), SequenceID))
		{
			AActor* PossessableActor = Cast<AActor>(PossessableObject.Get());
			if (PossessableActor != nullptr)
			{
				ControlledActors.Add(PossessableActor);
			}
		}
	}

	// @todo: should this happen on spawn, not here?
	// @todo: does setting tick prerequisites on spawnable *templates* actually propagate to spawned instances?
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
		for( UActorComponent* Component : ControlledActor->GetComponents() )
		{
			if (bAddTickPrerequisites)
			{
				Component->PrimaryComponentTick.AddPrerequisite(LevelSequenceActor, LevelSequenceActor->PrimaryActorTick);
			}
			else
			{
				Component->PrimaryComponentTick.RemovePrerequisite(LevelSequenceActor, LevelSequenceActor->PrimaryActorTick);
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

/* ULevelSequencePlayer implementation
 *****************************************************************************/

void ULevelSequencePlayer::Initialize(ULevelSequence* InLevelSequence, UWorld* InWorld, const FMovieSceneSequencePlaybackSettings& Settings)
{
	World = InWorld;
	UMovieSceneSequencePlayer::Initialize(InLevelSequence, Settings);
}

bool ULevelSequencePlayer::CanPlay() const
{
	return World.IsValid();
}

void ULevelSequencePlayer::OnStartedPlaying()
{
	SetTickPrerequisites(true);
}

void ULevelSequencePlayer::OnStopped()
{
	SetTickPrerequisites(false);
}

/* IMovieScenePlayer interface
 *****************************************************************************/

void ULevelSequencePlayer::UpdateCameraCut(UObject* CameraObject, UObject* UnlockIfCameraObject, bool bJumpCut)
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

	UCameraComponent* CameraComponent = MovieSceneHelpers::CameraComponentFromRuntimeObject(CameraObject);
	
	CachedCameraComponent = CameraComponent;

	if (CameraObject == ViewTarget)
	{
		if ( bJumpCut )
		{
			if (PC->PlayerCameraManager)
			{
				PC->PlayerCameraManager->bGameCameraCutThisFrame = true;
			}

			if (CameraComponent)
			{
				CameraComponent->NotifyCameraCut();
			}
		}
		return;
	}

	// skip unlocking if the current view target differs
	AActor* UnlockIfCameraActor = Cast<AActor>(UnlockIfCameraObject);

	// if unlockIfCameraActor is valid, release lock if currently locked to object
	if (CameraObject == nullptr && UnlockIfCameraActor != nullptr && UnlockIfCameraActor != ViewTarget)
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

	if (CameraComponent)
	{
		CameraComponent->NotifyCameraCut();
	}

	if (PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->bClientSimulatingViewTarget = (CameraActor != nullptr);
		PC->PlayerCameraManager->bGameCameraCutThisFrame = true;
	}
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
		GetEventContexts(*World, EventContexts);
	}
	return EventContexts;
}

void ULevelSequencePlayer::GetEventContexts(UWorld& InWorld, TArray<UObject*>& OutContexts)
{
	if (InWorld.GetLevelScriptActor())
	{
		OutContexts.Add(InWorld.GetLevelScriptActor());
	}

	for (ULevelStreaming* StreamingLevel : InWorld.StreamingLevels)
	{
		if (StreamingLevel->GetLevelScriptActor())
		{
			OutContexts.Add(StreamingLevel->GetLevelScriptActor());
		}
	}
}

void ULevelSequencePlayer::TakeFrameSnapshot(FLevelSequencePlayerSnapshot& OutSnapshot) const
{
	if (!ensure(Sequence))
	{
		return;
	}
	
	const float CurrentTime = StartTime + TimeCursorPosition;

	OutSnapshot.Settings = SnapshotSettings;

	OutSnapshot.MasterTime = CurrentTime;
	OutSnapshot.MasterName = FText::FromString(Sequence->GetName());

	OutSnapshot.CurrentShotName = OutSnapshot.MasterName;
	OutSnapshot.CurrentShotLocalTime = CurrentTime;
	OutSnapshot.CameraComponent = CachedCameraComponent.IsValid() ? CachedCameraComponent.Get() : nullptr;

	UMovieSceneCinematicShotTrack* ShotTrack = Sequence->GetMovieScene()->FindMasterTrack<UMovieSceneCinematicShotTrack>();
	if (ShotTrack)
	{
		int32 HighestRow = TNumericLimits<int32>::Max();
		UMovieSceneCinematicShotSection* ActiveShot = nullptr;
		for (UMovieSceneSection* Section : ShotTrack->GetAllSections())
		{
			if (!ensure(Section))
			{
				continue;
			}

			if (Section->IsActive() && Section->GetRange().Contains(CurrentTime) && (!ActiveShot || Section->GetRowIndex() < ActiveShot->GetRowIndex()))
			{
				ActiveShot = Cast<UMovieSceneCinematicShotSection>(Section);
			}
		}

		if (ActiveShot)
		{
			// Assume that shots with no sequence start at 0.
			const float ShotLowerBound = ActiveShot->GetSequence() 
				? ActiveShot->GetSequence()->GetMovieScene()->GetPlaybackRange().GetLowerBoundValue() 
				: 0;
			const float ShotOffset = ActiveShot->Parameters.StartOffset + ShotLowerBound - ActiveShot->Parameters.PrerollTime;
			const float ShotPosition = ShotOffset + (CurrentTime - (ActiveShot->GetStartTime() - ActiveShot->Parameters.PrerollTime)) / ActiveShot->Parameters.TimeScale;

			OutSnapshot.CurrentShotName = ActiveShot->GetShotDisplayName();
			OutSnapshot.CurrentShotLocalTime = ShotPosition;
		}
	}
}

