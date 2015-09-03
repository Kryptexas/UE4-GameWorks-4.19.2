// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "EventTrackEditor.h"
#include "EventTrackSection.h"


/* FEventTrackEditor static functions
 *****************************************************************************/

TSharedRef<FMovieSceneTrackEditor> FEventTrackEditor::CreateTrackEditor(TSharedRef<ISequencer> InSequencer)
{
	return MakeShareable(new FEventTrackEditor(InSequencer));
}


/* FEventTrackEditor structors
 *****************************************************************************/

FEventTrackEditor::FEventTrackEditor(TSharedRef<ISequencer> InSequencer)
	: FMovieSceneTrackEditor(InSequencer)
{ }


/* FMovieSceneTrackEditor interface
 *****************************************************************************/

void FEventTrackEditor::AddKey(const FGuid& ObjectGuid, UObject* AdditionalAsset)
{
	// todo gmp: Sequencer: implement event track section
}


TSharedRef<ISequencerSection> FEventTrackEditor::MakeSectionInterface(UMovieSceneSection& SectionObject, UMovieSceneTrack* Track)
{
	return MakeShareable(new FEventTrackSection(SectionObject/*, Track->GetTrackName()*/, GetSequencer()));
}


bool FEventTrackEditor::SupportsType(TSubclassOf<UMovieSceneTrack> Type) const
{
	return (Type == UMovieSceneEventTrack::StaticClass());
}
