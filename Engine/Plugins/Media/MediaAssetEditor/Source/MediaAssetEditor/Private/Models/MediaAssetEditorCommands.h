// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Holds the UI commands for the MediaAssetEditorToolkit widget.
 */
class FMediaAssetEditorCommands
	: public TCommands<FMediaAssetEditorCommands>
{
public:

	/**
	 * Default constructor.
	 */
	FMediaAssetEditorCommands( ) 
		: TCommands<FMediaAssetEditorCommands>("MediaAssetEditor", NSLOCTEXT("Contexts", "MediaAssetEditor", "MediaAsset Editor"), NAME_None, "MediaAssetEditorStyle")
	{ }

public:

	// TCommands interface

	virtual void RegisterCommands( ) override;
	
public:

	/** Fast forwards media playback. */
	TSharedPtr<FUICommandInfo> ForwardMedia;

	/** Pauses media playback. */
	TSharedPtr<FUICommandInfo> PauseMedia;

	/** Starts media playback. */
	TSharedPtr<FUICommandInfo> PlayMedia;

	/** Reverses media playback. */
	TSharedPtr<FUICommandInfo> ReverseMedia;

	/** Rewinds the media to the beginning. */
	TSharedPtr<FUICommandInfo> RewindMedia;
};
