// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTrackEditor.h"
#include "Sequencer.h"

void FMovieSceneTrackEditor::AnimatablePropertyChanged(FOnKeyProperty OnKeyProperty)
{
	check(OnKeyProperty.IsBound());

	// Get the movie scene we want to autokey
	UMovieSceneSequence* MovieSceneSequence = GetMovieSceneSequence();
	if (MovieSceneSequence)
	{
		float KeyTime = GetTimeForKey();

		if (!Sequencer.Pin()->IsRecordingLive())
		{
			// @todo Sequencer - The sequencer probably should have taken care of this
			MovieSceneSequence->SetFlags(RF_Transactional);

			// Create a transaction record because we are about to add keys
			const bool bShouldActuallyTransact = !Sequencer.Pin()->IsRecordingLive();		// Don't transact if we're recording in a PIE world.  That type of keyframe capture cannot be undone.
			FScopedTransaction AutoKeyTransaction(NSLOCTEXT("AnimatablePropertyTool", "PropertyChanged", "Animatable Property Changed"), bShouldActuallyTransact);

			if (OnKeyProperty.Execute(KeyTime))
			{
				// TODO: This should pass an appropriate change type here instead of always passing structure changed since most
				// changes will be value changes.
				Sequencer.Pin()->NotifyMovieSceneDataChanged(EMovieSceneDataChangeType::MovieSceneStructureItemAdded);
			}

			UpdatePlaybackRange();


			TSharedPtr<FSequencer> SequencerToUpdate = StaticCastSharedPtr<FSequencer>(Sequencer.Pin());
			if (SequencerToUpdate.IsValid())
			{
				SequencerToUpdate->SynchronizeSequencerSelectionWithExternalSelection();
			}
		}
	}
}