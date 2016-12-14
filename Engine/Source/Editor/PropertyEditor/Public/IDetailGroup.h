// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PropertyHandle.h"

class FDetailWidgetRow;
class IDetailPropertyRow;

/**
 * A group in the details panel that can have children                                                              
 */
class IDetailGroup
{
public:
	virtual ~IDetailGroup(){}

	/**
	 * Makes a custom row for the groups header
	 *
	 * @return a new row for customizing the header
	 */
	virtual FDetailWidgetRow& HeaderRow() = 0;

	/**
	 * Adds a property as the groups header      
	 */
	virtual IDetailPropertyRow& HeaderProperty( TSharedRef<IPropertyHandle> PropertyHandle ) = 0;

	/** 
	 * Adds a new row for custom widgets *
	 * 
	 * @return a new row for adding widgets
	 */
	virtual class FDetailWidgetRow& AddWidgetRow() = 0;

	/**
	 * Adds a new row for a property
	 *
	 * @return an interface for customizing the appearance of the property row
	 */
	virtual class IDetailPropertyRow& AddPropertyRow( TSharedRef<IPropertyHandle> PropertyHandle ) = 0;

	/**
	 * Adds a group to the category
	 *
	 * @param NewGroupName	The name of the group
	 * @param LocalizedDisplayName	The display name of the group
	 * @param true if the group should start expanded
	 */
	virtual IDetailGroup& AddGroup(FName NewGroupName, const FText& InLocalizedDisplayName, bool bInStartExpanded = false) = 0;

	/**
	 * Toggles expansion on the group
	 *
	 * @param bExpand	true to expand the group, false to collapse
	 */
	virtual void ToggleExpansion( bool bExpand ) = 0;

	/**
	 * Gets the current state of expansion for the group
	 */
	virtual bool GetExpansionState() const = 0;
};
