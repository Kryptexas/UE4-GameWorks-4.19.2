// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "IPropertyTableCellPresenter.h"

class FBooleanPropertyTableCellPresenter : public TSharedFromThis< FBooleanPropertyTableCellPresenter >, public IPropertyTableCellPresenter
{
public:

	FBooleanPropertyTableCellPresenter( const TSharedRef< class FPropertyEditor >& InPropertyEditor );

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
};