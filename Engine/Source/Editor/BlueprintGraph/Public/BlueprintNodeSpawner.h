// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlueprintNodeSpawner.generated.h"

// Forward declarations
class UEdGraph;
class UEdGraphNode;

/*******************************************************************************
* UBlueprintNodeSpawner Declaration
******************************************************************************/

/**
 * Intended to be wrapped and used by FBlueprintActionMenuItem. Rather than 
 * sub-classing the menu item, we choose to subclass this instead (for 
 * different node types). That way, we get the type inference that comes with  
 * UObjects (and we don't have to continuously compare identification strings). 
 *
 * Additionally, this decouples the "action" from ui settings (like the tooltip,
 * menu category, etc.). It gives us more leeway in how we choose to present 
 * these "actions".
 */
UCLASS(Transient)
class BLUEPRINTGRAPH_API UBlueprintNodeSpawner : public UObject
{
	GENERATED_UCLASS_BODY()
	DECLARE_DELEGATE_TwoParams(FCustomizeNodeDelegate, UEdGraphNode*, bool);

public:
	/**
	 * Creates a new UBlueprintNodeSpawner for the specified node class. Sets
	 * the allocated spawner's NodeClass and CustomizeNodeDelegate fields from
	 * the supplied parameters.
	 *
	 * @param  NodeClass			The node type that you want the spawner to spawn.
	 * @param  Outer				Optional outer for the new spawner (if left null, the transient package will be used).
	 * @param  PostSpawnDelegate    A delegate to perform specialized node setup post-spawn.
	 * @return A newly allocated instance of this class.
	 */
	static UBlueprintNodeSpawner* Create(TSubclassOf<UEdGraphNode> const NodeClass, UObject* Outer = nullptr, FCustomizeNodeDelegate PostSpawnDelegate = FCustomizeNodeDelegate());
	
	/**
	 * Templatized version of the above Create() method (where we specify the 
	 * spawner's node class through the template argument).
	 *
	 * @param  Outer				Optional outer for the new spawner (if left null, the transient package will be used).
	 * @param  PostSpawnDelegate    A delegate to perform specialized node setup post-spawn.
	 * @return A newly allocated instance of this class.
	 */
	template<class NodeType>
	static UBlueprintNodeSpawner* Create(UObject* Outer = nullptr, FCustomizeNodeDelegate PostSpawnDelegate = FCustomizeNodeDelegate());
	
	/**
	 * Holds the class of node to spawn. May be null for sub-classes that know
	 * specifically what node types to spawn (if null for an instance of this 
	 * class, then nothing will be spawned).
	 */
	UPROPERTY()
	TSubclassOf<UEdGraphNode> NodeClass;
	
	/**
	 * A delegate to perform specialized node setup post-spawn (so we don't have
	 * to sub-class this for every node type, and so we don't have to spawn a
	 * template node for each to hold on to).
	 */
	FCustomizeNodeDelegate CustomizeNodeDelegate;

	/**
	 * Takes care of spawning a node for the specified graph. Looks to see if 
	 * the supplied graph is transient, and if so, spawns a NOT fully formed
	 * node (intended for template use).
	 * 
	 * This function is intended to be overridden; sub-classes may return
	 * a pre-existing node, instead of a newly allocated one (for cases where
	 * only one instance of the node type can exist). Callers should check for 
	 * this case upon use.
	 * 
	 * @param  ParentGraph	The graph you want the node spawned into.
	 * @param  Location     Where you want the new node positioned in the graph.
	 * @return Null if it failed to spawn a node, otherwise a newly spawned node or possibly one that already existed.
	 */
	virtual UEdGraphNode* Invoke(UEdGraph* ParentGraph, FVector2D const Location) const;
	
	// @TODO: for favorites, generated for the spawner type + data (function, property, node, etc.)
	//virtual FGuid GetActionHash() const;
	
	/**
	 * To save on performance, certain sub-classes can concoct menu names from
	 * static data (like function names, etc.). This unfortunately doesn't 
	 * completely decouple these spawners from the ui we sought to separate 
	 * from. However, this can be thought of as a suggestion that the ui 
	 * building code is free to do with as it pleases.
	 *
	 * @return An empty text string (could be a localized name for sub-classes).
	 */
	virtual FText GetDefaultMenuName() const;
	
