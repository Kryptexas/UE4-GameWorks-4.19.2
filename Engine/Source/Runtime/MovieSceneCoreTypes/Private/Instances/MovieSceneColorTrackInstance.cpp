// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCoreTypesPCH.h"
#include "MovieSceneColorTrackInstance.h"
#include "MovieSceneCommonHelpers.h"


FMovieSceneColorTrackInstance::FMovieSceneColorTrackInstance( UMovieSceneColorTrack& InColorTrack )
{
	ColorTrack = &InColorTrack;

	PropertyBindings = MakeShareable( new FTrackInstancePropertyBindings( ColorTrack->GetPropertyName(), ColorTrack->GetPropertyPath() ) );
}

void FMovieSceneColorTrackInstance::Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) 
{
	for(UObject* Object : RuntimeObjects)
	{
		FLinearColor ColorValue = PropertyBindings->GetCurrentValue<FLinearColor>(Object);
		if(ColorTrack->Eval(Position, LastPosition, ColorValue))
		{
			PropertyBindings->CallFunction(Object, &ColorValue);
		}
	}
}

void FMovieSceneColorTrackInstance::RefreshInstance( const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player )
{
	PropertyBindings->UpdateBindings( RuntimeObjects );
}
