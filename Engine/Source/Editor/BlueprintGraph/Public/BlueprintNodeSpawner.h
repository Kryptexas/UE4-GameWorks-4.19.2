// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlueprintNodeBinder.h"
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
class BLUEPRINTGRAPH_API UBlueprintNodeSpawner : public UObject, public FBlueprintNodeBinder
{
	GENERATED_UCLASS_BODY()
	DECLARE_DELEGATE_TwoParams(FCustomizeNodeDelegate, UEdGraphNode*, bool);

public:
	/** */
	virtual ~UBlueprintNodeSpawner();

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
	virtual FName GetDefaultMenuIcon(FLinearColor& ColorOut) const;

	/**
	 * Retrieves a cached template for the node that this is set to spawn. Will
	 * instantiate a new template if one didn't previously exist. If the 
	 * template-node is not compatible with any of our cached UEdGraph outers, 
	 * then we use TargetGraph as a model to create one that will work.
	 *
	 * @param  TargetGraph    Optional param that defines a compatible graph outer (used as an achetype if we don't have a compatible outer on hand).
	 * @return Should return a new/cached template-node (but could be null, or some pre-existing node... depends on the sub-class's Invoke() method).
	 */
	UEdGraphNode* GetTemplateNode(UEdGraph* TargetGraph = nullptr) const;

	// FBlueprintNodeBinder interface
	virtual bool CanBind(UObject const* BindingCandidate) const override;
	virtual bool BindToNode(UEdGraphNode* Node, UObject* Binding) const override;
	// End FBlueprintNodeBinder interface

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
};

/*******************************************************************************
 * Templatized UBlueprintNodeSpawner Implementation
 ******************************************************************************/

//------------------------------------------------------------------------------
template<class NodeType>
UBlueprintNodeSpawner* UBlueprintNodeSpawner::Create(UObject* Outer, FCustomizeNodeDelegate PostSpawnDelegate)
{
	return Create(NodeType::StaticClass(), Outer, PostSpawnDelegate);
}