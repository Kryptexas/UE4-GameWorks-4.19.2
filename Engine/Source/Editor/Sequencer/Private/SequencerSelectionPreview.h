// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SelectedKey.h"

class UMovieSceneSection;
class FSequencerDisplayNode;

enum class ESelectionPreviewState
{
	Undefined,
	Selected,
	NotSelected
};

/**
 * Manages the selection of keys, sections, and outliner nodes for the sequencer.
 */
class FSequencerSelectionPreview
{
public:

	/** Access the defined key states */
	const TMap<FSelectedKey, ESelectionPreviewState>& GetDefinedKeyStates() const { return DefinedKeyStates; }

	/** Access the defined section states */
	const TMap<TWeakObjectPtr<UMovieSceneSection>, ESelectionPreviewState>& GetDefinedSectionStates() const { return DefinedSectionStates; }

	/** Adds a key to the selection */
	void SetSelectionState(FSelectedKey Key, ESelectionPreviewState InState);

	/** Adds a section to the selection */
	void SetSelectionState(UMovieSceneSection* Section, ESelectionPreviewState InState);

	/** Returns the selection state for the the specified key. */
	ESelectionPreviewState GetSelectionState(FSelectedKey Key) const;

	/** Returns the selection state for the the specified section. */
	ESelectionPreviewState GetSelectionState(UMovieSceneSection* Section) const;

	/** Empties all selections. */
	void Empty();

	/** Empties the key selection. */
	void EmptyDefinedKeyStates();

	/** Empties the section selection. */
	void EmptyDefinedSectionStates();

private:

	TMap<FSelectedKey, ESelectionPreviewState> DefinedKeyStates;
	TMap<TWeakObjectPtr<UMovieSceneSection>, ESelectionPreviewState> DefinedSectionStates;
};