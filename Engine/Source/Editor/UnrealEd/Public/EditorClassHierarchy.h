// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#ifndef _EDITOR_CLASS_HIERARCHY_H_
#define _EDITOR_CLASS_HIERARCHY_H_

#pragma once


class UNREALED_API FEditorClassHierarchy
{
public:
	/**
	 * Builds the tree from the class manifest
	 * @return - whether the class hierarchy was successfully built
	 */
	bool Init (void);

	/**
	 * true, if the manifest file loaded classes successfully
	 */
	bool WasLoadedSuccessfully(void) const;

	/**
	 * Gets the direct children of the class
	 * @param InClassIndex - the index of the class in question
	 * @param OutIndexArray - the array to fill in with child indices
	 */
	void GetChildrenOfClass(const int32 InClassIndex, TArray<int32> &OutIndexArray);

	/** 
	 * Adds the node and all children recursively to the list of OutAllClasses 
	 * @param InClassIndex - The node in the hierarchy to start with
	 * @param OutAllClasses - The list of classes generated recursively
	 */
	void GetAllClasses(const int32 InClassIndex, OUT TArray<UClass*>& OutAllClasses);

	/**
	 * Find the class index that matches the requested name
	 * @param InClassName - Name of the class to find
	 * @return - The index of the desired class
	 */
	int32 Find(const FString& InClassName);

	/**
	 * returns the name of the class in the tree
	 * @param InClassIndex - the index of the class in question
	 */
	FString GetClassName(const int32 InClassIndex);
	/**
	 * returns the UClass of the class in the tree
	 * @param InClassIndex - the index of the class in question
	 */
	UClass* GetClass(const int32 InClassIndex);

	/**
	 * Returns the class index of the provided index's parent class
	 *
	 * @param	InClassIndex	Class index to find the parent of
	 *
	 * @return	Class index of the parent class of the provided index, if any; INDEX_NONE if not
	 */
	int32 GetParentIndex(int32 InClassIndex) const;
	
	/** 
	 * Returns a list of class group names for the provided class index 
	 *
	 * @param InClassIndex	The class index to find group names for
	 * @param OutGroups		The list of class groups found.
	 */
	void GetClassGroupNames( int32 InClassIndex, TArray<FString>& OutGroups ) const;

	/**
	 * returns if the class is hidden or not
	 * @param InClassIndex - the index of the class in question
	 */
	bool IsHidden(const int32 InClassIndex) const;
	/**
	 * returns if the class is placeable or not
	 * @param InClassIndex - the index of the class in question
	 */
	bool IsPlaceable(const int32 InClassIndex) const;
	/**
	 * returns if the class is abstract or not
	 * @param InClassIndex - the index of the class in question
	 */
	bool IsAbstract(const int32 InClassIndex) const;
	/**
	 * returns if the class is a brush or not
	 * @param InClassIndex - the index of the class in question
	 */
	bool IsBrush(const int32 InClassIndex);
	/**
	 * Returns if the class is visible 
	 * @param InClassIndex - the index of the class in question
	 * @param bInPlaceable - if true, return number of placeable children, otherwise returns all children
	 * @return - Number of children
	 */
	bool IsClassVisible(const int32 InClassIndex, const bool bInPlaceableOnly);

	/**
	 * Returns if the class has any children (placeable or all)
	 * @param InClassIndex - the index of the class in question
	 * @param bInPlaceable - if true, return if has placeable children, otherwise returns if has any children
	 * @return - Whether this node has children (recursively) that are visible
	 */
	bool HasChildren(const int32 InClassIndex, const bool bInPlaceableOnly);

};

#endif
