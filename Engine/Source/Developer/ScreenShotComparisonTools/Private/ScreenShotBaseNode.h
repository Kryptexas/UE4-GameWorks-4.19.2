// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ScreenShotData.h: Declares the FScreenShotBaseNode class.
=============================================================================*/

#pragma once


/**
 * Base class for screenshot nodes.
 */
class FScreenShotBaseNode
	: public TSharedFromThis<FScreenShotBaseNode>
	, public IScreenShotData
{
public:

	/**
	* Construct test data with the given name.
	*
	* @param InName  The Name of this test data item.
	*/
	FScreenShotBaseNode( const FString& InName, const FString& InAssetName = "" )
		: ItemName(InName)
		, AssetName(InAssetName)
	{ }

public:

	// Begin IScreenShotData interface

	virtual void AddScreenShotData( const FScreenShotDataItem& InScreenDataItem ) OVERRIDE;
	virtual const FString& GetAssetName() const OVERRIDE;
	virtual TArray< TSharedPtr< IScreenShotData> >& GetChildren() OVERRIDE;
	virtual TArray< TSharedPtr< IScreenShotData> >& GetFilteredChildren() OVERRIDE;
	virtual const FString& GetName() const OVERRIDE;
	virtual EScreenShotDataType::Type GetScreenNodeType() OVERRIDE;
	virtual bool SetFilter( TSharedPtr< ScreenShotFilterCollection > ScreenFilter ) OVERRIDE;

	// End IScreenShotData interface

protected:

	/**
	 * Add a child to the tree.
	 *
	 * @param ChildName - The name of the node.
	 * @return The new node.
	 */
	IScreenShotDataRef AddChild( const FString& ChildName );

	/**
	 * Create a node to add to the tree.
	 *
	 * @param ChildName - The name of the node.
	 * @return The new node.
	 */

	virtual IScreenShotDataRef CreateNode( const FString& ChildName );

public:

	// The asset name
	FString AssetName;

	// Holds the array of child nodes
	TArray<IScreenShotDataPtr> Children;

	// Holds the array of filtered nodes
	TArray<IScreenShotDataPtr> FilteredChildren;

	// The item name
	FString ItemName;
};
