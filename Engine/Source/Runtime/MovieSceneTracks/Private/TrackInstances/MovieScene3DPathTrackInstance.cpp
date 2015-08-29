// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneCommonHelpers.h"
#include "MovieScene3DPathTrackInstance.h"
#include "MovieScene3DPathTrack.h"
#include "MovieScene3DPathSection.h"
#include "IMovieScenePlayer.h"
#include "Components/SplineComponent.h"

FMovieScene3DPathTrackInstance::FMovieScene3DPathTrackInstance( UMovieScene3DPathTrack& InPathTrack )
{
	PathTrack = &InPathTrack;
}

void FMovieScene3DPathTrackInstance::SaveState(const TArray<UObject*>& RuntimeObjects)
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

void FMovieScene3DPathTrackInstance::RestoreState(const TArray<UObject*>& RuntimeObjects)
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

void FMovieScene3DPathTrackInstance::Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) 
{
	FVector Translation;
	FRotator Rotation;

	UMovieScene3DPathSection* FirstPathSection = NULL;

	const TArray<UMovieSceneSection*>& PathSections = PathTrack->GetAllSections();

	for (int32 PathIndex = 0; PathIndex < PathSections.Num(); ++PathIndex)
	{
		UMovieScene3DPathSection* PathSection = CastChecked<UMovieScene3DPathSection>(PathSections[PathIndex]);

		if (PathSection->IsTimeWithinSection(Position) &&
			(FirstPathSection == NULL || FirstPathSection->GetRowIndex() > PathSection->GetRowIndex()))
		{
			TArray<UObject*> PathObjects;
			FGuid PathId = PathSection->GetPathId();

			if (PathId.IsValid())
			{
				Player.GetRuntimeObjects( Player.GetRootMovieSceneInstance(), PathId, PathObjects);

				for (int32 PathObjectIndex = 0; PathObjectIndex < PathObjects.Num(); ++PathObjectIndex)
				{
					AActor* Actor = Cast<AActor>(PathObjects[PathObjectIndex]);
					if (Actor)
					{
						TArray<USplineComponent*> SplineComponents;
						Actor->GetComponents(SplineComponents);

						if (SplineComponents.Num())
						{
							PathSection->Eval(Position, SplineComponents[0], Translation, Rotation);

							for( int32 ObjIndex = 0; ObjIndex < RuntimeObjects.Num(); ++ObjIndex )
							{
								USceneComponent* SceneComponent = MovieSceneHelpers::SceneComponentFromRuntimeObject(RuntimeObjects[ObjIndex]);

								if (SceneComponent != NULL)
								{
									// Set the relative translation and rotation.  Note they are set once instead of individually to avoid a redundant component transform update.
									SceneComponent->SetRelativeLocationAndRotation(Translation, Rotation);
								}
							}
						}
					}	
				}
			}
		}
	}
}

