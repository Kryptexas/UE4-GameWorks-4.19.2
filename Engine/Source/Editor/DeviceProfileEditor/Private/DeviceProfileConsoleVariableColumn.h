// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DeviceProfileConsoleVariableColumn.h: Declares the FDeviceProfileConsoleVariableColumn class.
=============================================================================*/

#pragma once


#include "IPropertyTableCustomColumn.h"


/** Delegate triggered when user opts to edit CVars **/
DECLARE_DELEGATE_OneParam(FOnEditDeviceProfileCVarsRequestDelegate, const TWeakObjectPtr<UDeviceProfile>&);


/**
 * A property table custom column used to bring the user to an editor which 
 * will manage the list of Console Variables associated with the device profile
 */
class FDeviceProfileConsoleVariableColumn : public IPropertyTableCustomColumn
{
public:
	FDeviceProfileConsoleVariableColumn();

	/** Begin IPropertyTableCustomColumn interface */
	virtual bool Supports( const TSharedRef< IPropertyTableColumn >& Column, const TSharedRef< IPropertyTableUtilities >& Utilities ) const override;
	virtual TSharedPtr< SWidget > CreateColumnLabel( const TSharedRef< IPropertyTableColumn >& Column, const TSharedRef< IPropertyTableUtilities >& Utilities, const FName& Style ) const override;
	virtual TSharedPtr< IPropertyTableCellPresenter > CreateCellPresenter( const TSharedRef< IPropertyTableCell >& Cell, const TSharedRef< IPropertyTableUtilities >& Utilities, const FName& Style ) const override;
	/** End IPropertyTableCustomColumn interface */

	/**
	 * Delegate used to notify listeners that an edit request was triggered from the property table
	 *
	 * @return - Access to the delegate
	 */
	FOnEditDeviceProfileCVarsRequestDelegate& OnEditCVarsRequest()
	{
		return OnEditCVarsRequestDelegate;
	}

private:

	/** Delegate triggered when user opts to edit CVars **/
	FOnEditDeviceProfileCVarsRequestDelegate OnEditCVarsRequestDelegate;
};

