// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SequenceRecorderPrivatePCH.h"
#include "SequenceRecorder.h"
#include "ActorRecording.h"
#include "SequenceRecorderUtils.h"
#include "SequenceRecorderSettings.h"
#include "ObjectTools.h"
#include "ScopedTransaction.h"
#include "AssetRegistryModule.h"
#include "SequenceRecorderActorFilter.h"
#include "Engine/DemoNetDriver.h"

FSequenceRecorder::FSequenceRecorder()
	: bQueuedRecordingsDirty(false)
	, CurrentDelay(0.0f)
	, CurrentTime(0.0f)
{
}

FSequenceRecorder& FSequenceRecorder::Get()
{
	static FSequenceRecorder SequenceRecorder;
	return SequenceRecorder;
}


bool FSequenceRecorder::IsRecordingQueued(AActor* Actor) const
{
	for (UActorRecording* QueuedRecording : QueuedRecordings)
	{
		if (QueuedRecording->ActorToRecord == Actor)
		{
			return true;
		}
	}

	return false;
}

UActorRecording* FSequenceRecorder::FindRecording(AActor* Actor) const
{
	for (UActorRecording* QueuedRecording : QueuedRecordings)
	{
		if (QueuedRecording->ActorToRecord == Actor)
		{
			return QueuedRecording;
		}
	}

	return nullptr;
}

void FSequenceRecorder::StartAllQueuedRecordings()
{
	for (UActorRecording* QueuedRecording : QueuedRecordings)
	{
		QueuedRecording->StartRecording(CurrentSequence.Get(), CurrentTime);
	}
}

void FSequenceRecorder::StopAllQueuedRecordings()
{
	for (UActorRecording* QueuedRecording : QueuedRecordings)
	{
		QueuedRecording->StopRecording(CurrentSequence.Get());
	}
}

UActorRecording* FSequenceRecorder::AddNewQueuedRecording(AActor* Actor, UAnimSequence* AnimSequence, float Length)
{
	const USequenceRecorderSettings* Settings = GetDefault<USequenceRecorderSettings>();

	UActorRecording* ActorRecording = NewObject<UActorRecording>();
	ActorRecording->AddToRoot();
	ActorRecording->ActorToRecord = Actor;
	ActorRecording->TargetAnimation = AnimSequence;
	ActorRecording->AnimationSettings.Length = Length;

	// We always record in world space as we need animations to record root motion
	ActorRecording->AnimationSettings.bRecordInWorldSpace = true;
	ActorRecording->ActorSettings.bRecordTransforms = true;

	QueuedRecordings.Add(ActorRecording);

	bQueuedRecordingsDirty = true;

	return ActorRecording;
}

void FSequenceRecorder::RemoveQueuedRecording(AActor* Actor)
{
	for (UActorRecording* QueuedRecording : QueuedRecordings)
	{
		if (QueuedRecording->ActorToRecord == Actor)
		{
			QueuedRecording->RemoveFromRoot();
			QueuedRecordings.Remove(QueuedRecording);
			break;
		}
	}

	bQueuedRecordingsDirty = true;
}

void FSequenceRecorder::RemoveQueuedRecording(class UActorRecording* Recording)
{
	for (UActorRecording* QueuedRecording : QueuedRecordings)
	{
		if (QueuedRecording == Recording)
		{
			QueuedRecording->RemoveFromRoot();
			QueuedRecordings.Remove(QueuedRecording);
			break;
		}
	}

	bQueuedRecordingsDirty = true;
}

bool FSequenceRecorder::HasQueuedRecordings() const
{
	return QueuedRecordings.Num() > 0;
}


