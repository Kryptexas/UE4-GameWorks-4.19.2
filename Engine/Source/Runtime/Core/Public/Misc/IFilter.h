// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

/**
 *	A generic interface that represents a Filter of ItemType
 */
template< typename ItemType >
class IFilter
{
public:

	/** Returns whether the specified Item passes the Filter's restrictions */
	virtual bool PassesFilter( ItemType InItem ) const = 0;

	/** Broadcasts anytime the restrictions of the Filter changes */
	DECLARE_EVENT( IFilter<ItemType>, FChangedEvent );
	virtual FChangedEvent& OnChanged() = 0;
};