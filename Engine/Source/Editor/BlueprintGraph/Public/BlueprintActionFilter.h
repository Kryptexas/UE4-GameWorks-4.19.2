// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

// Forward declarations
class UBlueprintNodeSpawner;
class UBlueprint;

/*******************************************************************************
 * FBlueprintActionContext
 ******************************************************************************/

struct FBlueprintActionContext
{
	/** 
	 * A list of all blueprints you want actions for. Generally, this will
	 * only contain a single blueprint, but it can have many (where an action
	 * has to be available for every blueprint listed to pass the filter).
	 */
	TArray<UBlueprint*> Blueprints;

	/** 
	 * A list of graphs you want compatible actions for. Generally, this will
	 * contain a single graph, but it can have several (where an action has to 
	 * be viable for every graph to pass the filter).
	 */
	TArray<UEdGraph*> Graphs;

	/** 
	 * A list of pins you want compatible actions for. Generally, this will
	 * contain a single pin, but it can have several (where an action has to 
	 * be viable for every pin to pass the filter).
	 */
	TArray<UEdGraphPin*> Pins;
};


/*******************************************************************************
 * FBlueprintActionFilter
 ******************************************************************************/

class BLUEPRINTGRAPH_API FBlueprintActionFilter
{
public:
	/** The filter uses a series of rejection tests matching */
	DECLARE_DELEGATE_RetVal_TwoParams(bool, FIsFilteredDelegate, FBlueprintActionFilter const&, UBlueprintNodeSpawner const*);

public:
	enum EFlags // Flags, which configure certain rejection tests.
	{
		/** Deprecated class actions will not be filtered out*/
		BPFILTER_IncludeDeprecated		= 0x00000001,

		/** 
		 * Excludes nodes associated with global field types (functions, 
		 * structs, enums, etc.); NOTE: that static function library members are 
		 * counted as "global". 
		 */
		BPFILTER_ExcludeGlobalFields	= 0x00000002,

		/**
		 * Makes NodeTypes tests more aggressive by excluding nodes sub-classes, 
		 * nodes have to explicitly match the class listed. 
		 */
		BPFILTER_ExcludeChildNodeTypes	= 0x00000004,
	};
	FBlueprintActionFilter(uint32 const Flags = 0x00);
	
	/**
	 * A list of allowed node types. If an action's NodeClass isn't one of
	 * these types, then it is filtered out. Use the "ExcludeChildNodeTypes"
	 * flag to aggressively filter out child classes (must be an explicit match).
	 */
	FBlueprintActionContext Context;	
	
	/**
	 * A list of allowed node types. If an action's NodeClass isn't one of
	 * these types, then it is filtered out. Use the "ExcludeChildNodeTypes"
	 * flag to aggressively filter out child classes (must be an explicit match).
	 */
	TArray< TSubclassOf<UEdGraphNode> > NodeTypes;
	
	/**
	 * A list of node types that should be filtered out. If a node class is 
	 * listed both here and in NodeTypes, then the exclusion wins (and it will 
	 * be filtered out).
	 */
	TArray< TSubclassOf<UEdGraphNode> > ExcludedNodeTypes;
	
	/**
	 * A list of classes that you want members for. If an action does not have 
	 * an associated field, or the associated field doesn't belong to any of 
	 * these classes, then it is filtered out.
	 */
	TArray<UClass*> OwnerClasses;

	/**
	 * Users can extend the filter and add their own rejection tests with this
	 * method. We use rejection "IsFiltered" tests rather than inclusive tests 
	 * because it is more optimal to whittle down the list of actions early.
	 * 
	 * @param  IsFilteredDelegate	The rejection test you wish to add to this filter.
	 */
	void AddIsFilteredTest(FIsFilteredDelegate IsFilteredDelegate);

	/**
	 * Query to check and see if the specified action gets filtered out by this 
	 * (and any and'd/or'd filters). NOT marked const to keep 
	 * FIsFilteredDelegates from recursively calling.
	 * 
	 * @param  BlueprintAction	The node-spawner you wish to test.
	 * @return False if the action passes the filter, otherwise false (the action got filtered out).
	 */
	bool IsFiltered(UBlueprintNodeSpawner const* BlueprintAction);

	/**
	 * Appends another filter to be utilized in IsFiltered() queries, extending  
	 * the query to be: IsFilteredByThis() || Rhs.IsFiltered()
	 *
	 * NOTE: Appending additional filters can hinder performance (as the
	 *       additional filter has to loop over its tests foreach database entry)
	 * 
	 * @param  Rhs	The filter to append to this one.
	 * @return This.
	 */
	FBlueprintActionFilter const& operator|=(FBlueprintActionFilter const& Rhs);

	/**
	 * Appends another filter to be utilized in IsFiltered() queries, extending  
	 * the query to be: IsFilteredByThis() && Rhs.IsFiltered()
	 *
	 * NOTE: Appending additional filters can hinder performance (as the
	 *       additional filter has to loop over its tests foreach database entry)
	 * 
	 * @param  Rhs	The filter to append to this one.
	 * @return This.
	 */
	FBlueprintActionFilter const& operator&=(FBlueprintActionFilter const& Rhs);

private:
	/**
	 * Query to check and see if the specified action gets filtered out by this 
	 * (does not take into consideration any and'd/or'd filters).
	 * 
	 * @param  BlueprintAction	The node-spawner you wish to test.
	 * @return False if the action passes the filter, otherwise false (the action got filtered out).
	 */
	bool IsFilteredByThis(UBlueprintNodeSpawner const* BlueprintAction) const;

	/** Set of rejection tests for this specific filter. */
	TArray<FIsFilteredDelegate> FilterTests;

	/** Filters to be logically and'd in with the IsFilteredByThis() result. */
	TArray<FBlueprintActionFilter> AndFilters;

	/** Alternative filters to be logically or'd in with the IsFilteredByThis() result. */
	TArray<FBlueprintActionFilter> OrFilters;
};