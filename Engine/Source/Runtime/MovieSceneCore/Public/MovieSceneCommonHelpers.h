// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


class MOVIESCENECORE_API MovieSceneHelpers
{
public:
	/**
	 * Gets the sections that were traversed over between the current time and the previous time
	 */
	static TArray<UMovieSceneSection*> GetTraversedSections( const TArray<UMovieSceneSection*>& Sections, float CurrentTime, float PreviousTime );

	/**
	* Finds a section that exists at a given time
	*
	* @param Time	The time to find a section at
	* @return The found section or null
	*/
	static UMovieSceneSection* FindSectionAtTime( const TArray<UMovieSceneSection*>& Sections, float Time );

};

/**
 * Manages bindings to keyed properties for a track instance. 
 * Calls UFunctions to set the value on runtime objects
 */
class MOVIESCENECORE_API FTrackInstancePropertyBindings
{
public:
	FTrackInstancePropertyBindings( FName PropertyName );

	void CallFunction( const TArray<UObject*>& InRuntimeObjects, void* FunctionParams );
	void UpdateBindings( const TArray<UObject*>& InRuntimeObjects );
private:
	/** Mapping of objects to bound functions that will be called to update data on the track */
	TMap< TWeakObjectPtr<UObject>, UFunction* > RuntimeObjectToFunctionMap;
	FName FunctionName;
};