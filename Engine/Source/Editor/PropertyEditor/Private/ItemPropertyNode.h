// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FItemPropertyNode : public FPropertyNode
{
public:

	FItemPropertyNode();
	virtual ~FItemPropertyNode();

	/**
	 * Calculates the memory address for the data associated with this item's property.  This is typically the value of a UProperty or a UObject address.
	 *
	 * @param	StartAddress	the location to use as the starting point for the calculation; typically the address of the object that contains this property.
	 *
	 * @return	a pointer to a UProperty value or UObject.  (For dynamic arrays, you'd cast this value to an FArray*)
	 */
	virtual uint8* GetValueBaseAddress( uint8* Base );
	/**
	 * Calculates the memory address for the data associated with this item's value.  For most properties, identical to GetValueBaseAddress.  For items corresponding
	 * to dynamic array elements, the pointer returned will be the location for that element's data. 
	 *
	 * @return	a pointer to a UProperty value or UObject.  (For dynamic arrays, you'd cast this value to whatever type is the Inner for the dynamic array)
	 */
	virtual uint8* GetValueAddress( uint8* Base );

	/**
	 * Overridden function to get the derived object node
	 */
	virtual FItemPropertyNode* AsItemPropertyNode() OVERRIDE { return this; }

	/** Display name override to use instead of the property name */
	void SetDisplayNameOverride( const FString& InDisplayNameOverride ) OVERRIDE;

	/**
	 * @return The formatted display name for the property in this node                                                              
	 */
	FString GetDisplayName() const OVERRIDE;

	/**
	 * Sets the tooltip override to use instead of the property tooltip
	 */
	virtual void SetToolTipOverride( const FString& InToolTipOverride ) OVERRIDE;

	/**
	 * @return The tooltip for the property in this node                                                              
	 */
	virtual FString GetToolTipText() const OVERRIDE;

protected:
	/**
	 * Overridden function for special setup
	 */
	virtual void InitExpansionFlags() OVERRIDE;

	/**
	 * Overridden function for Creating Child Nodes
	 */
	virtual void InitChildNodes() OVERRIDE;

private:
	/** Display name override to use instead of the property name */
	FString DisplayNameOverride;
	/** Tooltip override to use instead of the property tooltip */
	FString ToolTipOverride;
};
