// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "ModuleInterface.h"

#include "IFilter.h"
#include "FilterCollection.h"
#include "ISceneOutlinerColumn.h"
#include "ISceneOutliner.h"


/** Delegate used with the Scene Outliner in 'actor picking' mode.  You'll bind a delegate when the
    outliner widget is created, which will be fired off when an actor is selected in the list */
DECLARE_DELEGATE_OneParam( FOnActorPicked, AActor* );

DECLARE_DELEGATE_RetVal_OneParam( TSharedRef< ISceneOutlinerColumn >, FCreateSceneOutlinerColumnDelegate, const TWeakPtr< ISceneOutliner >& );
DECLARE_DELEGATE_OneParam( FCustomSceneOutlinerDeleteDelegate, const TArray< TWeakObjectPtr< AActor > >&  )

namespace ESceneOutlinerMode
{
	enum Type
	{
		/** Allows all actors to be browsed and selected; syncs selection with the editor; drag and drop attachment, stc. */
		ActorBrowsing,

		/** Sets the outliner to operate as an actor 'picker'. */
		ActorPicker,
	};
}

/** Types of actor data we can display in a 'custom' tree column */
namespace ECustomColumnMode
{
	enum Type
	{
		/** Empty column -- doesn't display anything */
		None = 0,

		/** Actor class */
		ActorClass,

		/** Level */
		Level,

		/** Layer */
		Layer,

		/** The socket the actor is attached to. */
		Socket,

		/** Actor's internal name (FName) */
		InternalName,

		/** Actor's number of uncached lights */
		UncachedLights,

		// ---

		/** Number of options */
		Count
	};
}

/**
 * Settings for the Scene Outliner set by the programmer before spawning an instance of the widget.  This
 * is used to modify the outliner's behavior in various ways, such as filtering in or out specific classes
 * of actors.
 */
class FSceneOutlinerInitializationOptions
{

public:

	/** Mode to operate in */
	ESceneOutlinerMode::Type Mode;

	/** True if we should draw the header row above the tree view */
	bool bShowHeaderRow;

	/** Optional collection of filters to use when filtering actors in the Scene Outliner */
	TSharedPtr< TFilterCollection< const AActor* const > > ActorFilters;

	/** Whether the Scene Outliner should display parenta actors in a Tree */
	bool bShowParentTree;

	/** Whether the Scene Outliner should expose its searchbox */
	bool bShowSearchBox;

	/** Broadcasts whenever the Scene Outliners selection changes */
	FSimpleMulticastDelegate::FDelegate OnSelectionChanged;

	/**	Invoked whenever the user attempts to delete an actor from within the Scene Outliner */
	FCustomSceneOutlinerDeleteDelegate CustomDelete;

	/**	A factory delegate for creating a custom column to show in the Scene Outliner */
	FCreateSceneOutlinerColumnDelegate CustomColumnFactory;

	/**	Set to non-zero to fix the width of the custom column otherwise it defaults to a % of the outliner width */
	float CustomColumnFixedWidth;

	/** If true, the search box will gain focus when the scene outliner is created */
	bool bFocusSearchBoxWhenOpened;

public:

	/** Constructor */
	FSceneOutlinerInitializationOptions()
		: Mode( ESceneOutlinerMode::ActorPicker )
		, bShowHeaderRow( true )
		, ActorFilters( new TFilterCollection< const AActor* const >() )
		, bShowParentTree( true )
		, bShowSearchBox( true )
		, CustomColumnFixedWidth( 0.0f )
		, bFocusSearchBoxWhenOpened( false )
	{

	}
};