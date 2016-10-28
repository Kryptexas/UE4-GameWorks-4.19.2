// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

class FEventGraphSample;


/**
 * An advanced tooltip used to show various informations in the event graph widget.
 */
class SEventGraphTooltip
{
public:

	static TSharedPtr<SToolTip> GetTableCellTooltip( const TSharedPtr<FEventGraphSample> EventSample );

protected:

	static EVisibility GetHotPathIconVisibility(const TSharedPtr<FEventGraphSample> EventSample);
};