	/**
	 * To save on performance, certain sub-classes can concoct menu categories
	 * from static data (like function metadata, etc.). This unfortunately
	 * doesn't completely decouple these spawners from the ui we sought to
	 * separate from. However, this can be thought of as a suggestion that the
	 * ui building code is free to do with as it pleases (like concatenate with 
	 * parent categories, etc.).
	 *
	 * @return An empty text string (could be a localized category for sub-classes).
	 */
	virtual FText GetDefaultMenuCategory() const;
	
	/**
	 * To save on performance, certain sub-classes can construct tooltips from
	 * static data (like function metadata, etc.). This unfortunately doesn't
	 * completely decouple these spawners from the ui we sought to separate
	 * them from. However, this can be thought of as a suggestion that the ui
	 * building code is free to do with as it pleases.
	 *
	 * @return An empty text string (could be a localized tooltip for sub-classes).
	 */
	virtual FText GetDefaultMenuTooltip() const;
	
	/**
	 * To save on performance, certain sub-classes can pull search keywords from
	 * static data (like function metadata, etc.). This unfortunately doesn't
	 * completely decouple these spawners from the ui we sought to separate
	 * them from. Although, this can be thought of as a suggestion that the ui
	 * building code is free to do with as it pleases (doesn't have to use it).
	 *
	 * @return An empty text string (could be a localized keywords for sub-classes).
	 */
	virtual FText GetDefaultSearchKeywords() const;

	/**
	 * To save on performance, certain sub-classes can pull icons info from
	 * static data (like function metadata, etc.). This unfortunately doesn't
	 * completely decouple these spawners from the ui we sought to separate
	 * them from. Although, this can be thought of as a suggestion that the ui
	 * building code is free to do with as it pleases (doesn't have to use it).
	 *
	 * @param  ColorOut		The color to tint the icon with.
	 * @return Name of the brush to use (use FEditorStyle::GetBrush() to resolve).
	 */
	virtual FName GetDefaultMenuIcon(FLinearColor& ColorOut);

	/**
	 * Expects the supplied outer to be transient. Will spawn a new template-
	 * node if the cached one is null, or if its graph outer doesn't match the
	 * one specified here. Once a new node is spawned, this class will cache it
	 * to save on spawning a new one with later requests (and to keep the node
	 * from being GC'd).
	 *
	 * This should not have to be overridden (it calls Invoke() with the
	 * specified outer; the transient-ness of the graph should signify that it
	 * is for a template).
	 *
	 * Unfortunately, the supplied graph requires a Blueprint outer. Certain 
	 * node types rely on this assumption; this is why the template-node and the
	 * supplied graph cannot belong to the transient package themselves.
	 *
	 * @param  Outer    A transient graph (with a Blueprint outer), to act as the template-node's outer.
	 * @return Should return a new/cached template-node (but could be null, or some pre-existing node... depends on the sub-class's Invoke() method).
	 */
	UEdGraphNode* MakeTemplateNode(UEdGraph* Outer) const;

protected:
	/**
	 * Protected Invoke() that let's sub-classes specify their own post-spawn
	 * delegate. Creates a new node based off of the set NodeClass.
	 * 
	 * @param  ParentGraph			The graph you want the node spawned into.
	 * @param  Location				Where you want the new node positioned in the graph.
	 * @param  PostSpawnDelegate	A delegate to run after spawning the node, but prior to allocating the node's pins.
	 * @return Null if it failed to spawn a node (if NodeClass is null), otherwise a newly spawned node.
	 */
	UEdGraphNode* Invoke(UEdGraph* ParentGraph, FVector2D const Location, FCustomizeNodeDelegate PostSpawnDelegate) const;

private:
	/**
	 * We try to avoid the spawning of node-templates (save on perf/mem where we
	 * can), but sometimes users need to pull non-static data from the node 
	 * itself (before it's spawned).
	 *
	 * Here we store the node template once it's created. Stored so that it can
	 * be reused (and not reallocated) and not GC'd.
	 */
	UPROPERTY(Transient)
	mutable UEdGraphNode* CachedNodeTemplate;
};

/*******************************************************************************
 * Templatized UBlueprintNodeSpawner Implementation
 ******************************************************************************/

//------------------------------------------------------------------------------
template<class NodeType>
UBlueprintNodeSpawner* UBlueprintNodeSpawner::Create(UObject* Outer, FCustomizeNodeDelegate PostSpawnDelegate)
{
	Create(NodeType::StaticClass(), Outer, PostSpawnDelegate);
}