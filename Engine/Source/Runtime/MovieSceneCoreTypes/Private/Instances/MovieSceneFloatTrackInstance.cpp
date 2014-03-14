// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCoreTypesPCH.h"
#include "MovieSceneFloatTrackInstance.h"
#include "MatineeUtils.h"


FMovieSceneFloatTrackInstance::FMovieSceneFloatTrackInstance( UMovieSceneFloatTrack& InFloatTrack )
{
	FloatTrack = &InFloatTrack;
}

void FMovieSceneFloatTrackInstance::Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) 
{
	float FloatValue = 0.0f;
	if( FloatTrack->Eval( Position, LastPosition, FloatValue ) )
	{
		for( int32 ObjIndex = 0; ObjIndex < RuntimeObjects.Num(); ++ObjIndex )
		{
			UObject* Object = RuntimeObjects[ObjIndex];

			UObject* PropertyOwner = NULL;
			UProperty* Property = NULL;
			//@todo Sequencer - Major performance problems here.  This needs to be initialized and stored (not serialized) somewhere
			float* Address = FMatineeUtils::GetPropertyAddress<float>( Object, FloatTrack->GetPropertyName(), Property, PropertyOwner );
			if( Address )
			{
				*Address = FloatValue;
				// Let the property owner know that we changed one of its properties
				PropertyOwner->PostInterpChange( Property );
			}
		}
	}
}

