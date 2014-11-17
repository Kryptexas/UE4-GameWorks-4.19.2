// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCoreTypesPCH.h"
#include "MovieSceneFloatTrackInstance.h"


FMovieSceneFloatTrackInstance::FMovieSceneFloatTrackInstance( UMovieSceneFloatTrack& InFloatTrack )
{
	FloatTrack = &InFloatTrack;

	PropertyBindings = MakeShareable( new FTrackInstancePropertyBindings( FloatTrack->GetPropertyName() ) );
}

void FMovieSceneFloatTrackInstance::Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) 
{
	float FloatValue = 0.0f;
	if( FloatTrack->Eval( Position, LastPosition, FloatValue ) )
	{
		PropertyBindings->CallFunction( RuntimeObjects, &FloatValue );
	}
}

void FMovieSceneFloatTrackInstance::RefreshInstance( const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player )
{
	PropertyBindings->UpdateBindings( RuntimeObjects );
}
