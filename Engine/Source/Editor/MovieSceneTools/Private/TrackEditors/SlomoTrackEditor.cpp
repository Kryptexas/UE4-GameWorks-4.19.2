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
	UMovieSceneSequence* RootMovieSceneSequence = GetSequencer()->GetRootMovieSceneSequence();

	if ((RootMovieSceneSequence == nullptr) || (RootMovieSceneSequence->GetClass()->GetName() != TEXT("LevelSequenceInstance")))
	{
		return;
	}

	MenuBuilder.AddMenuEntry(
		LOCTEXT("AddPlayRateTrack", "Play Rate Track"),
		LOCTEXT("AddPlayRateTrackTooltip", "Adds a new track that controls the playback rate of the sequence."),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "Sequencer.Tracks.Slomo"),
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
		SlomoTrack->AddSection(SlomoTrack->CreateNewSection());

		GetSequencer()->NotifyMovieSceneDataChanged();
	}
}


#undef LOCTEXT_NAMESPACE
