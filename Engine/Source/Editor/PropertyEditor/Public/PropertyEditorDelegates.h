// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/** Delegate called to see if a property should be drawn */
DECLARE_DELEGATE_RetVal_OneParam( bool, FIsPropertyVisible, const UProperty* const );

/** Delegate called to get a detail layout for a specific object class */
DECLARE_DELEGATE_RetVal( TSharedRef<class IDetailCustomization>, FOnGetDetailCustomizationInstance );

/** Delegate called to get a struct property layout for a specific struct class */
DECLARE_DELEGATE_RetVal( TSharedRef<class IStructCustomization>, FOnGetStructCustomizationInstance );

/** Notification for when a property view changes */
DECLARE_DELEGATE_TwoParams( FOnObjectArrayChanged, const FString&, const TArray< TWeakObjectPtr< UObject > >& );

/** Notification for when a property selection changes. */
DECLARE_DELEGATE_OneParam( FOnPropertySelectionChanged, UProperty* )

/** Notification for when a property is double clicked by the user*/
DECLARE_DELEGATE_OneParam( FOnPropertyDoubleClicked, UProperty* )

/** Notification for when a property is clicked by the user*/
DECLARE_DELEGATE_OneParam( FOnPropertyClicked, const TSharedPtr< class FPropertyPath >& )

/** */
DECLARE_DELEGATE_OneParam( FConstructExternalColumnHeaders, const TSharedRef< class SHeaderRow >& )

DECLARE_DELEGATE_RetVal_TwoParams( TSharedRef< class SWidget >, FConstructExternalColumnCell,  const FName& /*ColumnName*/, const TSharedRef< class IPropertyTreeRow >& /*RowWidget*/)

/** Delegate called to see if a property editing is enabled */
DECLARE_DELEGATE_RetVal(bool, FIsPropertyEditingEnabled );

/**
 * A delegate which is called after properties have been edited and PostEditChange has been called on all objects.
 * This can be used to safely make changes to data that the details panel is observing instead of during PostEditChange (which is
 * unsafe)
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnFinishedChangingProperties, const FPropertyChangedEvent&);