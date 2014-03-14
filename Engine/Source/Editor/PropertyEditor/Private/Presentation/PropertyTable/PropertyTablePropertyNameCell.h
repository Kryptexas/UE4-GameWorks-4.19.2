// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "IPropertyTableCell.h"
#include "IPropertyTableColumn.h"
#include "IPropertyTableRow.h"

class FPropertyTablePropertyNameCell : public TSharedFromThis< FPropertyTablePropertyNameCell >, public IPropertyTableCell
{
public:

	FPropertyTablePropertyNameCell( const TSharedRef< class FPropertyTablePropertyNameColumn >& InColumn, const TSharedRef< class IPropertyTableRow >& InRow );

	// Begin IPropertyTableCell Interface

	virtual void EnterEditMode() OVERRIDE;

	virtual void ExitEditMode() OVERRIDE;

	virtual TSharedRef< class IPropertyTableColumn > GetColumn() const OVERRIDE { return Column.Pin().ToSharedRef(); }

	virtual TSharedPtr< class FPropertyNode > GetNode() const OVERRIDE { return NULL; }

	virtual TWeakObjectPtr< UObject > GetObject() const OVERRIDE;

	virtual TSharedRef< class IPropertyTableRow > GetRow() const OVERRIDE { return Row.Pin().ToSharedRef(); }

	virtual TSharedRef< class IPropertyTable > GetTable() const OVERRIDE;

	virtual FString GetValueAsString() const OVERRIDE;

	virtual FText GetValueAsText() const OVERRIDE;

	virtual bool InEditMode() const OVERRIDE { return bInEditMode; }

	virtual bool IsReadOnly() const OVERRIDE { return true; }

	virtual bool IsBound() const OVERRIDE { return bIsBound; }

	virtual bool IsValid() const OVERRIDE  { return true; }

	DECLARE_DERIVED_EVENT( FPropertyTablePropertyNameCell, IPropertyTableCell::FEnteredEditModeEvent, FEnteredEditModeEvent );
	virtual FEnteredEditModeEvent& OnEnteredEditMode() OVERRIDE { return EnteredEditModeEvent; }

	DECLARE_DERIVED_EVENT( FPropertyTablePropertyNameCell, IPropertyTableCell::FExitedEditModeEvent, FExitedEditModeEvent  );
	virtual FExitedEditModeEvent& OnExitedEditMode() OVERRIDE { return ExitedEditModeEvent; }

	virtual void Refresh() OVERRIDE;

	virtual void SetValueFromString( const FString& InString ) OVERRIDE {}

	// End IPropertyTableCell Interface

private:

	/** Is this cell being edited? */
	bool bInEditMode;

	/** Is this cell valid? */
	bool bIsBound;

	/** The column this cell is in */
	TWeakPtr< class FPropertyTablePropertyNameColumn > Column;

	/** Delegate to execute when we enter edit mode */
	FEnteredEditModeEvent EnteredEditModeEvent;

	/** Delegate to execute when we exit edit mode */
	FExitedEditModeEvent ExitedEditModeEvent;

	/** The object node which is associated with this cell */
	TSharedPtr< class FObjectPropertyNode > ObjectNode;

	/** The row this cell is in */
	TWeakPtr< class IPropertyTableRow > Row;
};