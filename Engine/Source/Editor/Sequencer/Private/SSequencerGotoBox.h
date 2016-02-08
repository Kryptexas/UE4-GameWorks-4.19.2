// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NumericTypeInterface.h"
#include "SNumericEntryBox.h"


class FSequencer;
class USequencerSettings;


class SSequencerGotoBox
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSequencerGotoBox) { }
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<FSequencer>& InSequencer, USequencerSettings& InSettings, const TSharedRef<INumericTypeInterface<float>>& InNumericTypeInterface);

public:

	// SWidget interface

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:

	/** Callback for committed values in the entry box. */
	void HandleEntryBoxValueCommitted(float Value, ETextCommit::Type CommitType);

private:

	/** The entry box widget. */
	TSharedPtr<SNumericEntryBox<float>> EntryBox;

	/** Numeric type interface used for converting parsing and generating strings from numbers. */
	TSharedPtr<INumericTypeInterface<float>> NumericTypeInterface;

	/** The main sequencer interface. */
	TWeakPtr<FSequencer> SequencerPtr;

	/** Cached settings provided to the sequencer itself on creation. */
	USequencerSettings* Settings;

	/** Whether the box was hidden. */
	bool WasHidden;
};
