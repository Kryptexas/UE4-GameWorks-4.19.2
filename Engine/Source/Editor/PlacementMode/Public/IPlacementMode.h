// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

class IPlacementMode
{
public:
	/** @return the event that is broadcast whenever the placement mode exits a placing session */
	DECLARE_EVENT_OneParam( IPlacementMode, FOnStoppedPlacingEvent, bool /*bWasSuccessfullyPlaced*/ );
	virtual FOnStoppedPlacingEvent& OnStoppedPlacing() = 0;

	/** @return the event that is broadcast whenever the placement mode enters a placing session */
	DECLARE_EVENT_OneParam( IPlacementMode, FOnStartedPlacingEvent, const TArray< UObject* >& /*Assets*/ );
	virtual FOnStartedPlacingEvent& OnStartedPlacing() = 0;

	/** Ends the current placing session in failure, if one is active. Does nothing otherwise. */
	virtual void StopPlacing() = 0;

	/** @return whether currently in a placing session */
	virtual bool IsCurrentlyPlacing() const = 0;

	/** Start a placing session using the specified assets and factory. If no factory is specified the last used factory will be used. */
	virtual void StartPlacing( const TArray< UObject* >& Assets, UActorFactory* Factory = NULL ) = 0;

	/** @return the actor factory currently used for the active or last placing session. */
	virtual UActorFactory* GetPlacingFactory() const = 0;

	/** Changes the actor factory used for the active or next placing session. */
	virtual void SetPlacingFactory( UActorFactory* Factory ) = 0;

	/** @return the last used actor factory when placing a specific asset type. Will return NULL if asset type has never been placed. */
	virtual UActorFactory* FindLastUsedFactoryForAssetType( UObject* Asset ) const = 0;

	/** Adds a widget which when focused will not end the active placing session. */
	virtual void AddValidFocusTargetForPlacement( const TWeakPtr< SWidget >& Widget ) = 0;

	/** Removes a widget which when focused would have not ended the active placing session. */
	virtual void RemoveValidFocusTargetForPlacement( const TWeakPtr< SWidget >& Widget ) = 0;

	/** @return the event that is broadcasted whenever the recently placed actors change  */
	DECLARE_EVENT_OneParam( IPlacementMode, FOnRecentlyPlacedChanged, const TArray< FActorPlacementInfo >& /*NewRecentlyPlaced*/ );
	virtual FOnRecentlyPlacedChanged& OnRecentlyPlacedChanged() = 0;

	/** Adds a set of actors and an optional factory to the recently placed list*/
	virtual void AddToRecentlyPlaced( const TArray< UObject* >& PlacedObjects, UActorFactory* FactoryUsed = NULL ) = 0;

	/** @return an array of actor placement information of the recent placements */
	virtual const TArray< FActorPlacementInfo >& GetRecentlyPlaced() const = 0;

	/** @return an array of the objects curently being placed */
	virtual const TArray< TWeakObjectPtr<UObject> >& GetCurrentlyPlacingObjects() const = 0;
};
