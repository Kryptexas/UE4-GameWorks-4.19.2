// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

/** Opaque state interface for saving and restoring viewport state */
struct IPersonaViewportState
{
};

/** Abstract viewport that can save and restore state */
class IPersonaViewport : public SCompoundWidget
{
public:
	/** Save the viewport state */
	virtual TSharedRef<IPersonaViewportState> SaveState() const = 0;

	/** Restore the viewport state */
	virtual void RestoreState(TSharedRef<IPersonaViewportState> InState) = 0;
};