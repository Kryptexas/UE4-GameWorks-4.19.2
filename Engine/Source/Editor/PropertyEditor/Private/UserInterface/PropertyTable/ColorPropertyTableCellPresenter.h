// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "IPropertyTableCellPresenter.h"

class FColorPropertyTableCellPresenter : public TSharedFromThis< FColorPropertyTableCellPresenter >, public IPropertyTableCellPresenter
{
public:

	FColorPropertyTableCellPresenter( const TSharedRef< class FPropertyEditor >& InPropertyEditor, const TSharedRef< class IPropertyTableUtilities >& InPropertyUtilities );

	virtual TSharedRef< class SWidget > ConstructDisplayWidget() OVERRIDE;

	virtual bool RequiresDropDown() OVERRIDE;

	virtual TSharedRef< class SWidget > ConstructEditModeCellWidget() OVERRIDE;

	virtual TSharedRef< class SWidget > ConstructEditModeDropDownWidget() OVERRIDE;

	virtual TSharedRef< class SWidget > WidgetToFocusOnEdit() OVERRIDE;

	virtual FString GetValueAsString() OVERRIDE;

	virtual FText GetValueAsText() OVERRIDE;

	virtual bool HasReadOnlyEditMode() OVERRIDE { return false; }


private:

	TSharedPtr< class SWidget > FocusWidget;
	TSharedRef< class FPropertyEditor > PropertyEditor;
	TSharedRef< class IPropertyTableUtilities > PropertyUtilities;
};