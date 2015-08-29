// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneBoolTrack.h"
#include "MovieSceneBoolTrackInstance.h"


FMovieSceneBoolTrackInstance::FMovieSceneBoolTrackInstance( UMovieSceneBoolTrack& InBoolTrack )
{
	BoolTrack = &InBoolTrack;
	
	PropertyBindings = MakeShareable( new FTrackInstancePropertyBindings( BoolTrack->GetPropertyName(), BoolTrack->GetPropertyPath() ) );
}


void FMovieSceneBoolTrackInstance::SaveState(const TArray<UObject*>& RuntimeObjects)
{
	for( UObject* Object : RuntimeObjects )
	{
		bool BoolValue = PropertyBindings->GetCurrentValue<bool>(Object);
		InitBoolMap.Add(Object, BoolValue);
	}
}


void FMovieSceneBoolTrackInstance::RestoreState(const TArray<UObject*>& RuntimeObjects)
{
	for( UObject* Object : RuntimeObjects )
	{
		if (!IsValid(Object))
		{
			continue;
		}

		bool *BoolValue = InitBoolMap.Find(Object);
		if (BoolValue != NULL)
		{
			PropertyBindings->CallFunction(Object, BoolValue);
		}
	}

	PropertyBindings->UpdateBindings( RuntimeObjects );
}


void FMovieSceneBoolTrackInstance::Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) 
{
	bool BoolValue = false;
	if( BoolTrack->Eval( Position, LastPosition, BoolValue ) )
	{
		for( UObject* Object : RuntimeObjects )
		{
			PropertyBindings->CallFunction( Object, &BoolValue );
		}
	}
}


void FMovieSceneBoolTrackInstance::RefreshInstance( const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player )
{
	PropertyBindings->UpdateBindings( RuntimeObjects );
}
