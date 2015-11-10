// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Sequencer.h"

/**
 * Class responsible for generating a menu for the currently selected sections.
 * This is a shared class that's entirely owned by the context menu handlers.
 * Once the menu is closed, all references to this class are removed, and the 
 * instance is cleaned up. To ensure no weak references are held, AsShared is hidden
 */
struct FSectionContextMenu : TSharedFromThis<FSectionContextMenu>
{
	static void BuildMenu(FMenuBuilder& MenuBuilder, FSequencer& Sequencer);

private:

	/** Hidden AsShared() methods to discourage CreateSP delegate use. */
	TSharedRef<FSectionContextMenu> AsShared() { return TSharedFromThis::AsShared(); }
	TSharedRef<const FSectionContextMenu> AsShared() const { return TSharedFromThis::AsShared(); }

	FSectionContextMenu(FSequencer& InSeqeuncer)
		: Sequencer(StaticCastSharedRef<FSequencer>(InSeqeuncer.AsShared()))
	{}

	void PopulateMenu(FMenuBuilder& MenuBuilder);

	/** Add edit menu for trim and split */
	void AddEditMenu(FMenuBuilder& MenuBuilder);

	/** Add extrapolation menu for pre and post infinity */
	void AddExtrapolationMenu(FMenuBuilder& MenuBuilder, bool bPreInfinity);

	void SelectAllKeys();

	bool CanSelectAllKeys() const;

	void TrimSection(bool bTrimLeft);

	void SplitSection();

	bool IsTrimmable() const;

	void SetExtrapolationMode(ERichCurveExtrapolation ExtrapMode, bool bPreInfinity);

	bool IsExtrapolationModeSelected(ERichCurveExtrapolation ExtrapMode, bool bPreInfinity) const;

	void ToggleSectionActive();

	bool IsToggleSectionActive() const;

	void DeleteSection();

	/** The key itself */
	TSharedRef<FSequencer> Sequencer;
};

/**
 * Class responsible for generating a menu for the currently selected keys.
 * This is a shared class that's entirely owned by the context menu handlers.
 * Once the menu is closed, all references to this class are removed, and the 
 * instance is cleaned up. To ensure no weak references are held, AsShared is hidden
 */
struct FKeyContextMenu : TSharedFromThis<FKeyContextMenu>
{
	static void BuildMenu(FMenuBuilder& MenuBuilder, FSequencer& Sequencer);

private:

	FKeyContextMenu(FSequencer& InSeqeuncer)
		: Sequencer(StaticCastSharedRef<FSequencer>(InSeqeuncer.AsShared()))
	{}

	/** Hidden AsShared() methods to discourage CreateSP delegate use. */
	TSharedRef<FKeyContextMenu> AsShared() { return TSharedFromThis::AsShared(); }
	TSharedRef<const FKeyContextMenu> AsShared() const { return TSharedFromThis::AsShared(); }

	void PopulateMenu(FMenuBuilder& MenuBuilder);

	/** The key itself */
	TSharedRef<FSequencer> Sequencer;
};