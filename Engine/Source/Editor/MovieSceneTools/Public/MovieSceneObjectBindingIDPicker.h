// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class SWidget;
class UMovieSceneSequence;
class FMenuBuilder;
struct FSequenceBindingTree;
struct FSequenceBindingNode;
struct FMovieSceneObjectBindingID;

/**
 * Helper class that is used to pick object bindings for movie scene data
 */
class MOVIESCENETOOLS_API FMovieSceneObjectBindingIDPicker
{
protected:

	/** Get the sequence to look up object bindings within */
	virtual UMovieSceneSequence* GetSequence() const = 0;

	/** Set the current binding ID */
	virtual void SetCurrentValue(const FMovieSceneObjectBindingID& InBindingId) = 0;

	/** Get the current binding ID */
	virtual FMovieSceneObjectBindingID GetCurrentValue() const = 0;

protected:

	/** Initialize this class - rebuilds sequence hierarchy data and available IDs from the source sequence */
	void Initialize();

	/** Access the text that relates to the currently selected binding ID */
	FText GetCurrentText() const;

	/** Access the tooltip text that relates to the currently selected binding ID */
	FText GetToolTipText() const;

	/** Assign a new binding ID in response to user-input */
	void SetBindingId(FMovieSceneObjectBindingID InBindingId);

	/** Build menu content that allows the user to choose a binding from inside the source sequence */
	TSharedRef<SWidget> GetPickerMenu();

private:

	/** Called when the combo box has been clicked to populate its menu content */
	void OnGetMenuContent(FMenuBuilder& MenuBuilder, TSharedPtr<FSequenceBindingNode> Node);

	/** Cached current text and tooltips */
	FText CurrentText, ToolTipText;

	/** Data tree that stores all the available bindings for the current sequence, and their identifiers */
	TSharedPtr<FSequenceBindingTree> DataTree;
};