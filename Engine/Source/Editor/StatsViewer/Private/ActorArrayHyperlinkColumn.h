// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "IPropertyTableCustomColumn.h"

/**
 * A property table custom column used to display names of objects
 * that can be clicked on to jump to the objects in the scene or 
 * content browser.
 */
class FActorArrayHyperlinkColumn : public IPropertyTableCustomColumn
{
public:
	/** Begin IPropertyTableCustomColumn interface */
	virtual bool Supports( const TSharedRef< IPropertyTableColumn >& Column, const TSharedRef< IPropertyTableUtilities >& Utilities ) const OVERRIDE;
	virtual TSharedPtr< SWidget > CreateColumnLabel( const TSharedRef< IPropertyTableColumn >& Column, const TSharedRef< IPropertyTableUtilities >& Utilities, const FName& Style ) const OVERRIDE;
	virtual TSharedPtr< IPropertyTableCellPresenter > CreateCellPresenter( const TSharedRef< IPropertyTableCell >& Cell, const TSharedRef< IPropertyTableUtilities >& Utilities, const FName& Style ) const OVERRIDE;
	/** End IPropertyTableCustomColumn interface */
};

