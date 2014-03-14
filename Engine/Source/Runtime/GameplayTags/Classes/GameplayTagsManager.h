// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTagsManager.generated.h"

/** Simple struct for a table row in the gameplay tag table */
USTRUCT()
struct FGameplayTagTableRow : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

	/** Tag specified in the table */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=GameplayTag)
	FString Tag;

	/** Text that describes this category - not all tags have categories */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=GameplayTag)
	FText CategoryText;

	/** Constructors */
	FGameplayTagTableRow() { CategoryText = NSLOCTEXT("TAGCATEGORY", "TAGCATEGORYTESTING", "Category and Category"); }
	FGameplayTagTableRow(FGameplayTagTableRow const& Other);

	/** Assignment/Equality operators */
	FGameplayTagTableRow& operator=(FGameplayTagTableRow const& Other);
	bool operator==(FGameplayTagTableRow const& Other) const;
	bool operator!=(FGameplayTagTableRow const& Other) const;
};

/** Simple tree node for gameplay tags */
struct FGameplayTagNode
{
	/** Simple constructor */
	FGameplayTagNode(FName InTag, TWeakPtr<FGameplayTagNode> InParentNode, FText InCategoryDescription = FText());

	/**
	 * Get the complete tag for the node, including all parent tags, delimited by periods
	 * 
	 * @return Complete tag for the node
	 */
	GAMEPLAYTAGS_API FName GetCompleteTag() const;

	/**
	 * Get the simple tag for the node (doesn't include any parent tags)
	 * 
	 * @return Simple tag for the node
	 */
	GAMEPLAYTAGS_API FName GetSimpleTag() const;

	/**
	 * Get the category description for the node
	 * 
	 * @return Translatable text that describes this tag category
	 */
	GAMEPLAYTAGS_API FText GetCategoryDescription() const;

	/**
	 * Get the children nodes of this node
	 * 
	 * @return Reference to the array of the children nodes of this node
	 */
	GAMEPLAYTAGS_API TArray< TSharedPtr<FGameplayTagNode> >& GetChildTagNodes();

	/**
	 * Get the children nodes of this node
	 * 
	 * @return Reference to the array of the children nodes of this node
	 */
	GAMEPLAYTAGS_API const TArray< TSharedPtr<FGameplayTagNode> >& GetChildTagNodes() const;

	/**
	 * Get the parent tag node of this node
	 * 
	 * @return The parent tag node of this node
	 */
	GAMEPLAYTAGS_API TWeakPtr<FGameplayTagNode> GetParentTagNode() const;

	/** Reset the node of all of its values */
	GAMEPLAYTAGS_API void ResetNode();

private:
	/** Tag for the node */
	FName Tag;

	/** Complete tag for the node, including parent tags */
	FName CompleteTag;

	/** Category description of the for the node */
	FText CategoryDescription;

	/** Child gameplay tag nodes */
	TArray< TSharedPtr<FGameplayTagNode> > ChildTags;

	/** Owner gameplay tag node, if any */
	TWeakPtr<FGameplayTagNode> ParentNode;
};

/** Holds global data loaded at startup, is in a singleton UObject so it works properly with hot reload */
UCLASS(config=Game)
class GAMEPLAYTAGS_API UGameplayTagsManager : public UObject
{
	GENERATED_UCLASS_BODY()

	// Destructor
	~UGameplayTagsManager();

	/**
	 * Helper function to insert a tag into a tag node array
	 * 
	 * @param Tag					Tag to insert
	 * @param ParentNode			Parent node, if any, for the tag
	 * @param NodeArray				Node array to insert the new node into, if necessary (if the tag already exists, no insertion will occur)
	 * @param CategoryDescription	The description of this category
	 * 
	 * @return Index of the node of the tag
	 */
	int32 InsertTagIntoNodeArray(FName Tag, TWeakPtr<FGameplayTagNode> ParentNode, TArray< TSharedPtr<FGameplayTagNode> >& NodeArray, FText CategoryDescription = FText());

	/**
	 * Get the best tag category description
	 *
	 * @param Tag				Tag that we will find all nodes for
	 * @param OutDescription	The Description that will be returned
	 *
	 * @return The index that we got the description from
	 */
	int32 GetBestTagCategoryDescription(FString Tag, FText& OutDescription);

	/** Gets all nodes that make up a tag, if any (eg weapons.ranged.pistol will return the nodes weapons, weapons.ranged and weapons.ranged.pistol)
	 * @param Tag The . delimited tag we wish to get nodes for
	 * @param OutTagArray The array of tag nodes that were found
	 */
	void GetAllNodesForTag( const FString& Tag, TArray< TSharedPtr<FGameplayTagNode> >& OutTagArray );

	// this is here because the tag tree doesn't start with a node and recursion can't be done until the first node is found
	void GetAllNodesForTag_Recurse(TArray<FString>& Tags, int32 CurrentTagDepth, TSharedPtr<FGameplayTagNode> CurrentTagNode, TArray< TSharedPtr<FGameplayTagNode> >& OutTagArray );

#if WITH_EDITOR
	/** Gets a Filtered copy of the GameplayRootTags Array based on the comma delimited filter string passed in*/
	void GetFilteredGameplayRootTags( const FString& InFilterString, TArray< TSharedPtr<FGameplayTagNode> >& OutTagArray );

	/** 
	 * Called via delegate when an object is re-imported in the editor
	 * 
	 * @param InObject	Object that was re-imported
	 */
	void OnObjectReimported(UObject* InObject);

#endif //WITH_EDITOR

	/** 
	 * Loads the tag table
	 * 
	 * @param TagTableName	The name of the table to load
	 *
	 * @return The data table that has been loaded
	 */
	const UDataTable* LoadGameplayTagTable( FString TagTableName );

	/** Helper function to construct the gameplay tag tree */
	void ConstructGameplayTagTree();

	/** Helper function to destroy the gameplay tag tree */
	void DestroyGameplayTagTree();

	/** Holds all of the valid gameplay-related tags that can be applied to assets */
	UPROPERTY()
	class UDataTable* GameplayTagTable;

	/** Roots of gameplay tag nodes */
	TArray< TSharedPtr<FGameplayTagNode> > GameplayRootTags;

};
