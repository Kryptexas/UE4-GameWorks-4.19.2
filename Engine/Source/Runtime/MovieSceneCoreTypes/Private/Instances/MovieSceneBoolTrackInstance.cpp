// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCoreTypesPCH.h"
#include "MovieSceneBoolTrackInstance.h"


FMovieSceneBoolTrackInstance::FMovieSceneBoolTrackInstance( UMovieSceneBoolTrack& InBoolTrack )
{
	BoolTrack = &InBoolTrack;
	
	PropertyBindings = MakeShareable( new FTrackInstancePropertyBindings( BoolTrack->GetPropertyName() ) );
}

void FMovieSceneBoolTrackInstance::Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) 
{
	bool BoolValue = false;
	if( BoolTrack->Eval( Position, LastPosition, BoolValue ) )
	{
		PropertyBindings->CallFunction( RuntimeObjects, &BoolValue );
	}
}

void FMovieSceneBoolTrackInstance::RefreshInstance( const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player )
{
	PropertyBindings->UpdateBindings( RuntimeObjects );
}

