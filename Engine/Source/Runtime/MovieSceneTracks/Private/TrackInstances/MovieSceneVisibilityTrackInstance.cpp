// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneVisibilityTrackInstance.h"
#include "MovieSceneVisibilityTrack.h"

FMovieSceneVisibilityTrackInstance::FMovieSceneVisibilityTrackInstance(UMovieSceneVisibilityTrack& InVisibilityTrack)
{
	VisibilityTrack = &InVisibilityTrack;

	PropertyBindings = MakeShareable( new FTrackInstancePropertyBindings( VisibilityTrack->GetPropertyName(), VisibilityTrack->GetPropertyPath() ) );
}

void FMovieSceneVisibilityTrackInstance::SaveState(const TArray<UObject*>& RuntimeObjects)
{
 	for( UObject* Object : RuntimeObjects )
 	{
		AActor* Actor = Cast<AActor>(Object);

		if (Actor != NULL)
		{
#if WITH_EDITOR
			if (GIsEditor && !Actor->GetWorld()->IsPlayInEditor())
			{
				InitHiddenInEditorMap.Add(Actor, Actor->IsTemporarilyHiddenInEditor());
			}
#endif // WITH_EDITOR

			InitHiddenInGameMap.Add(Object, Actor->bHidden);
		}
 	}
}

void FMovieSceneVisibilityTrackInstance::RestoreState(const TArray<UObject*>& RuntimeObjects)
{
 	for( UObject* Object : RuntimeObjects )
 	{
		AActor* Actor = Cast<AActor>(Object);

		if (Actor != NULL)
		{
			bool *HiddenInGameValue = InitHiddenInGameMap.Find(Object);
			if (HiddenInGameValue != NULL)
			{
				Actor->SetActorHiddenInGame(*HiddenInGameValue);
			}

#if WITH_EDITOR
			if (GIsEditor && !Actor->GetWorld()->IsPlayInEditor())
			{
				bool *HiddenInEditorValue = InitHiddenInEditorMap.Find(Object);
				if (HiddenInEditorValue != NULL)
				{
					if (Actor->IsTemporarilyHiddenInEditor() != *HiddenInEditorValue)
					{
						Actor->SetIsTemporarilyHiddenInEditor(*HiddenInEditorValue);
					}
				}
			}
		}
#endif // WITH_EDITOR
 	}
}

void FMovieSceneVisibilityTrackInstance::Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) 
{
 	bool Visible = false;
 	if( VisibilityTrack->Eval( Position, LastPosition, Visible ) )
 	{
		// Invert this evaluation since the property is "bHiddenInGame" and we want the visualization to be the inverse of that. Green means visible.
		Visible = !Visible;

 		for( UObject* Object : RuntimeObjects )
 		{
			AActor* Actor = Cast<AActor>(Object);

			if (Actor != NULL)
			{
				Actor->SetActorHiddenInGame(Visible);

#if WITH_EDITOR
				if (GIsEditor && !Actor->GetWorld()->IsPlayInEditor())
				{
					// In editor HiddenGame flag is not respected so set bHiddenEdTemporary too.
					// It will be restored just like HiddenGame flag when Matinee is closed.
					if (Actor->IsTemporarilyHiddenInEditor() != Visible)
					{
						Actor->SetIsTemporarilyHiddenInEditor(Visible);
					}
				}
#endif // WITH_EDITOR
			}
 		}
	}
}

void FMovieSceneVisibilityTrackInstance::RefreshInstance( const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player )
{
	PropertyBindings->UpdateBindings( RuntimeObjects );
}

