// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FCategoryPropertyNode : public FPropertyNode
{
public:
	FCategoryPropertyNode();
	virtual ~FCategoryPropertyNode();

	/**
	 * Overridden function to get the derived object node
	 */
	virtual FCategoryPropertyNode* AsCategoryNode() OVERRIDE { return this; }

	/**
	 * Accessor functions for getting a category name
	 */
	void SetCategoryName(const FName& InCategoryName) { CategoryName = InCategoryName; }
	const FName& GetCategoryName(void) const { return CategoryName; }

	/**
	 * Checks to see if this category is a 'sub-category'
	 *
	 * @return	True if this category node is a sub-category, otherwise false
	 */
	bool IsSubcategory() const;

	virtual FString GetDisplayName() const OVERRIDE;

protected:
	/**
	 * Overridden function for special setup
	 */
	virtual void InitBeforeNodeFlags() OVERRIDE;
	/**
	 * Overridden function for Creating Child Nodes
	 */
	virtual void InitChildNodes() OVERRIDE;

	/**
	 * Appends my path, including an array index (where appropriate)
	 */
	virtual void GetQualifiedName( FString& PathPlusIndex, const bool bWithArrayIndex ) const OVERRIDE;

	FString GetSubcategoryName() const;
	/**
	 * Stored category name for the window
	 */
	FName CategoryName;
};
