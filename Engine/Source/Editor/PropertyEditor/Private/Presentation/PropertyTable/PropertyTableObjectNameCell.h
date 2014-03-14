// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "IPropertyTableCell.h"
#include "IPropertyTableColumn.h"
#include "IPropertyTableRow.h"

class FPropertyTableObjectNameCell : public TSharedFromThis< FPropertyTableObjectNameCell >, public IPropertyTableCell
{
public:

	FPropertyTableObjectNameCell( const TSharedRef< class FPropertyTableObjectNameColumn >& InColumn, const TSharedRef< class IPropertyTableRow >& InRow );

	virtual void Refresh() OVERRIDE;

	virtual bool IsReadOnly() const OVERRIDE;
	virtual bool IsBound() const OVERRIDE { return bIsBound; }
	virtual bool InEditMode() const OVERRIDE { return bInEditMode; }
	virtual bool IsValid() const OVERRIDE;

	virtual FString GetValueAsString() const OVERRIDE;
	virtual FText GetValueAsText() const OVERRIDE;
	virtual void SetValueFromString( const FString& InString ) OVERRIDE {}

	virtual TWeakObjectPtr< UObject > GetObject() const OVERRIDE;

	virtual TSharedPtr< class FPropertyNode > GetNode() const OVERRIDE { return NULL; }
	virtual TSharedRef< class IPropertyTableColumn > GetColumn() const OVERRIDE { return Column.Pin().ToSharedRef(); }
	virtual TSharedRef< class IPropertyTableRow > GetRow() const OVERRIDE { return Row.Pin().ToSharedRef(); }
	virtual TSharedRef< class IPropertyTable > GetTable() const OVERRIDE;
	virtual TSharedPtr< class IPropertyHandle > GetPropertyHandle() const OVERRIDE { return NULL; }

	virtual void EnterEditMode() OVERRIDE;
	virtual void ExitEditMode() OVERRIDE;

	DECLARE_DERIVED_EVENT( FPropertyTableObjectNameCell, IPropertyTableCell::FEnteredEditModeEvent, FEnteredEditModeEvent );
	virtual FEnteredEditModeEvent& OnEnteredEditMode() OVERRIDE { return EnteredEditModeEvent; }

	DECLARE_DERIVED_EVENT( FPropertyTableObjectNameCell, IPropertyTableCell::FExitedEditModeEvent, FExitedEditModeEvent  );
	virtual FExitedEditModeEvent& OnExitedEditMode() OVERRIDE { return ExitedEditModeEvent; }


private:

	FEnteredEditModeEvent EnteredEditModeEvent;
	FExitedEditModeEvent ExitedEditModeEvent;

	TWeakPtr< class FPropertyTableObjectNameColumn > Column;
	TWeakPtr< class IPropertyTableRow > Row;

	TSharedPtr< class FObjectPropertyNode > ObjectNode;
	FString ObjectName;

	bool bIsBound;
	bool bInEditMode;
};