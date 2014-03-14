// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MessagingDebuggerEndpointFilter.h: Declares the FMessagingDebuggerEndpointFilter class.
=============================================================================*/

#pragma once


/**
 * Type definition for shared pointers to instances of FMessagingDebuggerEndpointFilter.
 */
typedef TSharedPtr<class FMessagingDebuggerEndpointFilter> FMessagingDebuggerEndpointFilterPtr;

/**
 * Type definition for shared references to instances of FMessagingDebuggerEndpointFilter.
 */
typedef TSharedRef<class FMessagingDebuggerEndpointFilter> FMessagingDebuggerEndpointFilterRef;


/**
 * Implements a filter for the message endpoints list.
 */
class FMessagingDebuggerEndpointFilter
{
public:

	/**
	 * Filters the specified endpoint based on the current filter settings.
	 *
	 * @param EndpointInfo - The endpoint to filter.
	 *
	 * @return true if the endpoint passed the filter, false otherwise.
	 */
	bool FilterEndpoint( const FMessageTracerEndpointInfoPtr& EndpointInfo ) const
	{
		if (!EndpointInfo.IsValid())
		{
			return false;
		}

		// filter by search string
		TArray<FString> FilterSubstrings;
		
		if (FilterString.ParseIntoArray(&FilterSubstrings, TEXT(" "), true) > 0)
		{
			for (int32 SubstringIndex = 0; SubstringIndex < FilterSubstrings.Num(); ++SubstringIndex)
			{
				if (!EndpointInfo->Name.ToString().Contains(FilterSubstrings[SubstringIndex]))
				{
					return false;
				}
			}
		}

		return true;
	}

	/**
	 * Sets the filter string.
	 *
	 * @param InFilterString - The filter string to set.
	 */
	void SetFilterString( const FString& InFilterString )
	{
		FilterString = InFilterString;
		ChangedEvent.Broadcast();
	}

public:

	/**
	 * Gets an event delegate that is invoked when the filter settings changed.
	 *
	 * @return The event delegate.
	 */
	DECLARE_EVENT(FMessagingDebuggerEndpointFilter, FOnMessagingEndpointFilterChanged);
	FOnMessagingEndpointFilterChanged& OnChanged( )
	{
		return ChangedEvent;
	}

private:

	// Holds the filter string used to filter endpoints by their names.
	FString FilterString;

private:

	// Holds an event delegate that is invoked when the filter settings changed.
	FOnMessagingEndpointFilterChanged ChangedEvent;
};
