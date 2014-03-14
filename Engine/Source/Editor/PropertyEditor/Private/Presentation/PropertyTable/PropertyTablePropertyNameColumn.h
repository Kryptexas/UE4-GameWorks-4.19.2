// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "IPropertyTableUtilities.h"
#include "IPropertyTableColumn.h"
#include "IPropertyTable.h"
#include "IPropertyTableCell.h"
#include "IPropertyTableRow.h"
#include "PropertyTablePropertyNameCell.h"
#include "DataSource.h"
#include "PropertyPath.h"

#define LOCTEXT_NAMESPACE "PropertyNameColumnHeader"

class FPropertyTablePropertyNameColumn : public TSharedFromThis< FPropertyTablePropertyNameColumn >, public IPropertyTableColumn
{
public:

	FPropertyTablePropertyNameColumn( const TSharedRef< IPropertyTable >& InTable );

	// Begin IPropertyTableColumn Interface

	virtual bool CanSelectCells() const OVERRIDE { return true; }

	virtual bool CanSortBy() const OVERRIDE { return true; }

	virtual TSharedRef< class IPropertyTableCell > GetCell( const TSharedRef< class IPropertyTableRow >& Row ) OVERRIDE;

	virtual TSharedRef< IDataSource > GetDataSource() const OVERRIDE { return DataSource; }

	virtual TSharedRef< class FPropertyPath > GetPartialPath() const OVERRIDE { return FPropertyPath::CreateEmpty(); }

	virtual FText GetDisplayName() const OVERRIDE { return LOCTEXT( "DisplayName", "Name" ); }

	virtual FName GetId() const OVERRIDE { return FName( TEXT("PropertyName") ); }

	virtual EPropertyTableColumnSizeMode::Type GetSizeMode() const OVERRIDE { return EPropertyTableColumnSizeMode::Fill; }

	virtual TSharedRef< class IPropertyTable > GetTable() const OVERRIDE { return Table.Pin().ToSharedRef(); }

	virtual float GetWidth() const OVERRIDE { return Width; }

	virtual bool IsFrozen() const OVERRIDE { return false; }

	virtual bool IsHidden() const OVERRIDE { return bIsHidden; }

	virtual void RemoveCellsForRow( const TSharedRef< class IPropertyTableRow >& Row ) OVERRIDE
	{
		Cells.Remove( Row );
	}

	virtual void SetFrozen( bool InIsFrozen ) OVERRIDE {}

	virtual void SetHidden( bool InIsHidden ) OVERRIDE { bIsHidden = InIsHidden; }

	virtual void SetWidth( float InWidth ) OVERRIDE { Width = InWidth; }

	virtual void Sort( TArray< TSharedRef< class IPropertyTableRow > >& Rows, const EColumnSortMode::Type SortMode ) OVERRIDE;

	virtual void Tick() OVERRIDE {}

	// End IPropertyTableColumn Interface

private:

	FString GetPropertyNameAsString( const TSharedRef< IPropertyTableRow >& Row );

private:

	/** Has this column been hidden? */
	bool bIsHidden;

	/** A map of all cells in this column */
	TMap< TSharedRef< IPropertyTableRow >, TSharedRef< class IPropertyTableCell > > Cells;

	/** The data source for this column */
	TSharedRef< IDataSource > DataSource;

	/** A reference to the owner table */
	TWeakPtr< IPropertyTable > Table;

	/** The width of the column */
	float Width;
};

#undef LOCTEXT_NAMESPACE