// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "IPropertyTableCellPresenter.h"

/** 
 * Boilerplate implementations of IPropertyTableCellPresenter functions.
 * Here so we dont have to override all these functions each time.
 */
class FStatsCellPresenter : public IPropertyTableCellPresenter
{
public:
	/** Begin IPropertyTableCellPresenter interface */
	virtual bool RequiresDropDown() OVERRIDE
	{
		return false;
	}

	virtual TSharedRef< class SWidget > ConstructEditModeCellWidget() OVERRIDE
	{
		return ConstructDisplayWidget();
	}

	virtual TSharedRef< class SWidget > ConstructEditModeDropDownWidget() OVERRIDE
	{
		return SNullWidget::NullWidget;
	}

	virtual TSharedRef< class SWidget > WidgetToFocusOnEdit() OVERRIDE
	{
		return SNullWidget::NullWidget;
	}

	virtual bool HasReadOnlyEditMode() OVERRIDE 
	{ 
		return true; 
	}

	virtual FString GetValueAsString() OVERRIDE
	{
		return Text.ToString();
	}

	virtual FText GetValueAsText() OVERRIDE
	{
		return Text;
	}
	/** End IPropertyTableCellPresenter interface */

protected:

	/** 
	 * The text we display - we just store this as we never need to *edit* properties in the stats viewer,
	 * only view them. Therefore we dont need to dynamically update this string.
	 */
	FText Text;
};
