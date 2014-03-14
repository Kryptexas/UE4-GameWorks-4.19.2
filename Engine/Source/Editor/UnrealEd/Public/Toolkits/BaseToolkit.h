// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IToolkit.h"


/**
 * Base class for all toolkits (abstract)
 */
class UNREALED_API FBaseToolkit : public IToolkit
{

public:

	/** FBaseToolkit constructor */
	FBaseToolkit();

	/** Virtual destructor */
	virtual ~FBaseToolkit();

	/** IToolkit interface */
	virtual bool ProcessCommandBindings( const FKeyboardEvent& InKeyboardEvent ) const OVERRIDE;
	virtual bool IsHosted() const OVERRIDE;
	virtual const TSharedRef< class IToolkitHost > GetToolkitHost() const OVERRIDE;
	virtual const TMap< EToolkitTabSpot::Type, TArray< TWeakPtr< SDockableTab > > >& GetToolkitTabsInSpots() const OVERRIDE;
	virtual void BringToolkitToFront() OVERRIDE;
	virtual TSharedPtr<class SWidget> GetInlineContent() const OVERRIDE;
	virtual bool IsBlueprintEditor() const OVERRIDE;

	/** @return	Returns true if this is a world-centric asset editor.  That is, the user is editing the asset inline in a Level Editor app. */
	bool IsWorldCentricAssetEditor() const;	

	/** @returns Returns our toolkit command list */
	const TSharedRef< FUICommandList > GetToolkitCommands() const
	{
		return ToolkitCommands;
	}

	/**
	 * Adds an already-created toolkit tab to the toolkit.  Used for tabs that have no tab identifier, such as a "document" tab
	 *
	 * @param	TabToAdd		The dockable tab object to add
	 * @param	TabSpot			Which spot to spawn this tab into
	 */
	void AddToolkitTab( const TSharedRef< SDockableTab >& TabToAdd, const EToolkitTabSpot::Type TabSpot );

protected:

	/** @return Returns the prefix string to use for tabs created for this toolkit.  In world-centric mode, tabs get a
	    name prefix to make them distinguishable from other tabs */
	FString GetTabPrefix() const;

	/** @return Returns the color to use for tabs created for this toolkit.  In world-centric mode, tabs may be colored to
	    make them more easy to distinguish compared to other tabs. */
	FLinearColor GetTabColorScale() const;


protected:

	/** Asset editing mode, set at creation-time and never changes */
	EToolkitMode::Type ToolkitMode;

	/** List of UI commands for this toolkit.  This should be filled in by the derived class! */
	TSharedRef< FUICommandList > ToolkitCommands;

	/** The host application for this editor.  If editing in world-centric mode, this is the level level editor that we're editing the asset within 
	    Use GetToolkitHost() method to access this member. */
	TWeakPtr< class IToolkitHost > ToolkitHost;

	/** Map of toolkit tab spots to known tabs (these are weak pointers and may be invalid after tabs are closed.) */
	TMap< EToolkitTabSpot::Type, TArray< TWeakPtr< SDockableTab > > > ToolkitTabsInSpots;
};



/**
 * Base class for all editor mode toolkits.
 */
class UNREALED_API FModeToolkit : public FBaseToolkit, public TSharedFromThis<FModeToolkit>
{
public:
	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE {}
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE {}

	/** Initializes the mode toolkit */
	virtual void Init(const TSharedPtr< class IToolkitHost >& InitToolkitHost);

	/** IToolkit interface */
	virtual FText GetToolkitName() const OVERRIDE { return GetBaseToolkitName(); }
	virtual FString GetWorldCentricTabPrefix() const OVERRIDE;
	virtual bool IsAssetEditor() const OVERRIDE;
	virtual const TArray< UObject* >* GetObjectsCurrentlyBeingEdited() const OVERRIDE;
	virtual FLinearColor GetWorldCentricTabColorScale() const OVERRIDE;
};


