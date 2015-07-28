// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "FloatPropertyTrackEditor.h"
#include "FloatPropertySection.h"

TSharedRef<FMovieSceneTrackEditor> FFloatPropertyTrackEditor::CreateTrackEditor( TSharedRef<ISequencer> OwningSequencer )
{
	return MakeShareable(new FFloatPropertyTrackEditor(OwningSequencer));
}

TSharedRef<ISequencerSection> FFloatPropertyTrackEditor::MakeSectionInterface( UMovieSceneSection& SectionObject, UMovieSceneTrack* Track )
{
	return MakeShareable(new FFloatPropertySection( SectionObject, Track->GetTrackName() ));
}

bool FFloatPropertyTrackEditor::TryGenerateKeyFromPropertyChanged( const FPropertyChangedParams& PropertyChangedParams, float& OutKey )
{
	OutKey = *PropertyChangedParams.GetPropertyValue<float>();
	return true;
}