// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCoreTypesPCH.h"
#include "MovieSceneVectorTrackInstance.h"

FMovieSceneVectorTrackInstance::FMovieSceneVectorTrackInstance( UMovieSceneVectorTrack& InVectorTrack )
{
	VectorTrack = &InVectorTrack;

	PropertyBindings = MakeShareable( new FTrackInstancePropertyBindings( VectorTrack->GetPropertyName() ) );
}

void FMovieSceneVectorTrackInstance::Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) 
{
	FVector4 Vector;
	if( VectorTrack->Eval( Position, LastPosition, Vector ) )
	{
		int32 NumChannelsUsed = VectorTrack->GetNumChannelsUsed();
		switch( NumChannelsUsed )
		{
			case 2:
			{
				FVector2D Value(Vector.X, Vector.Y);
				PropertyBindings->CallFunction(RuntimeObjects, &Value);
				break;
			}
			case 3:
			{
				FVector Value(Vector.X, Vector.Y, Vector.Z);
				PropertyBindings->CallFunction(RuntimeObjects, &Value);
				break;
			}
			case 4:
			{
				PropertyBindings->CallFunction(RuntimeObjects, &Vector);
				break;
			}
			default:
				UE_LOG(LogSequencerRuntime, Warning, TEXT("Invalid number of channels(%d) for vector track"), NumChannelsUsed );
				break;
		}
		
	}
}


void FMovieSceneVectorTrackInstance::RefreshInstance(const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player)
{
	PropertyBindings->UpdateBindings(RuntimeObjects);
}
