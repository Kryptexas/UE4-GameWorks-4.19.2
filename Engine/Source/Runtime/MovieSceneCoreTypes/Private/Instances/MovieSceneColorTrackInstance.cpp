// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCoreTypesPCH.h"
#include "MovieSceneColorTrackInstance.h"
#include "MovieSceneCommonHelpers.h"


FMovieSceneColorTrackInstance::FMovieSceneColorTrackInstance( UMovieSceneColorTrack& InColorTrack )
{
	ColorTrack = &InColorTrack;

	PropertyBindings = MakeShareable( new FTrackInstancePropertyBindings( ColorTrack->GetPropertyName() ) );
}

void FMovieSceneColorTrackInstance::Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) 
{
	FLinearColor ColorValue;
	if( ColorTrack->Eval( Position, LastPosition, ColorValue ) )
	{
		PropertyBindings->CallFunction( RuntimeObjects, &ColorValue );
	}
}

void FMovieSceneColorTrackInstance::RefreshInstance( const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player )
{
	PropertyBindings->UpdateBindings( RuntimeObjects );
}