void FSequenceRecorder::Tick(float DeltaSeconds)
{
	// if a replay recording is in progress and channels are paused, wait until we have data again before recording
	if(CurrentReplayWorld.IsValid() && CurrentReplayWorld->DemoNetDriver != nullptr)
	{
		if(CurrentReplayWorld->DemoNetDriver->bChannelsArePaused)
		{
			return;
		}
	}

	const USequenceRecorderSettings* Settings = GetDefault<USequenceRecorderSettings>();

	// check for spawned actors and if they have begun playing yet
	for(TWeakObjectPtr<AActor>& QueuedSpawnedActor : QueuedSpawnedActors)
	{
		if(AActor* Actor = QueuedSpawnedActor.Get())
		{
			if(Actor->HasActorBegunPlay())
			{
				if(UActorRecording::IsRelevantForRecording(Actor) && IsActorValidForRecording(Actor))
				{
					UActorRecording* NewRecording = AddNewQueuedRecording(Actor);
					NewRecording->bWasSpawnedPostRecord = true;
					NewRecording->StartRecording(CurrentSequence.Get(), CurrentTime);
				}

				QueuedSpawnedActor.Reset();
			}
		}
	}

	QueuedSpawnedActors.RemoveAll([](TWeakObjectPtr<AActor>& QueuedSpawnedActor) { return !QueuedSpawnedActor.IsValid(); } );

	FAnimationRecorderManager::Get().Tick(DeltaSeconds);

	for(UActorRecording* Recording : QueuedRecordings)
	{
		Recording->Tick(DeltaSeconds, CurrentSequence.Get(), CurrentTime);
	}

	if(CurrentDelay > 0.0f)
	{
		CurrentDelay -= DeltaSeconds;
		if(CurrentDelay <= 0.0f)
		{
			CurrentDelay = 0.0f;
			StartRecordingInternal(nullptr);
		}
	}

	if(Settings->bCreateLevelSequence && CurrentSequence.IsValid())
	{
		CurrentTime += DeltaSeconds;

		// check if all our actor recordings are finished or we timed out
		if(QueuedRecordings.Num() > 0)
		{
			bool bAllFinished = true;
			for(UActorRecording* Recording : QueuedRecordings)
			{
				if(Recording->IsRecording())
				{
					bAllFinished = false;
					break;
				}
			}

			if(bAllFinished || (Settings->SequenceLength > 0.0f && CurrentTime >= Settings->SequenceLength))
			{
				StopRecording();
			}
		}

		auto RemoveDeadActorPredicate = 
			[&](UActorRecording* Recording)
			{
				if(!Recording->ActorToRecord.IsValid())
				{
					DeadRecordings.Add(Recording);
					return true;
				}
				return false; 
			};

		// remove all dead actors
		int32 Removed = QueuedRecordings.RemoveAll(RemoveDeadActorPredicate);
		if(Removed > 0)
		{
			bQueuedRecordingsDirty = true;
		}
	}
}

bool FSequenceRecorder::StartRecording()
{
	const USequenceRecorderSettings* Settings = GetDefault<USequenceRecorderSettings>();

	CurrentTime = 0.0f;

	if(Settings->RecordingDelay > 0.0f)
	{
		CurrentDelay = Settings->RecordingDelay;
		
		return QueuedRecordings.Num() > 0;
	}
	
	return StartRecordingInternal(nullptr);
}

bool FSequenceRecorder::StartRecordingForReplay(UWorld* World, const FSequenceRecorderActorFilter& ActorFilter)
{
	// set up our recording settings
	USequenceRecorderSettings* Settings = GetMutableDefault<USequenceRecorderSettings>();
	
	Settings->bCreateLevelSequence = true;
	Settings->SequenceLength = 0.0f;
	Settings->RecordingDelay = 0.0f;
	Settings->bRecordNearbySpawnedActors = true;
	Settings->NearbyActorRecordingProximity = 0.0f;
	Settings->bRecordWorldSettingsActor = true;
	Settings->ActorFilter = ActorFilter;

	CurrentReplayWorld = World;
	
	return StartRecordingInternal(World);
}

bool FSequenceRecorder::StartRecordingInternal(UWorld* World)
{
	CurrentTime = 0.0f;

	const USequenceRecorderSettings* Settings = GetDefault<USequenceRecorderSettings>();

	if(Settings->bRecordWorldSettingsActor)
	{
		if(World != nullptr || (QueuedRecordings.Num() > 0 && QueuedRecordings[0]->ActorToRecord.IsValid()))
		{
			UWorld* ActorWorld = World != nullptr ? World : QueuedRecordings[0]->ActorToRecord->GetWorld();
			if(ActorWorld)
			{
				AWorldSettings* WorldSettings = ActorWorld->GetWorldSettings();
				AddNewQueuedRecording(WorldSettings);
			}
		}
	}

	if(QueuedRecordings.Num() > 0)
	{
		ULevelSequence* LevelSequence = nullptr;
		
		if(Settings->bCreateLevelSequence)
		{
			// build an asset path
			FString AssetPath(TEXT("/Game"));
			AssetPath /= Settings->SequenceRecordingBasePath.Path;

			FString AssetName = Settings->SequenceName.Len() > 0 ? Settings->SequenceName : TEXT("RecordedSequence");

			LevelSequence = SequenceRecorderUtils::MakeNewAsset<ULevelSequence>(AssetPath, AssetName);
			if(LevelSequence)
			{
				LevelSequence->Initialize();
				CurrentSequence = LevelSequence;

				FAssetRegistryModule::AssetCreated(LevelSequence);
			}
		}

		// register for spawning delegate in the world(s) of recorded actors
		for(UActorRecording* Recording : QueuedRecordings)
		{
			if(Recording->ActorToRecord.IsValid())
			{
				UWorld* ActorWorld = Recording->ActorToRecord->GetWorld();
				if(ActorWorld != nullptr)
				{
					FDelegateHandle* FoundHandle = ActorSpawningDelegateHandles.Find(ActorWorld);
					if(FoundHandle == nullptr)
					{
						FDelegateHandle NewHandle = ActorWorld->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateStatic(&FSequenceRecorder::HandleActorSpawned));
						ActorSpawningDelegateHandles.Add(ActorWorld, NewHandle);
					}
				}
			}
		}

		// start recording 
		bool bStartedRecordingAllActors = true;
		for(UActorRecording* Recording : QueuedRecordings)
		{
			if(!Recording->StartRecording(CurrentSequence.Get(), CurrentTime))
			{
				bStartedRecordingAllActors = false;
				break;
			}
		}

		if(!bStartedRecordingAllActors)
		{
			// if we couldnt start a recording, stop all others
			TArray<FAssetData> AssetsToCleanUp;
			if(LevelSequence)
			{
				AssetsToCleanUp.Add(LevelSequence);
			}

			for(UActorRecording* Recording : QueuedRecordings)
			{
				Recording->StopRecording(CurrentSequence.Get());
			}

			// clean up any assets that we can
			if(AssetsToCleanUp.Num() > 0)
			{
				ObjectTools::DeleteAssets(AssetsToCleanUp, false);
			}
		}

		return true;
	}

	return false;
} 

