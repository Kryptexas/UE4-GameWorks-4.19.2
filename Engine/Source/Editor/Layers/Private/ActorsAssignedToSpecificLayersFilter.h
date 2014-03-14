// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once


class FActorsAssignedToSpecificLayersFilter : public IFilter< const AActor* const >, public TSharedFromThis< FActorsAssignedToSpecificLayersFilter >
{
public:

	DECLARE_DERIVED_EVENT(FActorsAssignedToSpecificLayersFilter, IFilter< const AActor* const >::FChangedEvent, FChangedEvent);
	virtual FChangedEvent& OnChanged() OVERRIDE { return ChangedEvent; }

	/** 
	 * Returns whether the specified Item passes the Filter's text restrictions 
	 *
	 *	@param	InItem	The Item to check 
	 *	@return			Whether the specified Item passed the filter
	 */
	virtual bool PassesFilter( const AActor* const InActor ) const OVERRIDE
	{
		if( LayerNames.Num() == 0 )
		{
			return false;
		}

		if( InActor->Layers.Num() < LayerNames.Num() )
		{
			return false;
		}

		bool bBelongsToLayers = true;
		for( auto LayerIter = LayerNames.CreateConstIterator(); bBelongsToLayers && LayerIter; ++LayerIter )
		{
			bBelongsToLayers = InActor->Layers.Contains( *LayerIter );
		}

		return bBelongsToLayers;
	}

	/**
	 *	
	 */
	void SetLayers( const TArray< FName >& InLayerNames )
	{
		LayerNames.Empty();

		for( auto LayerNameIter = InLayerNames.CreateConstIterator(); LayerNameIter; ++LayerNameIter )
		{
			LayerNames.AddUnique( *LayerNameIter );
		}

		ChangedEvent.Broadcast();
	}

	/**
	 *	
	 */
	void AddLayer( FName LayerName )
	{
		LayerNames.AddUnique( LayerName );
		ChangedEvent.Broadcast();
	}

	/**
	 *	
	 */
	bool RemoveLayer( FName LayerName )
	{
		return LayerNames.Remove( LayerName ) > 0;
		ChangedEvent.Broadcast();
	}

	/**
	 *	
	 */
	void ClearLayers()
	{
		LayerNames.Empty();
		ChangedEvent.Broadcast();
	}


private:

	/**	The list of layer names which actors need to belong to */
	TArray< FName > LayerNames;

	/**	The event that broadcasts whenever a change occurs to the filter */
	FChangedEvent ChangedEvent;
};