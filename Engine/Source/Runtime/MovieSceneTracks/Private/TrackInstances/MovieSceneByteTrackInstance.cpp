// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneByteTrack.h"
#include "MovieSceneByteTrackInstance.h"


FMovieSceneByteTrackInstance::FMovieSceneByteTrackInstance( UMovieSceneByteTrack& InByteTrack )
{
	ByteTrack = &InByteTrack;
	
	PropertyBindings = MakeShareable( new FTrackInstancePropertyBindings( ByteTrack->GetPropertyName(), ByteTrack->GetPropertyPath() ) );
}


void FMovieSceneByteTrackInstance::SaveState(const TArray<UObject*>& RuntimeObjects)
{
	for( UObject* Object : RuntimeObjects )
	{
		uint8 ByteValue = PropertyBindings->GetCurrentValue<uint8>(Object);
		InitByteMap.Add(Object, ByteValue);
	}
}


void FMovieSceneByteTrackInstance::RestoreState(const TArray<UObject*>& RuntimeObjects)
{
	for( UObject* Object : RuntimeObjects )
	{
		uint8 *ByteValue = InitByteMap.Find(Object);
		if (ByteValue != NULL)
		{
			PropertyBindings->CallFunction(Object, ByteValue);
		}
	}

	PropertyBindings->UpdateBindings( RuntimeObjects );
}


void FMovieSceneByteTrackInstance::Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) 
{
	uint8 ByteValue = 0;
	if( ByteTrack->Eval( Position, LastPosition, ByteValue ) )
	{
		for( UObject* Object : RuntimeObjects )
		{
			PropertyBindings->CallFunction( Object, &ByteValue );
		}
	}
}


void FMovieSceneByteTrackInstance::RefreshInstance( const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player )
{
	PropertyBindings->UpdateBindings( RuntimeObjects );
}
