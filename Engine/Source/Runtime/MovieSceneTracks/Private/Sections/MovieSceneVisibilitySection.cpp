// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieSceneVisibilitySection.h"


UMovieSceneVisibilitySection::UMovieSceneVisibilitySection( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{
}

void UMovieSceneVisibilitySection::AddKey( float Time, bool bHiddenInGame )
{
	// The property that's being changed is bHiddenInGame. But we want to store the inverse of that as visibility, so invert the incoming value.
	Super::AddKey(Time, !bHiddenInGame);
}

bool UMovieSceneVisibilitySection::NewKeyIsNewData( float Time, bool bHiddenInGame ) const
{
	// The property that's being changed is bHiddenInGame. But we want to store the inverse of that as visibility, so invert the incoming value.
	return Super::NewKeyIsNewData(Time, !bHiddenInGame);
}
