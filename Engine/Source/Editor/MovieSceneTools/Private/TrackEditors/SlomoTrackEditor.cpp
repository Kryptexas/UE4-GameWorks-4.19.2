// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "SlomoTrackEditor.h"


/* FSlomoTrackEditor static functions
 *****************************************************************************/

TSharedRef<FMovieSceneTrackEditor> FSlomoTrackEditor::CreateTrackEditor(TSharedRef<ISequencer> InSequencer)
{
	return MakeShareable(new FSlomoTrackEditor(InSequencer));
}


/* FSlomoTrackEditor structors
 *****************************************************************************/

FSlomoTrackEditor::FSlomoTrackEditor(TSharedRef<ISequencer> InSequencer)
	: FFloatPropertyTrackEditor(InSequencer)
{ }


/* FMovieSceneTrackEditor interface
 *****************************************************************************/

bool FSlomoTrackEditor::SupportsType(TSubclassOf<UMovieSceneTrack> Type) const
{
	return (Type == UMovieSceneSlomoTrack::StaticClass());
}
