// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FClassViewerNode
{
public:
	/**
	 * Creates a node for the widget's tree.
	 *
	 * @param	_ClassName						The name of the class this node represents.
	 * @param	_bIsPlaceable					true if the class is a placeable class.
	 */
	FClassViewerNode( FString Item, bool bIsPlaceable);

	FClassViewerNode( const FClassViewerNode& InCopyObject);

	/**
	 * Adds the specified child to the node.
	 *
	 * @param	Child							The child to be added to this node for the tree.
	 */
	void AddChild( TSharedPtr<FClassViewerNode> Child );

	/**
	 * Adds the specified child to the node. If a child with the same class already exists the function add the child storing more info.
	 * The function does not persist child order.
	 *
	 * @param	NewChild	The child to be added to this node for the tree.
	 */
	void AddUniqueChild(TSharedPtr<FClassViewerNode> NewChild);

	/** Retrieves the class name this node is associated with. */
	TSharedPtr<FString> GetClassName() const
	{
		return ClassName;	
	}

	/** Retrieves the children list. */
	TArray<TSharedPtr<FClassViewerNode>>& GetChildrenList()
	{
		return ChildrenList;
	}

	/** Checks if the class is placeable. */
	bool IsClassPlaceable() const
	{
		return bIsClassPlaceable;
	}

	bool IsRestricted() const;

private:
	/** The class name for this tree node. */
	TSharedPtr<FString> ClassName;

	/** List of children. */
	TArray<TSharedPtr<FClassViewerNode>> ChildrenList;

	/** true if the class is placeable. */
	bool bIsClassPlaceable;

public:
	/** The class this node is associated with. */
	TWeakObjectPtr<UClass> Class;

	/** The blueprint this node is associated with. */
	TWeakObjectPtr<UBlueprint> Blueprint;

	/** Used to load up the package if it is unloaded, retrieved from meta data for the package. */
	FString GeneratedClassPackage;

	/** Used to examine the classname, retrieved from meta data for the package. */
	FString GeneratedClassname;

	/** Used to find the parent of this class, retrieved from meta data for the package. */
	FString ParentClassname;

	/** Used to load up the class if it is unloaded. */
	FString AssetName;

	/** true if the class passed the filter. */
	bool bPassesFilter;

	/** true if the class is a "normal type", this is used to identify unloaded blueprints as blueprint bases. */
	bool bIsBPNormalType;

	/** Pointer to the parent to this object. */
	TWeakPtr< FClassViewerNode > ParentNode;

	/** Data for unloaded blueprints, only valid if the class is unloaded. */
	TSharedPtr< class IUnloadedBlueprintData > UnloadedBlueprintData;

	/** The property this node will be working on. */
	TSharedPtr<class IPropertyHandle> PropertyHandle;
};