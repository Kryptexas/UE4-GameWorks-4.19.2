// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "IPropertyTable.h"

class FPropertyTable : public TSharedFromThis< FPropertyTable >, public IPropertyTable
{
public: 

	FPropertyTable();

	virtual void Tick() OVERRIDE;

	virtual void RequestRefresh() OVERRIDE;

	virtual class FNotifyHook* GetNotifyHook() const OVERRIDE;
	virtual bool AreFavoritesEnabled() const OVERRIDE;
	virtual void ToggleFavorite( const TSharedRef< class FPropertyEditor >& PropertyEditor ) const OVERRIDE;
	virtual void CreateColorPickerWindow( const TSharedRef< class FPropertyEditor >& PropertyEditor, bool bUseAlpha ) const OVERRIDE;
	virtual void EnqueueDeferredAction( FSimpleDelegate DeferredAction ) OVERRIDE;
	virtual TSharedPtr<class FAssetThumbnailPool> GetThumbnailPool() const OVERRIDE;
	virtual void NotifyFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE {}

	virtual bool GetIsUserAllowedToChangeRoot() OVERRIDE;
	virtual void SetIsUserAllowedToChangeRoot( bool InAllowUserToChangeRoot ) OVERRIDE;

	virtual void AddColumn( const TWeakObjectPtr< UObject >& Object ) OVERRIDE;
	virtual void AddColumn( const TWeakObjectPtr< UProperty >& Property ) OVERRIDE;
	virtual void AddColumn( const TSharedRef< FPropertyPath >& PropertyPath ) OVERRIDE;
	virtual void AddColumn( const TSharedRef< class IPropertyTableColumn >& Column ) OVERRIDE;
	virtual void RemoveColumn( const TSharedRef< class IPropertyTableColumn >& Column ) OVERRIDE;

	virtual void AddRow( const TWeakObjectPtr< UObject >& Object ) OVERRIDE;
	virtual void AddRow( const TWeakObjectPtr< UProperty >& Property ) OVERRIDE;
	virtual void AddRow( const TSharedRef< FPropertyPath >& PropertyPath ) OVERRIDE;
	virtual void AddRow( const TSharedRef< class IPropertyTableRow >& Row ) OVERRIDE;
	virtual void RemoveRow( const TSharedRef< class IPropertyTableRow >& Row ) OVERRIDE;

	virtual const EPropertyTableOrientation::Type GetOrientation() const OVERRIDE { return Orientation; }
	virtual void SetOrientation( EPropertyTableOrientation::Type InOrientation ) OVERRIDE;

	virtual void SetRootPath( const TSharedPtr< FPropertyPath >& Path ) OVERRIDE;
	virtual TSharedRef< FPropertyPath > GetRootPath() const OVERRIDE { return RootPath; }
	virtual TArray< FPropertyInfo > GetPossibleExtensionsForPath( const TSharedRef< FPropertyPath >& Path ) const OVERRIDE;

	virtual void GetSelectedObjects( TArray< TWeakObjectPtr< UObject > >& OutSelectedObjects) const OVERRIDE;

	virtual const TArray< TWeakObjectPtr< UObject > >& GetObjects() const OVERRIDE { return SourceObjects; }
	virtual void SetObjects( const TArray< TWeakObjectPtr< UObject > >& Objects ) OVERRIDE;
	virtual void SetObjects( const TArray< UObject* >& Objects ) OVERRIDE;

	virtual TSharedRef< class FObjectPropertyNode > GetObjectPropertyNode( const TSharedRef< IPropertyTableColumn >& Column, const TSharedRef< IPropertyTableRow >& Row ) OVERRIDE;
	virtual TSharedRef< class FObjectPropertyNode > GetObjectPropertyNode( const TWeakObjectPtr< UObject >& Object ) OVERRIDE;

	virtual bool GetShowRowHeader() const OVERRIDE { return ShowRowHeader; }
	virtual void SetShowRowHeader( const bool InShowRowHeader ) OVERRIDE;

	virtual bool GetShowObjectName() const OVERRIDE { return ShowObjectName; }
	virtual void SetShowObjectName( const bool ShowObjectName ) OVERRIDE;

	virtual const TArray< TSharedRef< class IPropertyTableColumn > >& GetColumns() OVERRIDE { return Columns; }
	virtual TArray< TSharedRef< class IPropertyTableRow > >& GetRows() OVERRIDE { return Rows; }

