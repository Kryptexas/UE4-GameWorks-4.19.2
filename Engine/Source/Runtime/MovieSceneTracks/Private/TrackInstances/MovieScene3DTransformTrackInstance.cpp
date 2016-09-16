// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneCommonHelpers.h"
#include "MovieScene3DTransformTrackInstance.h"
#include "MovieScene3DTransformTrack.h"

FMovieScene3DTransformTrackInstance::FMovieScene3DTransformTrackInstance( UMovieScene3DTransformTrack& InTransformTrack )
{
	TransformTrack = &InTransformTrack;
}


void FMovieScene3DTransformTrackInstance::SaveState(const TArray<TWeakObjectPtr<UObject>>& RuntimeObjects, IMovieScenePlayer& Player, FMovieSceneSequenceInstance& SequenceInstance)
{
	for (int32 ObjIndex = 0; ObjIndex < RuntimeObjects.Num(); ++ObjIndex)
	{
		UObject* Object = RuntimeObjects[ObjIndex].Get();
		USceneComponent* SceneComponent = MovieSceneHelpers::SceneComponentFromRuntimeObject(Object);

		if (SceneComponent != nullptr)
		{
			if (InitTransformMap.Find(Object) == nullptr)
			{
				InitTransformMap.Add(Object, SceneComponent->GetRelativeTransform());
			}
			if (InitMobilityMap.Find(Object) == nullptr)
			{
				InitMobilityMap.Add(Object, SceneComponent->Mobility);
			}
		}
	}
}


void FMovieScene3DTransformTrackInstance::RestoreState(const TArray<TWeakObjectPtr<UObject>>& RuntimeObjects, IMovieScenePlayer& Player, FMovieSceneSequenceInstance& SequenceInstance)
{
	for (int32 ObjIndex = 0; ObjIndex < RuntimeObjects.Num(); ++ObjIndex)
	{
		UObject* Object = RuntimeObjects[ObjIndex].Get();

		if (!IsValid(Object))
		{
			continue;
		}

		USceneComponent* SceneComponent = MovieSceneHelpers::SceneComponentFromRuntimeObject(Object);
		if (SceneComponent != nullptr)
		{
			FTransform *Transform = InitTransformMap.Find(Object);
			if (Transform != nullptr)
			{
				SceneComponent->SetRelativeTransform(*Transform);
			}

			EComponentMobility::Type* ComponentMobility = InitMobilityMap.Find(Object);
			if (ComponentMobility != nullptr)
			{
				SceneComponent->SetMobility(*ComponentMobility);
			}
		}
	}
}

void FMovieScene3DTransformTrackInstance::UpdateRuntimeMobility(const TArray<TWeakObjectPtr<UObject>>& RuntimeObjects)
{
	for( int32 ObjIndex = 0; ObjIndex < RuntimeObjects.Num(); ++ObjIndex )
	{
		UObject* Object = RuntimeObjects[ObjIndex].Get();
		USceneComponent* SceneComponent = MovieSceneHelpers::SceneComponentFromRuntimeObject(Object);

		if (SceneComponent != nullptr)
		{
			if (SceneComponent->Mobility != EComponentMobility::Movable)
			{
				if (InitMobilityMap.Find(Object) == nullptr)
				{
					InitMobilityMap.Add(Object, SceneComponent->Mobility);
				}

				SceneComponent->SetMobility(EComponentMobility::Movable);
			}
		}
	}
}

void FMovieScene3DTransformTrackInstance::Update(EMovieSceneUpdateData& UpdateData, const TArray<TWeakObjectPtr<UObject>>& RuntimeObjects, class IMovieScenePlayer& Player, FMovieSceneSequenceInstance& SequenceInstance) 
{
	if (UpdateData.UpdatePass == MSUP_PreUpdate)
	{
		UpdateRuntimeMobility(RuntimeObjects);
	}
	else // UpdateData.UpdatePass == MSUP_Update
	{
		for (TWeakObjectPtr<UObject> ObjectPtr : RuntimeObjects)
		{
			if (ObjectPtr.IsValid())
			{
				USceneComponent* SceneComponent = MovieSceneHelpers::SceneComponentFromRuntimeObject(ObjectPtr.Get());
				if (SceneComponent != nullptr)
				{
					FVector Translation = SceneComponent->GetRelativeTransform().GetTranslation();
					FRotator Rotation = FRotator(SceneComponent->GetRelativeTransform().GetRotation());
					FVector Scale = SceneComponent->GetRelativeTransform().GetScale3D();

					if (TransformTrack->Eval(UpdateData.Position, UpdateData.LastPosition, Translation, Rotation, Scale))
					{
						SceneComponent->SetRelativeLocationAndRotation(Translation, Rotation);
						SceneComponent->SetRelativeScale3D(Scale);

						// Force the location and rotation values to avoid Rot->Quat->Rot conversions
						SceneComponent->RelativeLocation = Translation;
						SceneComponent->RelativeRotation = Rotation;
					}
				}
			}
		}
	}
}
 
void FMovieScene3DTransformTrackInstance::RefreshInstance( const TArray<TWeakObjectPtr<UObject>>& RuntimeObjects, IMovieScenePlayer& Player, FMovieSceneSequenceInstance& SequenceInstance )
{
	UpdateRuntimeMobility(RuntimeObjects);
}
