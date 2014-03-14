// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IStatsPage.h"
#include "Editor/PropertyEditor/Public/IPropertyTable.h"


/** 
 * Template for all stats pages/factories. 
 * These classes generate uniform arrays of identically-typed objects that
 * are displayed in a PropertyTable.
 * Boilerplate implementations are below that all pages in this module currently more-or-less share
 */
template <typename Entry>
class FStatsPage : public IStatsPage
{
public:
	FStatsPage()
	{
		FString EnumName = Entry::StaticClass()->GetName();
		EnumName += TEXT(".");
		EnumName += Entry::StaticClass()->GetMetaData( TEXT("ObjectSetType") );
		ObjectSetEnum = FindObject<UEnum>( NULL, *EnumName );
		bRefresh = false;
		bShow = false;
		ObjectSetIndex = 0;
	}

	/** Begin IStatsPage interface */
	virtual void Show( bool bInShow = true ) OVERRIDE
	{
		bShow = bInShow;
	}

	virtual bool IsShowPending() const OVERRIDE
	{
		return bShow;
	}

	virtual void Refresh( bool bInRefresh = true ) OVERRIDE
	{
		bRefresh = bInRefresh;
	}

	virtual bool IsRefreshPending() const OVERRIDE
	{
		return bRefresh;
	}

	virtual FName GetName() const OVERRIDE
	{
		return Entry::StaticClass()->GetFName();
	}

	virtual const FText GetDisplayName() const OVERRIDE
	{
		return Entry::StaticClass()->GetDisplayNameText();
	}

	virtual const FText GetToolTip() const OVERRIDE
	{
		return Entry::StaticClass()->GetToolTipText();
	}

	virtual int32 GetObjectSetCount() const OVERRIDE
	{
		if(ObjectSetEnum != NULL)
		{
			return ObjectSetEnum->NumEnums() - 1;
		}
		return 1;
	}

	virtual FString GetObjectSetName( int32 InObjectSetIndex ) const OVERRIDE
	{
		if(ObjectSetEnum != NULL)
		{
			return ObjectSetEnum->GetDisplayNameText( InObjectSetIndex ).ToString();
		}

		// if no valid object set, return a static empty string
		static FString EmptyString;
		return EmptyString;
	}

	virtual FString GetObjectSetToolTip( int32 InObjectSetIndex ) const OVERRIDE
	{
		if(ObjectSetEnum != NULL)
		{
			return ObjectSetEnum->GetToolTipText( InObjectSetIndex ).ToString();
		}

		// if no valid object set, return a static empty string
		static FString EmptyString;
		return EmptyString;
	}

	virtual UClass* GetEntryClass() const OVERRIDE
	{
		return Entry::StaticClass();
	}

	virtual TSharedPtr<SWidget> GetCustomWidget( TWeakPtr< class IStatsViewer > InParentStatsViewer ) OVERRIDE
	{
		return NULL;
	}

	virtual void SetSelectedObjectSet( int32 InObjectSetIndex ) OVERRIDE
	{
		ObjectSetIndex = InObjectSetIndex;
	}

	virtual int32 GetSelectedObjectSet() const OVERRIDE
	{
		return ObjectSetIndex;
	}

	virtual void GetCustomColumns(TArray< TSharedRef< class IPropertyTableCustomColumn > >& OutCustomColumns) const OVERRIDE
	{
	}
	/** End IStatsPage interface */

protected:

	/** The enum we use for our object set */
	UEnum* ObjectSetEnum;
	
	/** Selected object set index */
	int32 ObjectSetIndex;

	/** Flag to refresh the page */
	bool bRefresh;

	/** Flag to show the page */
	bool bShow;	
};


