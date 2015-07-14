// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneCommonHelpers.h"
#include "MovieScene3DTransformTrackInstance.h"
#include "MovieScene3DTransformTrack.h"


FMovieScene3DTransformTrackInstance::FMovieScene3DTransformTrackInstance( UMovieScene3DTransformTrack& InTransformTrack )
{
	TransformTrack = &InTransformTrack;
}

void FMovieScene3DTransformTrackInstance::SaveState(const TArray<UObject*>& RuntimeObjects)
{
	for (int32 ObjIndex = 0; ObjIndex < RuntimeObjects.Num(); ++ObjIndex)
	{
		USceneComponent* SceneComponent = MovieSceneHelpers::SceneComponentFromRuntimeObject(RuntimeObjects[ObjIndex]);
		if (SceneComponent != NULL)
		{
			InitTransformMap.Add(RuntimeObjects[ObjIndex], SceneComponent->GetRelativeTransform());
		}
	}
}

void FMovieScene3DTransformTrackInstance::RestoreState(const TArray<UObject*>& RuntimeObjects)
{
	for (int32 ObjIndex = 0; ObjIndex < RuntimeObjects.Num(); ++ObjIndex)
	{
		if (!IsValid(RuntimeObjects[ObjIndex]))
		{
			continue;
		}

		USceneComponent* SceneComponent = MovieSceneHelpers::SceneComponentFromRuntimeObject(RuntimeObjects[ObjIndex]);
		if (SceneComponent != NULL)
		{
			FTransform *Transform = InitTransformMap.Find(RuntimeObjects[ObjIndex]);
			if (Transform != NULL)
			{
				SceneComponent->SetRelativeTransform(*Transform);
			}
		}
	}
}

void FMovieScene3DTransformTrackInstance::Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) 
{
	FVector Translation;
	FRotator Rotation;
	FVector Scale;
	TArray<bool> bHasTranslationKeys;
	TArray<bool> bHasRotationKeys;
	TArray<bool> bHasScaleKeys;

	if( TransformTrack->Eval( Position, LastPosition, Translation, Rotation, Scale, bHasTranslationKeys, bHasRotationKeys, bHasScaleKeys ) )
	{
		for( int32 ObjIndex = 0; ObjIndex < RuntimeObjects.Num(); ++ObjIndex )
		{
			USceneComponent* SceneComponent = MovieSceneHelpers::SceneComponentFromRuntimeObject(RuntimeObjects[ObjIndex]);

			if (SceneComponent != NULL)
			{
				// Set the relative translation and rotation.  Note they are set once instead of individually to avoid a redundant component transform update.
				FVector NewTranslation;
				NewTranslation[0] = bHasTranslationKeys[0] ? Translation[0] : SceneComponent->RelativeLocation[0];
				NewTranslation[1] = bHasTranslationKeys[1] ? Translation[1] : SceneComponent->RelativeLocation[1];
				NewTranslation[2] = bHasTranslationKeys[2] ? Translation[2] : SceneComponent->RelativeLocation[2];

				FRotator NewRotation;
				NewRotation.Roll = bHasRotationKeys[0] ? Rotation.Roll : SceneComponent->RelativeRotation.Roll;
				NewRotation.Pitch = bHasRotationKeys[1] ? Rotation.Pitch : SceneComponent->RelativeRotation.Pitch;
				NewRotation.Yaw = bHasRotationKeys[2] ? Rotation.Yaw : SceneComponent->RelativeRotation.Yaw;

				SceneComponent->SetRelativeLocationAndRotation(NewTranslation, NewRotation);

				FVector NewScale;
				NewScale[0] = bHasScaleKeys[0] ? Scale[0] : SceneComponent->RelativeScale3D[0];
				NewScale[1] = bHasScaleKeys[1] ? Scale[1] : SceneComponent->RelativeScale3D[1];
				NewScale[2] = bHasScaleKeys[2] ? Scale[2] : SceneComponent->RelativeScale3D[2];

				SceneComponent->SetRelativeScale3D(NewScale);
			}
		}
	}
}