	virtual const TSet< TSharedRef< class IPropertyTableRow > >& GetSelectedRows() OVERRIDE { return SelectedRows; }
	virtual void SetSelectedRows( const TSet< TSharedRef< IPropertyTableRow > >& InSelectedRows ) OVERRIDE;

	virtual const TSet< TSharedRef< class IPropertyTableCell > >& GetSelectedCells() OVERRIDE { return SelectedCells; }
	virtual void SetSelectedCells( const TSet< TSharedRef< class IPropertyTableCell > >& InSelectedCells ) OVERRIDE;

	virtual void SelectCellRange( const TSharedRef< class IPropertyTableCell >& StartingCell, const TSharedRef< class IPropertyTableCell >& EndingCell ) OVERRIDE;

	virtual float GetItemHeight() const OVERRIDE { return ItemHeight; }
	virtual void SetItemHeight( float NewItemHeight ) OVERRIDE { ItemHeight = NewItemHeight; }

	virtual TSharedPtr< class IPropertyTableCell > GetLastClickedCell() const OVERRIDE { return LastClickedCell; }
	virtual void SetLastClickedCell( const TSharedPtr< class IPropertyTableCell >& Cell ) OVERRIDE { LastClickedCell = Cell; }

	virtual TSharedPtr< class IPropertyTableCell > GetCurrentCell() const OVERRIDE { return CurrentCell; }
	virtual void SetCurrentCell( const TSharedPtr< class IPropertyTableCell >& Cell ) OVERRIDE;

	virtual TSharedPtr< class IPropertyTableColumn > GetCurrentColumn() const OVERRIDE { return CurrentColumn; }
	virtual void SetCurrentColumn( const TSharedPtr< class IPropertyTableColumn >& Column ) OVERRIDE;

	virtual TSharedPtr< class IPropertyTableRow > GetCurrentRow() const OVERRIDE { return CurrentRow; }
	virtual void SetCurrentRow( const TSharedPtr< class IPropertyTableRow >& Row ) OVERRIDE;

	virtual TSharedPtr< class IPropertyTableCell > GetFirstCellInSelection() OVERRIDE;
	virtual TSharedPtr< class IPropertyTableCell > GetLastCellInSelection() OVERRIDE;

	virtual TSharedPtr< class IPropertyTableCell > GetNextCellInRow( const TSharedRef< class IPropertyTableCell >& Cell ) OVERRIDE;
	virtual TSharedPtr< class IPropertyTableCell > GetPreviousCellInRow( const TSharedRef< class IPropertyTableCell >& Cell ) OVERRIDE;

	virtual TSharedPtr< class IPropertyTableCell > GetNextCellInColumn( const TSharedRef< class IPropertyTableCell >& Cell ) OVERRIDE;
	virtual TSharedPtr< class IPropertyTableCell > GetPreviousCellInColumn( const TSharedRef< class IPropertyTableCell >& Cell ) OVERRIDE;

	virtual TSharedPtr< class IPropertyTableCell > GetFirstCellInRow( const TSharedRef< class IPropertyTableRow >& Row ) OVERRIDE;
	virtual TSharedPtr< class IPropertyTableCell > GetLastCellInRow( const TSharedRef< class IPropertyTableRow >& Row ) OVERRIDE;

	virtual TSharedPtr< class IPropertyTableCell > GetFirstCellInColumn( const TSharedRef< class IPropertyTableColumn >& Column ) OVERRIDE;
	virtual TSharedPtr< class IPropertyTableCell > GetLastCellInColumn( const TSharedRef< class IPropertyTableColumn >& Column ) OVERRIDE;

	virtual TSharedPtr< class IPropertyTableCell > GetFirstCellInTable() OVERRIDE;
	virtual TSharedPtr< class IPropertyTableCell > GetLastCellInTable() OVERRIDE;

	virtual EPropertyTableSelectionUnit::Type GetSelectionUnit() const OVERRIDE { return SelectionUnit; }
	virtual void SetSelectionUnit( const EPropertyTableSelectionUnit::Type Unit ) OVERRIDE { SelectionUnit = Unit; }

	virtual ESelectionMode::Type GetSelectionMode() const OVERRIDE { return SelectionMode; }
	virtual void SetSelectionMode( const ESelectionMode::Type Mode ) OVERRIDE;

	virtual EColumnSortMode::Type GetColumnSortMode( const TSharedRef< class IPropertyTableColumn > Column ) const OVERRIDE;
	virtual void SortByColumnWithId( const FName& ColumnId, EColumnSortMode::Type SortMode ) OVERRIDE;
	virtual void SortByColumn( const TSharedRef< class IPropertyTableColumn >& Column, EColumnSortMode::Type SortMode ) OVERRIDE;

