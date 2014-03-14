// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "IPropertyTableUtilities.h"
#include "IPropertyTableColumn.h"
#include "IPropertyTable.h"
#include "IPropertyTableCell.h"
#include "IPropertyTableRow.h"
#include "PropertyTableCell.h"
#include "DataSource.h"
#include "PropertyPath.h"

class FPropertyTableRowHeaderColumn : public TSharedFromThis< FPropertyTableRowHeaderColumn >, public IPropertyTableColumn
{
public:

	FPropertyTableRowHeaderColumn( const TSharedRef< IPropertyTable >& InTable )
		: Table( InTable )
		, Cells()
		, bIsHidden( false )
		, DataSource( MakeShareable( new NoDataSource() ) )
	{

	}

	virtual FName GetId() const OVERRIDE { return FName( TEXT("RowHeader") ); }
	virtual FText GetDisplayName() const OVERRIDE { return FText::GetEmpty(); }
	virtual TSharedRef< IDataSource > GetDataSource() const OVERRIDE { return MakeShareable( new PropertyPathDataSource( FPropertyPath::CreateEmpty() ) ); }
	virtual TSharedRef< class FPropertyPath > GetPartialPath() const OVERRIDE { return FPropertyPath::CreateEmpty(); }

	virtual TSharedRef< class IPropertyTableCell > GetCell( const TSharedRef< class IPropertyTableRow >& Row ) OVERRIDE
	{
		//@todo Clean Cells cache when rows get updated [11/27/2012 Justin.Sargent]
		TSharedRef< IPropertyTableCell >* CellPtr = Cells.Find( Row );

		if( CellPtr != NULL )
		{
			return *CellPtr;
		}

		TSharedRef< IPropertyTableCell > Cell = MakeShareable( new FPropertyTableCell( SharedThis( this ), Row ) );
		Cells.Add( Row, Cell );

		return Cell;
	}

	virtual void RemoveCellsForRow( const TSharedRef< class IPropertyTableRow >& Row ) OVERRIDE
	{
		Cells.Remove( Row );
	}

	virtual TSharedRef< class IPropertyTable > GetTable() const OVERRIDE { return Table.Pin().ToSharedRef(); }

	virtual bool CanSelectCells() const OVERRIDE { return false; }

	virtual EPropertyTableColumnSizeMode::Type GetSizeMode() const OVERRIDE { return EPropertyTableColumnSizeMode::Fixed; }

	virtual float GetWidth() const OVERRIDE { return 20.0f; } 

	virtual void SetWidth( float InWidth ) OVERRIDE {  }

	virtual bool IsHidden() const OVERRIDE { return bIsHidden; }

	virtual void SetHidden( bool InIsHidden ) OVERRIDE { bIsHidden = InIsHidden; }

	virtual bool IsFrozen() const OVERRIDE { return true; }

	virtual void SetFrozen( bool InIsFrozen ) OVERRIDE {}

	virtual bool CanSortBy() const OVERRIDE { return false; }

	virtual void Sort( TArray< TSharedRef< class IPropertyTableRow > >& Rows, const EColumnSortMode::Type SortMode ) OVERRIDE {  }

	virtual void Tick() OVERRIDE {}

private:

	TWeakPtr< IPropertyTable > Table;
	TMap< TSharedRef< IPropertyTableRow >, TSharedRef< class IPropertyTableCell > > Cells;
	bool bIsHidden;
	TSharedRef< IDataSource > DataSource;
};