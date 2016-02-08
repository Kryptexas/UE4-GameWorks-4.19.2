// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SequenceRecorderPrivatePCH.h"
#include "SequenceRecorder.h"
#include "ActorRecording.h"
#include "SequenceRecorderUtils.h"
#include "SequenceRecorderSettings.h"
#include "ObjectTools.h"
#include "LevelSequence.h"
#include "AssetSelection.h"
#include "MovieSceneSkeletalAnimationTrack.h"
#include "MovieSceneSkeletalAnimationSection.h"
#include "MovieSceneSpawnTrack.h"
#include "MovieScene3DTransformTrack.h"
#include "MovieScene3DTransformSection.h"
#include "MovieSceneBoolSection.h"
#include "ScopedTransaction.h"
#include "AssetRegistryModule.h"
#include "IMovieScenePropertyRecorder.h"
#include "MovieScene3DTransformPropertyRecorder.h"
#include "MovieSceneVisibilityPropertyRecorder.h"

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

void FSequenceRecorder::StartAllQueuedRecordings()
{
	for (UActorRecording* QueuedRecording : QueuedRecordings)
	{
		QueuedRecording->StartRecording();
	}
}

void FSequenceRecorder::StopAllQueuedRecordings()
{
	for (UActorRecording* QueuedRecording : QueuedRecordings)
	{
		QueuedRecording->StopRecording();
	}
}

void FSequenceRecorder::AddNewQueuedRecording(AActor* Actor, UAnimSequence* AnimSequence, float Length)
{
	const USequenceRecorderSettings* Settings = GetDefault<USequenceRecorderSettings>();

	UActorRecording* ActorRecording = NewObject<UActorRecording>();
	ActorRecording->AddToRoot();
	ActorRecording->ActorToRecord = Actor;
	ActorRecording->TargetAnimation = AnimSequence;
	ActorRecording->AnimationSettings.Length = Length;
	ActorRecording->AnimationSettings.bRecordInWorldSpace = !Settings->bRecordTransformsAsSeperateTrack;
	ActorRecording->ActorSettings.bRecordTransforms = Settings->bRecordTransformsAsSeperateTrack;

	QueuedRecordings.Add(ActorRecording);

	bQueuedRecordingsDirty = true;
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
	FAnimationRecorderManager::Get().Tick(DeltaSeconds);

	for(UActorRecording* Recording : QueuedRecordings)
	{
		Recording->Tick(DeltaSeconds);
	}

	if(CurrentDelay > 0.0f)
	{
		CurrentDelay -= DeltaSeconds;
		if(CurrentDelay <= 0.0f)
		{
			CurrentDelay = 0.0f;
			StartRecordingInternal();
		}
	}

	const USequenceRecorderSettings* Settings = GetDefault<USequenceRecorderSettings>();
	if(Settings->bCreateLevelSequence && CurrentSequence.IsValid())
	{
		// record our property recorders
		for(auto& PropertyRecorder : PropertyRecorders)
		{
			PropertyRecorder->Record(CurrentTime);
		}

		CurrentTime += DeltaSeconds;

		// check if all our actor recordings are finished
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

			if(bAllFinished)
			{
				StopRecording();
			}
		}
	}
}

bool FSequenceRecorder::StartRecording()
{
	const USequenceRecorderSettings* Settings = GetDefault<USequenceRecorderSettings>();

	if(Settings->RecordingDelay > 0.0f)
	{
		CurrentDelay = Settings->RecordingDelay;
		return QueuedRecordings.Num() > 0;
	}
	
	return StartRecordingInternal();
}

