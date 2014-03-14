// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once


/*=============================================================================
	AutomationFilter.h: Declares the AutomationFilter class.
=============================================================================*/

class FAutomationFilter : public IFilter< const TSharedPtr< class IAutomationReport >&  >
{
public:

	FAutomationFilter()
		: OnlySmokeTests( false )
		, ShowErrors( false )
		, ShowWarnings( false )
	{}

	// Begin IFilter functions
	DECLARE_DERIVED_EVENT(FAutomationFilter, IFilter< const TSharedPtr< class IAutomationReport >& >::FChangedEvent, FChangedEvent);
	virtual FChangedEvent& OnChanged() OVERRIDE { return ChangedEvent; }

	/**
	 * Checks if the report passes the filter
	 * @param InReport - The automation report
	 * @return true if it passes the test
	 */
	virtual bool PassesFilter( const TSharedPtr< IAutomationReport >& InReport ) const OVERRIDE
	{
		bool FilterPassed = true;
		if( OnlySmokeTests && !InReport->IsSmokeTest( ) )
		{
			FilterPassed = false;
		}

		if( ShowErrors || ShowWarnings )
		{
		 	FilterPassed = InReport->HasErrors() || InReport->HasWarnings();
		}

		return FilterPassed;
	}

	// End IFilter functions


	/**
	 * Set if we should only show warnings
	 * @param InShowWarnings If we should show warnings or not
	 */
	void SetShowWarnings( const bool InShowWarnings )
	{
		ShowWarnings = InShowWarnings;
	}

	/**
	 * Should we show warnings
	 * @return True if we should be showing warnings
	 */
	const bool ShouldShowWarnings( ) const
	{
		return ShowWarnings;
	}

	/**
	 * Set if we should only show errors
	 * @param InShowErrors If we should show errors
	 */
	void SetShowErrors( const bool InShowErrors )
	{
		ShowErrors = InShowErrors;
	}

	/**
	 * Should we show errors
	 * @return True if we should be showing errors
	 */
	const bool ShouldShowErrors( ) const
	{
		return ShowErrors;
	}

	/**
	 * Set if we should only show smoke tests
	 * @param InOnlySmokeTests If we should show smoke tests
	 */
	void SetOnlyShowSmokeTests( const bool InOnlySmokeTests )
	{
		OnlySmokeTests = InOnlySmokeTests;
	}

	/**
	 * Should we only show errors
	 * @return True if we should be showing errors
	 */
	const bool OnlyShowSmokeTests( ) const
	{
		return OnlySmokeTests;
	}

private:

	/**	The event that broadcasts whenever a change occurs to the filter */
	FChangedEvent ChangedEvent;
	
	/** Only smoke tests will pass the test **/
	bool OnlySmokeTests;

	/** Only errors will pass the test **/
	bool ShowErrors;

	/** Only warnings will pass the test **/
	bool ShowWarnings;
};