// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#ifndef __LevelViewportContextMenu_h__
#define __LevelViewportContextMenu_h__

#pragma once


/**
 * Unreal editor main frame Slate widget
 */
class FLevelViewportContextMenu
{

public:

	/**
	 * Summons the level viewport context menu
	 */
	static void SummonMenu( const TSharedRef< class SLevelEditor >& LevelEditor );

	/**
	 * Creates a widget for the context menu that can be inserted into a pop-up window
	 *
	 * @param	LevelEditor		The level editor using this menu.
	 * @param	Extender		Allows extension of this menu based on context.
	 * @return	Widget for this context menu
	 */
	static TSharedPtr< SWidget > BuildMenuWidget( TWeakPtr< SLevelEditor > LevelEditor, TSharedPtr<FExtender> Extender = TSharedPtr<FExtender>() );


private:

	/**
	 * Builds the actor group menu
	 *
	 * @param MenuBuilder		The menu builder to add items to.
	 * @param SelectedActorInfo	Information about the selected actors.
	 */
	static void BuildGroupMenu( FMenuBuilder& MenuBuilder, const struct FSelectedActorInfo& SelectedActorInfo );
};

#endif		// __LevelViewportContextMenu_h__