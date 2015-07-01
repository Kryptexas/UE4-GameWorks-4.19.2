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
	bool bHasTranslationKeys = false;
	bool bHasRotationKeys = false;
	bool bHasScaleKeys = false;

	if( TransformTrack->Eval( Position, LastPosition, Translation, Rotation, Scale, bHasTranslationKeys, bHasRotationKeys, bHasScaleKeys ) )
	{
		for( int32 ObjIndex = 0; ObjIndex < RuntimeObjects.Num(); ++ObjIndex )
		{
			USceneComponent* SceneComponent = MovieSceneHelpers::SceneComponentFromRuntimeObject(RuntimeObjects[ObjIndex]);

			if (SceneComponent != NULL)
			{
				// Set the relative translation and rotation.  Note they are set once instead of individually to avoid a redundant component transform update.
				SceneComponent->SetRelativeLocationAndRotation(
					bHasTranslationKeys ? Translation : SceneComponent->RelativeLocation,
					bHasRotationKeys ? Rotation : SceneComponent->RelativeRotation );

				if( bHasScaleKeys )
				{
					SceneComponent->SetRelativeScale3D( Scale );
				}
			}
		}
	}
}

