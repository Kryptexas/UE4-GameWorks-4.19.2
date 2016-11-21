// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieScene3DTransformSection.h"
#include "MovieScene3DTransformTrack.h"
#include "IMovieScenePlayer.h"
#include "MovieSceneSegmentCompiler.h"

UMovieScene3DTransformTrack::UMovieScene3DTransformTrack( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
{
	static FName Transform("Transform");
	SetPropertyNameAndPath(Transform, Transform.ToString());

#if WITH_EDITORONLY_DATA
	TrackTint = FColor(65, 173, 164, 65);
#endif

	EvalOptions.bEvaluateNearestSection = EvalOptions.bCanEvaluateNearestSection = true;
}

UMovieSceneSection* UMovieScene3DTransformTrack::CreateNewSection()
{
	return NewObject<UMovieSceneSection>(this, UMovieScene3DTransformSection::StaticClass(), NAME_None, RF_Transactional);
}