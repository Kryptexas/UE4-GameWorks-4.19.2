// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
 PropertyTablePropertyNameCell.cpp: Implements the FPropertyTablePropertyNameCell class.
=============================================================================*/

#include "PropertyEditorPrivatePCH.h"
#include "PropertyTablePropertyNameCell.h"
#include "PropertyTablePropertyNameColumn.h"
#include "IPropertyTable.h"
#include "IPropertyTableRow.h"
#include "IPropertyTableColumn.h"
#include "PropertyPath.h"
#include "PropertyEditor.h"
#include "ObjectPropertyNode.h"
#include "ItemPropertyNode.h"
#include "PropertyHandle.h"

FPropertyTablePropertyNameCell::FPropertyTablePropertyNameCell( const TSharedRef< class FPropertyTablePropertyNameColumn >& InColumn, const TSharedRef< class IPropertyTableRow >& InRow )
	: EnteredEditModeEvent()
	, ExitedEditModeEvent()
	, Column( InColumn )
	, Row( InRow )
	, bIsBound( true )
	, bInEditMode( false )
{
	Refresh();
}

void FPropertyTablePropertyNameCell::Refresh()
{
	const TSharedRef< IPropertyTableColumn > ColumnRef = Column.Pin().ToSharedRef();
	const TSharedRef< IPropertyTableRow > RowRef = Row.Pin().ToSharedRef();

	ObjectNode = GetTable()->GetObjectPropertyNode( ColumnRef, RowRef );

	bIsBound = ObjectNode.IsValid();
}

FString FPropertyTablePropertyNameCell::GetValueAsString() const
{
	return Row.Pin()->GetDataSource()->AsPropertyPath()->ToString();
}

FText FPropertyTablePropertyNameCell::GetValueAsText() const
{
	return FText::FromString(GetValueAsString());
}

TSharedRef< class IPropertyTable > FPropertyTablePropertyNameCell::GetTable() const
{
	return Column.Pin()->GetTable();
}

void FPropertyTablePropertyNameCell::EnterEditMode() 
{
}

TWeakObjectPtr< UObject > FPropertyTablePropertyNameCell::GetObject() const
{
	if ( !ObjectNode.IsValid() )
	{
		return NULL;
	}

	return ObjectNode->GetUObject( 0 );
}

void FPropertyTablePropertyNameCell::ExitEditMode()
{
}