bool FSequenceRecorder::StartRecordingInternal()
{
	CurrentTime = 0.0f;

	const USequenceRecorderSettings* Settings = GetDefault<USequenceRecorderSettings>();
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
			}
		}

		// start recording 
		bool bStartedRecordingAllActors = true;
		for(UActorRecording* Recording : QueuedRecordings)
		{
			if(!Recording->StartRecording())
			{
				bStartedRecordingAllActors = false;
				break;
			}

			// set up our spawnable for this actor
			UMovieScene* MovieScene = LevelSequence->GetMovieScene();
			UAnimSequence* AnimSequence = Recording->LastRecordedAnimation.Get();
			UActorFactory* FactoryToUse = FActorFactoryAssetProxy::GetFactoryForAssetObject(AnimSequence);
			check(FactoryToUse);

			FString BlueprintName = AnimSequence->GetName();
			auto DuplName = [&](FMovieSceneSpawnable& InSpawnable)
			{
				return InSpawnable.GetName() == BlueprintName;
			};

			int32 Index = 2;
			FString UniqueString;
			while (MovieScene->FindSpawnable(DuplName))
			{
				BlueprintName.RemoveFromEnd(UniqueString);
				UniqueString = FString::Printf(TEXT(" (%d)"), Index);
				BlueprintName += UniqueString;
			}

			UBlueprint* Blueprint = FactoryToUse->CreateBlueprint(AnimSequence, MovieScene, *BlueprintName);
			check(Blueprint);

			Recording->Guid = MovieScene->AddSpawnable(BlueprintName, Blueprint);
			
			// add the anim track
			UMovieSceneSkeletalAnimationTrack* AnimTrack = MovieScene->AddTrack<UMovieSceneSkeletalAnimationTrack>(Recording->Guid);
			if(AnimTrack)
			{
				AnimTrack->AddNewAnimation(0.0f, AnimSequence);
			}

			// now add tracks to record

			// we need to create a transform track even if we arent recording transforms
			TSharedPtr<FMovieScene3DTransformPropertyRecorder> TransformRecorder = MakeShareable(new FMovieScene3DTransformPropertyRecorder);
			TransformRecorder->CreateSection(Recording->ActorToRecord.Get(), MovieScene, Recording->Guid, Recording->ActorSettings.bRecordTransforms);
			PropertyRecorders.Add(TransformRecorder);

			// add a visibility track 
			TSharedPtr<FMovieSceneVisibilityPropertyRecorder> VisibilityRecorder = MakeShareable(new FMovieSceneVisibilityPropertyRecorder);
			VisibilityRecorder->CreateSection(Recording->ActorToRecord.Get(), MovieScene, Recording->Guid ,Recording->ActorSettings.bRecordVisibility);
			PropertyRecorders.Add(VisibilityRecorder);
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
				Recording->StopRecording();
				if(Recording->LastRecordedAnimation.IsValid())
				{
					AssetsToCleanUp.Add(FAssetData(Recording->LastRecordedAnimation.Get()));
				}
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

	for(UActorRecording* Recording : QueuedRecordings)
	{
		Recording->StopRecording();
	}

	// also stop all dead animation recordings, i.e. ones that use GCed components
	FAnimationRecorderManager::Get().StopRecordingDeadAnimations();

	const USequenceRecorderSettings* Settings = GetDefault<USequenceRecorderSettings>();
	if(Settings->bCreateLevelSequence)
	{
		if(CurrentSequence.IsValid())
		{
			for(auto& PropertyRecorder : PropertyRecorders)
			{
				PropertyRecorder->FinalizeSection();
			}

			ULevelSequence* LevelSequence = CurrentSequence.Get();

			// Stop referencing the sequence so we are listed as 'not recording'
			CurrentSequence = nullptr;

			UMovieScene* MovieScene = LevelSequence->GetMovieScene();
			if(MovieScene)
			{
				float MinRange = 0.0f;
				float MaxRange = 0.0f;

				for(UActorRecording* Recording : QueuedRecordings)
				{
					if(Recording->LastRecordedAnimation.IsValid())
					{
						UAnimSequence* AnimSequence = Recording->LastRecordedAnimation.Get();

						// resize anim sequence section to fit now it is recorded
						UMovieSceneSkeletalAnimationTrack* AnimTrack = MovieScene->FindTrack<UMovieSceneSkeletalAnimationTrack>(Recording->Guid);
						check(AnimTrack);
					
						UMovieSceneSkeletalAnimationSection* AnimSection = Cast<UMovieSceneSkeletalAnimationSection>(AnimTrack->GetAllSections()[0]);
						AnimSection->SetEndTime(AnimSequence->GetPlayLength());

						MaxRange = FMath::Max(MaxRange, AnimSequence->GetPlayLength());

						// add a spawn track so we actually get spawned
						UMovieSceneSpawnTrack* SpawnTrack = MovieScene->AddTrack<UMovieSceneSpawnTrack>(Recording->Guid);
						if(SpawnTrack)
						{
							UMovieSceneBoolSection* SpawnSection = Cast<UMovieSceneBoolSection>(SpawnTrack->CreateNewSection());

							SpawnTrack->AddSection(*SpawnSection);
							SpawnTrack->SetObjectId(Recording->Guid);

							SpawnSection->SetStartTime(0.0f);
							SpawnSection->SetEndTime(AnimSequence->GetPlayLength());

							SpawnSection->AddKey(0.0f, true, EMovieSceneKeyInterpolation::Break);
							SpawnSection->AddKey(AnimSequence->GetPlayLength(), false, EMovieSceneKeyInterpolation::Break);
						}
					}
				}

				MovieScene->SetPlaybackRange(MinRange, MaxRange);

				FAssetRegistryModule::AssetCreated(MovieScene);
			}
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