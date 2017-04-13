// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SlateIcon.h"

class SWidget;
class UMovieSceneSequence;
class FMenuBuilder;
class STextBlock;
struct EVisibility;
struct FSlateBrush;
struct FSequenceBindingTree;
struct FSequenceBindingNode;
struct FMovieSceneObjectBindingID;

/**
 * Helper class that is used to pick object bindings for movie scene data
 */
class MOVIESCENETOOLS_API FMovieSceneObjectBindingIDPicker
{
public:
	FMovieSceneObjectBindingIDPicker()
		: bIsCurrentItemSpawnable(false)
	{}

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

	/** Get the icon that represents the currently assigned binding */
	FSlateIcon GetCurrentIcon() const;
	const FSlateBrush* GetCurrentIconBrush() const;

	/** Get the visibility for the spawnable icon overlap */ 
	EVisibility GetSpawnableIconOverlayVisibility() const;

	/** Assign a new binding ID in response to user-input */
	void SetBindingId(FMovieSceneObjectBindingID InBindingId);

	/** Build menu content that allows the user to choose a binding from inside the source sequence */
	TSharedRef<SWidget> GetPickerMenu();

	/** Get a widget that represents the currently chosen item */
	TSharedRef<SWidget> GetCurrentItemWidget(TSharedRef<STextBlock> TextContent);

private:

	/** UPdate the cached text, tooltip and icon */
	void UpdateCachedData();

	/** Called when the combo box has been clicked to populate its menu content */
	void OnGetMenuContent(FMenuBuilder& MenuBuilder, TSharedPtr<FSequenceBindingNode> Node);

	/** Cached current text and tooltips */
	FText CurrentText, ToolTipText;

	/** Cached current icon */
	FSlateIcon CurrentIcon;

	/** Cached value indicating whether the current item is a spawnable */
	bool bIsCurrentItemSpawnable;

	/** Data tree that stores all the available bindings for the current sequence, and their identifiers */
	TSharedPtr<FSequenceBindingTree> DataTree;
};