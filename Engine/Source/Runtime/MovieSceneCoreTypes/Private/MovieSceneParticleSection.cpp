// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCoreTypesPCH.h"
#include "MovieSceneParticleSection.h"

UMovieSceneParticleSection::UMovieSceneParticleSection( const FPostConstructInitializeProperties& PCIP )
	: Super( PCIP )
{
	KeyType = EParticleKey::Toggle;
}

