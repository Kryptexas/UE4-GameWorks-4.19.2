// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "MovieSceneMarginTrackInstance.h"

FMovieSceneMarginTrackInstance::FMovieSceneMarginTrackInstance( UMovieSceneMarginTrack& InMarginTrack )
{
	MarginTrack = &InMarginTrack;
}

void FMovieSceneMarginTrackInstance::Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) 
{
	FMargin MarginValue;
	if( MarginTrack->Eval( Position, LastPosition, MarginValue ) )
	{
		for( int32 ObjIndex = 0; ObjIndex < RuntimeObjects.Num(); ++ObjIndex )
		{
			UObject* Object = RuntimeObjects[ObjIndex];

			UFunction* Function = RuntimeObjectToFunctionMap.FindRef( Object );
			if( Function )
			{
				Object->ProcessEvent( Function, &MarginValue );
			}
		}
	}
}

void FMovieSceneMarginTrackInstance::RefreshInstance( const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player )
{
	static const FString Set(TEXT("Set") );
	
	FString FunctionString = Set + MarginTrack->GetPropertyName().ToString();
	
	FName FunctionName = FName( *FunctionString );
	
	//Player.GetRuntimeObjects( )
	for( UObject* Object : RuntimeObjects )
	{
		// If the mapping for the object already exists we dont need to search for it again
		UFunction* Function = RuntimeObjectToFunctionMap.FindRef( Object );
		
		if( !Function )
		{
			Function = Object->FindFunction( FunctionName );
			if( Function )
			{
				RuntimeObjectToFunctionMap.Add( Object, Function );
			}
		}
	}
}
