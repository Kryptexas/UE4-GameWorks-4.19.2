// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCoreTypesPCH.h"
#include "MovieSceneFloatTrackInstance.h"


FMovieSceneFloatTrackInstance::FMovieSceneFloatTrackInstance( UMovieSceneFloatTrack& InFloatTrack )
{
	FloatTrack = &InFloatTrack;

	PropertyBindings = MakeShareable( new FTrackInstancePropertyBindings( FloatTrack->GetPropertyName(), FloatTrack->GetPropertyPath() ) );
}

void FMovieSceneFloatTrackInstance::SaveState(const TArray<UObject*>& RuntimeObjects)
{
	for( UObject* Object : RuntimeObjects )
	{
		float FloatValue = PropertyBindings->GetCurrentValue<float>(Object);
		InitFloatMap.Add(Object, FloatValue);
	}
}

void FMovieSceneFloatTrackInstance::RestoreState(const TArray<UObject*>& RuntimeObjects)
{
	for( UObject* Object : RuntimeObjects )
	{
		float *FloatValue = InitFloatMap.Find(Object);
		if (FloatValue != NULL)
		{
			PropertyBindings->CallFunction(Object, FloatValue);
		}
	}

	PropertyBindings->UpdateBindings( RuntimeObjects );
}

void FMovieSceneFloatTrackInstance::Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) 
{
	float FloatValue = 0.0f;
	if( FloatTrack->Eval( Position, LastPosition, FloatValue ) )
	{
		for(UObject* Object : RuntimeObjects)
		{
			PropertyBindings->CallFunction( Object, &FloatValue );
		}
	}
}

void FMovieSceneFloatTrackInstance::RefreshInstance( const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player )
{
	PropertyBindings->UpdateBindings( RuntimeObjects );
}
