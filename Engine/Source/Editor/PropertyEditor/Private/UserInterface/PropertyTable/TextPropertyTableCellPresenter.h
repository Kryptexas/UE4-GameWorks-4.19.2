// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "IPropertyTableCellPresenter.h"
#include "PropertyTableConstants.h"

class FTextPropertyTableCellPresenter : public TSharedFromThis< FTextPropertyTableCellPresenter >, public IPropertyTableCellPresenter
{
public:

	FTextPropertyTableCellPresenter( const TSharedRef< class FPropertyEditor >& InPropertyEditor, const TSharedRef< class IPropertyTableUtilities >& InPropertyUtilities, FSlateFontInfo InFont = FEditorStyle::GetFontStyle( PropertyTableConstants::NormalFontStyle ) );

	virtual TSharedRef< class SWidget > ConstructDisplayWidget() OVERRIDE;

	virtual bool RequiresDropDown() OVERRIDE;

	virtual TSharedRef< class SWidget > ConstructEditModeCellWidget() OVERRIDE;

	virtual TSharedRef< class SWidget > ConstructEditModeDropDownWidget() OVERRIDE;

	virtual TSharedRef< class SWidget > WidgetToFocusOnEdit() OVERRIDE;

	virtual FString GetValueAsString() OVERRIDE;

	virtual FText GetValueAsText() OVERRIDE;

	virtual bool HasReadOnlyEditMode() OVERRIDE { return HasReadOnlyEditingWidget; }


private:

	bool CalculateIfUsingReadOnlyEditingWidget() const;


private:

	TSharedPtr< class SWidget > PropertyWidget; 

	TSharedRef< class FPropertyEditor > PropertyEditor;
	TSharedRef< class IPropertyTableUtilities > PropertyUtilities;

	bool HasReadOnlyEditingWidget;
	FSlateFontInfo Font;
};