// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneVisibilitySection.h"


UMovieSceneVisibilitySection::UMovieSceneVisibilitySection( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{ }


void UMovieSceneVisibilitySection::AddKey( float Time, bool bHiddenInGame, FKeyParams KeyParams )
{
	// The property that's being changed is bHiddenInGame. But we want to store the inverse of that as visibility, so invert the incoming value.
	Super::AddKey(Time, !bHiddenInGame, KeyParams);
}


bool UMovieSceneVisibilitySection::NewKeyIsNewData( float Time, bool bHiddenInGame, FKeyParams KeyParams ) const
{
	// The property that's being changed is bHiddenInGame. But we want to store the inverse of that as visibility, so invert the incoming value.
	return Super::NewKeyIsNewData(Time, !bHiddenInGame, KeyParams);
}
