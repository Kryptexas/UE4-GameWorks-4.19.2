// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

struct SEQUENCER_API FSequencerUtilities
{
	static TSharedRef<SWidget> MakeAddButton(FText HoverText, FOnGetContent MenuContent, const TAttribute<bool>& HoverState);
};