	virtual void PasteTextAtCell( const FString& Text, const TSharedRef< class IPropertyTableCell >& Cell ) OVERRIDE;

	DECLARE_DERIVED_EVENT( FPropertyTable, IPropertyTable::FSelectionChanged, FSelectionChanged );
	FSelectionChanged* OnSelectionChanged() OVERRIDE { return &SelectionChanged; }

	DECLARE_DERIVED_EVENT( FPropertyTable, IPropertyTable::FColumnsChanged, FColumnsChanged );
	FColumnsChanged* OnColumnsChanged() OVERRIDE { return &ColumnsChanged; }

	DECLARE_DERIVED_EVENT( FPropertyTable, IPropertyTable::FRowsChanged, FRowsChanged );
	FRowsChanged* OnRowsChanged() OVERRIDE { return &RowsChanged; }

	DECLARE_DERIVED_EVENT( FPropertyTable, IPropertyTable::FRootPathChanged, FRootPathChanged );
	virtual FRootPathChanged* OnRootPathChanged() OVERRIDE { return &RootPathChanged; }

	virtual bool IsPropertyEditingEnabled() const OVERRIDE { return true; }
private:

	void UpdateColumns();
	void UpdateRows();

	bool CanSelectCells() const;
	bool CanSelectRows() const;

	TSharedPtr< IPropertyTableColumn > ScanForColumnWithSelectableCells( const int32 StartIndex, const int32 Step ) const;
	TSharedPtr< IPropertyTableRow > ScanForRowWithCells( const int32 StartIndex, const int32 Step ) const;

	TSharedRef< IPropertyTableRow > CreateRow( const TWeakObjectPtr< UObject >& Object );
	TSharedRef< IPropertyTableRow > CreateRow( const TWeakObjectPtr< UProperty >& Property );
	TSharedRef< IPropertyTableRow > CreateRow( const TSharedRef< FPropertyPath >& PropertyPath );

	TSharedRef< IPropertyTableColumn > CreateColumn( const TWeakObjectPtr< UObject >& Object );
	TSharedRef< IPropertyTableColumn > CreateColumn( const TWeakObjectPtr< UProperty >& Property );
	TSharedRef< IPropertyTableColumn > CreateColumn( const TSharedRef< FPropertyPath >& PropertyPath );

	void PurgeInvalidObjectNodes();


private:

	TArray< TWeakObjectPtr< UObject > > SourceObjects;
	TMap< TWeakObjectPtr< UObject >, TSharedRef< FObjectPropertyNode > > SourceObjectPropertyNodes;
	
	TArray< TSharedRef< class IPropertyTableColumn > > Columns;
	TArray< TSharedRef< class IPropertyTableRow > > Rows;

	TSet< TSharedRef< class IPropertyTableColumn > > SelectedColumns;
	TSet< TSharedRef< class IPropertyTableRow > > SelectedRows;
	TSet< TSharedRef< class IPropertyTableCell > > SelectedCells;
	TSharedPtr< class IPropertyTableCell > StartingCellSelectionRange;
	TSharedPtr< class IPropertyTableCell > EndingCellSelectionRange;

	TSharedPtr< class IPropertyTableRow > CurrentRow;
	TSharedPtr< class IPropertyTableCell > CurrentCell;
	TSharedPtr< class IPropertyTableColumn > CurrentColumn;

	TSharedRef< FPropertyPath > RootPath;

	EPropertyTableSelectionUnit::Type SelectionUnit;
	ESelectionMode::Type SelectionMode;

	bool ShowRowHeader;
	bool ShowObjectName;

	float ItemHeight;
	TSharedPtr< class IPropertyTableCell > LastClickedCell;

	/** Actions that should be executed next tick */
	TArray<FSimpleDelegate> DeferredActions;

	FSelectionChanged SelectionChanged;
	FColumnsChanged ColumnsChanged;
	FRowsChanged RowsChanged;
	FRootPathChanged RootPathChanged;

	TWeakPtr< class IPropertyTableColumn > SortedByColumn;
	EColumnSortMode::Type SortedColumnMode;
	bool AllowUserToChangeRoot;

	/** Refresh the table contents? */
	bool bRefreshRequested;

	/** The Orientation of this table, I.e. do we swap columns and rows */
	EPropertyTableOrientation::Type Orientation;
};

