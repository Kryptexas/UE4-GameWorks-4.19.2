// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * The public interface for the Scene Outliner widget
 */
class ISceneOutliner : public SCompoundWidget
{

public:

	/** Sends a requests to the Scene Outliner to refresh itself the next chance it gets */
	virtual void Refresh() = 0;

	/** @return Returns a string to use for highlighting results in the outliner list */
	virtual FText GetFilterHighlightText() const = 0;
};