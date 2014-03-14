// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "IPropertyTableUtilities.h"
#include "IPropertyTableColumn.h"
#include "IPropertyTable.h"
#include "IPropertyTableCell.h"
#include "IPropertyTableRow.h"
#include "PropertyTableObjectNameCell.h"
#include "DataSource.h"
#include "PropertyPath.h"

#define LOCTEXT_NAMESPACE "ObjectNameColumnHeader"

class FPropertyTableObjectNameColumn : public TSharedFromThis< FPropertyTableObjectNameColumn >, public IPropertyTableColumn
{
public:

	FPropertyTableObjectNameColumn( const TSharedRef< IPropertyTable >& InTable )
		: Table( InTable )
		, Cells()
		, bIsHidden( false )
		, Width( 2.0f )
	{

	}

	//  Begin IPropertyTable Interface

	virtual FName GetId() const OVERRIDE { return FName( TEXT("ObjectName") ); }

	virtual FText GetDisplayName() const OVERRIDE { return LOCTEXT( "DisplayName", "Name" ); }

	virtual TSharedRef< IDataSource > GetDataSource() const OVERRIDE { return MakeShareable( new NoDataSource() ); }

	virtual TSharedRef< class FPropertyPath > GetPartialPath() const OVERRIDE { return FPropertyPath::CreateEmpty(); }

	virtual TSharedRef< class IPropertyTableCell > GetCell( const TSharedRef< class IPropertyTableRow >& Row ) OVERRIDE
	{
		//@todo Clean Cells cache when rows get updated [11/27/2012 Justin.Sargent]
		TSharedRef< IPropertyTableCell >* CellPtr = Cells.Find( Row );

		if( CellPtr != NULL )
		{
			return *CellPtr;
		}

		TSharedRef< IPropertyTableCell > Cell = MakeShareable( new FPropertyTableObjectNameCell( SharedThis( this ), Row ) );
		Cells.Add( Row, Cell );

		return Cell;
	}

	virtual void RemoveCellsForRow( const TSharedRef< class IPropertyTableRow >& Row ) OVERRIDE
	{
		Cells.Remove( Row );
	}

	virtual TSharedRef< class IPropertyTable > GetTable() const OVERRIDE { return Table.Pin().ToSharedRef(); }

	virtual bool CanSelectCells() const OVERRIDE { return true; }

	virtual EPropertyTableColumnSizeMode::Type GetSizeMode() const OVERRIDE { return EPropertyTableColumnSizeMode::Fill; }

	virtual float GetWidth() const OVERRIDE { return Width; } 

	virtual void SetWidth( float InWidth ) OVERRIDE { Width = InWidth; }

	virtual bool IsHidden() const OVERRIDE { return bIsHidden; }

	virtual void SetHidden( bool InIsHidden ) OVERRIDE { bIsHidden = InIsHidden; }

	virtual bool IsFrozen() const OVERRIDE { return true; }

	virtual void SetFrozen( bool InIsFrozen ) OVERRIDE {}

	virtual bool CanSortBy() const OVERRIDE { return true; }

	virtual void Sort( TArray< TSharedRef< class IPropertyTableRow > >& Rows, const EColumnSortMode::Type SortMode ) OVERRIDE
	{
		struct FCompareRowByObjectNameAscending
		{
		public:
			FCompareRowByObjectNameAscending( const TSharedRef< FPropertyTableObjectNameColumn >& Column )
				: NameColumn( Column )
			{ }

			FORCEINLINE bool operator()( const TSharedRef< IPropertyTableRow >& Lhs, const TSharedRef< IPropertyTableRow >& Rhs ) const
			{
				return NameColumn->GetObjectNameAsString( Lhs ) < NameColumn->GetObjectNameAsString( Rhs );
			}

			TSharedRef< FPropertyTableObjectNameColumn > NameColumn;
		};

		struct FCompareRowByObjectNameDescending
		{
		public:
			FCompareRowByObjectNameDescending( const TSharedRef< FPropertyTableObjectNameColumn >& Column )
				: Comparer( Column )
			{ }

			FORCEINLINE bool operator()( const TSharedRef< IPropertyTableRow >& Lhs, const TSharedRef< IPropertyTableRow >& Rhs ) const
			{
				return !Comparer( Lhs, Rhs ); 
			}

		private:

			const FCompareRowByObjectNameAscending Comparer;
		};

		if ( SortMode == EColumnSortMode::None )
		{
			return;
		}

		if ( SortMode == EColumnSortMode::Ascending )
		{
			Rows.Sort( FCompareRowByObjectNameAscending( SharedThis( this ) ) );
		}
		else
		{
			Rows.Sort( FCompareRowByObjectNameDescending( SharedThis( this ) ) );
		}
	}

	virtual void Tick() OVERRIDE {}

	// End IPropertyTable Interface

public:

	FString GetObjectNameAsString( const TSharedRef< IPropertyTableRow >& Row )
	{
		FString ObjectName;
		FString Suffix = GetTable()->GetRootPath()->ExtendPath( Row->GetPartialPath() )->ToString();
		if ( !Suffix.IsEmpty() )
		{
			Suffix = FString( TEXT("->") ) + Suffix;
		}

		const UObject* const Object = GetTable()->GetObjectPropertyNode( SharedThis( this ), Row )->GetUObject( 0 );
		if ( Object != NULL )
		{
			UClass* ObjectClass = Object->GetClass();
			const UBlueprintGeneratedClass* const BlueprintGeneratedClass = Cast<UBlueprintGeneratedClass>( ObjectClass ); 
			const bool IsCDO = Object->HasAnyFlags(RF_ClassDefaultObject);
			if ( BlueprintGeneratedClass != NULL && IsCDO )
			{
				ObjectName = BlueprintGeneratedClass->ClassGeneratedBy->GetFName().ToString();
			}
			else if ( Object->IsA( AActor::StaticClass() ) )
			{
				ObjectName = Cast<const  AActor >( Object )->GetActorLabel();
			}
			else
			{
				ObjectName = Object->GetFName().ToString();
			}
		}

		return ObjectName + Suffix;
	}


private:

	TWeakPtr< IPropertyTable > Table;
	TMap< TSharedRef< IPropertyTableRow >, TSharedRef< class IPropertyTableCell > > Cells;
	float Width;
	bool bIsHidden;
};

#undef LOCTEXT_NAMESPACE