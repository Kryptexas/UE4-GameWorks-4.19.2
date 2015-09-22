// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "MovieSceneSlomoTrack.h"
#include "MovieSceneTrack.h"
#include "SlomoTrackEditor.h"


#define LOCTEXT_NAMESPACE "FSlomoTrackEditor"


/* FSlomoTrackEditor static functions
 *****************************************************************************/

TSharedRef<ISequencerTrackEditor> FSlomoTrackEditor::CreateTrackEditor(TSharedRef<ISequencer> InSequencer)
{
	return MakeShareable(new FSlomoTrackEditor(InSequencer));
}


/* FSlomoTrackEditor structors
 *****************************************************************************/

FSlomoTrackEditor::FSlomoTrackEditor(TSharedRef<ISequencer> InSequencer)
	: FFloatPropertyTrackEditor(InSequencer)
{ }


/* ISequencerTrackEditor interface
 *****************************************************************************/

void FSlomoTrackEditor::BuildAddTrackMenu(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddSlomoTrack", "Add Slomo Track"),
		LOCTEXT("AddSlomoTooltip", "Adds a new slomo track that controls the playback speed of the sequence."),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "ActorAnimationEditor.Tracks.Slomo"),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FSlomoTrackEditor::HandleAddSlomoTrackMenuEntryExecute)
		)
	);
}


bool FSlomoTrackEditor::SupportsType(TSubclassOf<UMovieSceneTrack> Type) const
{
	return (Type == UMovieSceneSlomoTrack::StaticClass());
}


/* FSlomoTrackEditor callbacks
 *****************************************************************************/

void FSlomoTrackEditor::HandleAddSlomoTrackMenuEntryExecute()
{
	if (SlomoTrack.IsValid())
	{
		return;
	}

	UMovieSceneSequence* FocusedSequence = GetSequencer()->GetFocusedMovieSceneSequence();
	UMovieScene* MovieScene = FocusedSequence->GetMovieScene();

	if (MovieScene != nullptr)
	{
		SlomoTrack = MovieScene->AddMasterTrack(UMovieSceneSlomoTrack::StaticClass());
	}
}


#undef LOCTEXT_NAMESPACE
