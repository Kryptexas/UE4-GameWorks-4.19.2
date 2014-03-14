// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCoreTypesPCH.h"
#include "MovieSceneBoolTrackInstance.h"
#include "MatineeUtils.h"


FMovieSceneBoolTrackInstance::FMovieSceneBoolTrackInstance( UMovieSceneBoolTrack& InBoolTrack )
{
	BoolTrack = &InBoolTrack;
}

void FMovieSceneBoolTrackInstance::Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) 
{
	bool BoolValue = false;
	if( BoolTrack->Eval( Position, LastPosition, BoolValue ) )
	{
		for( int32 ObjIndex = 0; ObjIndex < RuntimeObjects.Num(); ++ObjIndex )
		{
			UObject* Object = RuntimeObjects[ObjIndex];

			UObject* PropertyOwner = NULL;
			UProperty* Property = NULL;
			//@todo Sequencer - Major performance problems here.  This needs to be initialized and stored (not serialized) somewhere
			uint8* Address = FMatineeUtils::GetPropertyAddress<uint8>( Object, BoolTrack->GetPropertyName(), Property, PropertyOwner );
			if( Address )
			{
				CastChecked<UBoolProperty>(Property)->SetPropertyValue(Address, BoolValue);
				// Let the property owner know that we changed one of its properties
				PropertyOwner->PostInterpChange( Property );
			}
		}
	}
}

