// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "MovieSceneMarginTrackInstance.h"

FMovieSceneMarginTrackInstance::FMovieSceneMarginTrackInstance( UMovieSceneMarginTrack& InMarginTrack )
{
	MarginTrack = &InMarginTrack;

	PropertyBindings = MakeShareable( new FTrackInstancePropertyBindings( MarginTrack->GetPropertyName() ) );
}

void FMovieSceneMarginTrackInstance::Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) 
{
	FMargin MarginValue;
	if( MarginTrack->Eval( Position, LastPosition, MarginValue ) )
	{
		PropertyBindings->CallFunction( RuntimeObjects, &MarginValue );
	}
}

void FMovieSceneMarginTrackInstance::RefreshInstance( const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player )
{
	PropertyBindings->UpdateBindings( RuntimeObjects );
}
