// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


namespace MovieSceneHelpers
{
	/**
	 * Gets the sections that were traversed over between the current time and the previous time
	 */
	TArray<UMovieSceneSection*> GetTraversedSections( const TArray<UMovieSceneSection*>& Sections, float CurrentTime, float PreviousTime );

	/**
	* Finds a section that exists at a given time
	*
	* @param Time	The time to find a section at
	* @return The found section or null
	*/
	UMovieSceneSection* FindSectionAtTime( const TArray<UMovieSceneSection*>& Sections, float Time );
}