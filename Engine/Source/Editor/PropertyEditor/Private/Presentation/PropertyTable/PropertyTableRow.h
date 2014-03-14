// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "IPropertyTableRow.h"

class FPropertyTableRow : public TSharedFromThis< FPropertyTableRow >, public IPropertyTableRow
{
public:
	
	FPropertyTableRow( const TSharedRef< class IPropertyTable >& InTable, const TWeakObjectPtr< UObject >& InObject  );

	FPropertyTableRow( const TSharedRef< class IPropertyTable >& InTable, const TSharedRef< FPropertyPath >& InPropertyPath );

	FPropertyTableRow( const TSharedRef< class IPropertyTable >& InTable, const TWeakObjectPtr< UObject >& InObject, const TSharedRef< FPropertyPath >& InPartialPropertyPath );

	FPropertyTableRow( const TSharedRef< class IPropertyTable >& InTable, const TSharedRef< FPropertyPath >& InPropertyPath, const TSharedRef< FPropertyPath >& InPartialPropertyPath );

	virtual TSharedRef< class IDataSource > GetDataSource() const OVERRIDE { return DataSource; }

	virtual bool HasChildren() const OVERRIDE;

	virtual void GetChildRows( TArray< TSharedRef< class IPropertyTableRow > >& OutChildren ) const OVERRIDE;

	virtual TSharedRef< class IPropertyTable > GetTable() const OVERRIDE;

	virtual bool HasCells() const OVERRIDE { return true; }

	virtual TSharedRef< class FPropertyPath > GetPartialPath() const OVERRIDE { return PartialPath; }

	virtual void Tick() OVERRIDE;

	virtual void Refresh() OVERRIDE;

	DECLARE_DERIVED_EVENT( FPropertyTableRow, IPropertyTableRow::FRefreshed, FRefreshed );
	virtual FRefreshed* OnRefresh() { return &Refreshed; }


private:

	void GenerateChildren();


private:

	TSharedRef< class IDataSource > DataSource;

	TWeakPtr< class IPropertyTable > Table;

	TArray< TSharedRef< class IPropertyTableRow > > Children;

	TSharedRef< class FPropertyPath > PartialPath;

	FRefreshed Refreshed;
};