bool FSequenceRecorder::StopRecording()
{
	CurrentDelay = 0.0f;

	// remove spawn delegates
	for(auto It = ActorSpawningDelegateHandles.CreateConstIterator(); It; ++It)
	{
		TWeakObjectPtr<UWorld> World = It->Key;
		if(World.IsValid())
		{
			World->RemoveOnActorSpawnedHandler(It->Value);
		}
	}

	ActorSpawningDelegateHandles.Empty();

	// also stop all dead animation recordings, i.e. ones that use GCed components
	FAnimationRecorderManager::Get().StopRecordingDeadAnimations();

	for(UActorRecording* Recording : QueuedRecordings)
	{
		Recording->StopRecording(CurrentSequence.Get());
	}

	for(UActorRecording* Recording : DeadRecordings)
	{
		Recording->StopRecording(CurrentSequence.Get());
	}

	DeadRecordings.Empty();

	const USequenceRecorderSettings* Settings = GetDefault<USequenceRecorderSettings>();
	if(Settings->bCreateLevelSequence)
	{
		if(CurrentSequence.IsValid())
		{
			ULevelSequence* LevelSequence = CurrentSequence.Get();

			// Stop referencing the sequence so we are listed as 'not recording'
			CurrentSequence = nullptr;

			// set movie scene playback range to encompass all sections
			UMovieScene* MovieScene = LevelSequence->GetMovieScene();
			if(MovieScene)
			{
				float MaxRange = 0.0f;
				float MinRange = 0.0f;

				TArray<UMovieSceneSection*> MovieSceneSections = MovieScene->GetAllSections();
				for(UMovieSceneSection* Section : MovieSceneSections)
				{
					MaxRange = FMath::Max(MaxRange, Section->GetEndTime());
					MinRange = FMath::Min(MinRange, Section->GetStartTime());
				}

				MovieScene->SetPlaybackRange(MinRange, MaxRange);
			}

			// @todo: post-process sequence to remove redundant tracks (empty transforms etc.)

			return true;
		}
	}
	else
	{
		return true;
	}

	return false;
}

bool FSequenceRecorder::IsDelaying() const
{
	return CurrentDelay > 0.0f;
}

float FSequenceRecorder::GetCurrentDelay() const
{
	return CurrentDelay;
}

bool FSequenceRecorder::IsActorValidForRecording(AActor* Actor)
{
	const USequenceRecorderSettings* Settings = GetDefault<USequenceRecorderSettings>();

	float Distance = Settings->NearbyActorRecordingProximity;

	// check distance if valid
	if(Distance > 0.0f)
	{
		const FTransform ActorTransform = Actor->GetTransform();
		const FVector ActorTranslation = ActorTransform.GetTranslation();

		for(UActorRecording* Recording : QueuedRecordings)
		{
			if(AActor* OtherActor = Recording->ActorToRecord.Get())
			{
				if(OtherActor != Actor)
				{
					const FTransform OtherActorTransform = OtherActor->GetTransform();
					const FVector OtherActorTranslation = OtherActorTransform.GetTranslation();

					if((OtherActorTranslation - ActorTranslation).Size() < Distance)
					{
						return true;
					}
				}
			}
		}
	}

	// check class if any
	for(const TSubclassOf<AActor>& ActorClass : Settings->ActorFilter.ActorClassesToRecord)
	{
		if(Actor->IsA(*ActorClass))
		{
			return true;
		}
	}

	return false;
}

void FSequenceRecorder::HandleActorSpawned(AActor* Actor)
{
	const USequenceRecorderSettings* Settings = GetDefault<USequenceRecorderSettings>();

	if(Actor && FSequenceRecorder::Get().IsRecording() && Settings->bRecordNearbySpawnedActors)
	{
		// Queue a possible recording - we need to wait until the actor has begun playing
		FSequenceRecorder::Get().QueuedSpawnedActors.Add(Actor);
	}
}