// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneVectorTrackInstance.h"
#include "MovieSceneVectorTrack.h"

FMovieSceneVectorTrackInstance::FMovieSceneVectorTrackInstance( UMovieSceneVectorTrack& InVectorTrack )
{
	VectorTrack = &InVectorTrack;

	PropertyBindings = MakeShareable( new FTrackInstancePropertyBindings( VectorTrack->GetPropertyName(), VectorTrack->GetPropertyPath() ) );
}

void FMovieSceneVectorTrackInstance::SaveState(const TArray<UObject*>& RuntimeObjects)
{
	int32 NumChannelsUsed = VectorTrack->GetNumChannelsUsed();
	for( UObject* Object : RuntimeObjects )
	{
		switch( NumChannelsUsed )
		{
			case 2:
			{
				FVector2D VectorValue = PropertyBindings->GetCurrentValue<FVector2D>(Object);
				InitVector2DMap.Add(Object, VectorValue);
				break;
			}
			case 3:
			{
				FVector VectorValue = PropertyBindings->GetCurrentValue<FVector>(Object);
				InitVector3DMap.Add(Object, VectorValue);
				break;
			}
			case 4:
			{
				FVector4 VectorValue = PropertyBindings->GetCurrentValue<FVector4>(Object);
				InitVector4DMap.Add(Object, VectorValue);
				break;
			}
		}
	}
}

void FMovieSceneVectorTrackInstance::RestoreState(const TArray<UObject*>& RuntimeObjects)
{
	int32 NumChannelsUsed = VectorTrack->GetNumChannelsUsed();
	for( UObject* Object : RuntimeObjects )
	{
		switch( NumChannelsUsed )
		{
			case 2:
			{
				FVector2D *VectorValue = InitVector2DMap.Find(Object);
				if (VectorValue != NULL)
				{
					PropertyBindings->CallFunction(Object, VectorValue);
				}
				break;
			}
			case 3:
			{
				FVector *VectorValue = InitVector3DMap.Find(Object);
				if (VectorValue != NULL)
				{
					PropertyBindings->CallFunction(Object, VectorValue);
				}
				break;
			}
			case 4:
			{
				FVector4 *VectorValue = InitVector4DMap.Find(Object);
				if (VectorValue != NULL)
				{
					PropertyBindings->CallFunction(Object, VectorValue);
				}
				break;
			}
		}
	}

	PropertyBindings->UpdateBindings( RuntimeObjects );
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
				for(UObject* Object : RuntimeObjects)
				{
					PropertyBindings->CallFunction(Object, &Value);
				}
				break;
			}
			case 3:
			{
				FVector Value(Vector.X, Vector.Y, Vector.Z);
				for(UObject* Object : RuntimeObjects)
				{
					PropertyBindings->CallFunction(Object, &Value);
				}
				break;
			}
			case 4:
			{
				for(UObject* Object : RuntimeObjects)
				{
					PropertyBindings->CallFunction(Object, &Vector);
				}
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
