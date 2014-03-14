// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Editor/ClassViewer/Public/ClassViewerModule.h"

/** 
  * Flags describing how to handle graph removal
  */
namespace EGraphRemoveFlags
{
	enum Type
	{
		/** No options */
		None = 0x00000000,

		/** If true recompile the blueprint after removing the graph, false if operations are being batched */
		Recompile = 0x00000001,

		/** If true mark the graph as transient, false otherwise */
		MarkTransient = 0x00000002,

		/** Helper enum for most callers */
		Default = Recompile | MarkTransient
	};
};


class UNREALED_API FBlueprintEditorUtils
{
public:

	/**
	 * Schedules and refreshes all nodes in the blueprint, making sure that nodes that affect function signatures get regenerated first
	 */
	static void RefreshAllNodes(UBlueprint* Blueprint);

	/**
	 * Optimized refresh of nodes that depend on external blueprints.  Refreshes the nodes, but does not recompile the skeleton class
	 */
	static void RefreshExternalBlueprintDependencyNodes(UBlueprint* Blueprint);

	/**
	 * Preloads the object and all the members it owns (nodes, pins, etc)
	 */
	static void PreloadMembers(UObject* InObject);

	/**
	 * Preloads the construction script, and all templates therein
	 */
	static void PreloadConstructionScript(UBlueprint* Blueprint);

	/**
	 * Regenerates the class at class load time, and refreshes the blueprint
	 */
	static UClass* RegenerateBlueprintClass(UBlueprint* Blueprint, UClass* ClassToRegenerate, UObject* PreviousCDO, TArray<UObject*>& ObjLoaded);

	/**
	 * Copies the default properties of all parent blueprint classes in the chain to the specified blueprint's skeleton CDO
	 */
	static void PropagateParentBlueprintDefaults(UClass* ClassToPropagate);

	/** Called on a Blueprint after it has been duplicated */
	static void PostDuplicateBlueprint(UBlueprint* Blueprint);

	/** Consigns the blueprint's generated classes to oblivion */
	static void RemoveGeneratedClasses(UBlueprint* Blueprint);

	/**
	 * Helper function to get the blueprint that ultimately owns a node.
	 *
	 * @param	InNode	Node to find the blueprint for.
	 * @return	The corresponding blueprint or NULL.
	 */
	static UBlueprint* FindBlueprintForNode(const UEdGraphNode* Node);

	/**
	 * Helper function to get the blueprint that ultimately owns a node.  Cannot fail.
	 *
	 * @param	InNode	Node to find the blueprint for.
	 * @return	The corresponding blueprint or NULL.
	 */
	static UBlueprint* FindBlueprintForNodeChecked(const UEdGraphNode* Node);

	/**
	 * Helper function to get the blueprint that ultimately owns a graph.
	 *
	 * @param	InGraph	Graph to find the blueprint for.
	 * @return	The corresponding blueprint or NULL.
	 */
	static UBlueprint* FindBlueprintForGraph(const UEdGraph* Graph);

	/**
	 * Helper function to get the blueprint that ultimately owns a graph.  Cannot fail.
	 *
	 * @param	InGraph	Graph to find the blueprint for.
	 * @return	The corresponding blueprint or NULL.
	 */
	static UBlueprint* FindBlueprintForGraphChecked(const UEdGraph* Graph);

	/**
	 * Updates sources of delegates.
	 */
	static void UpdateDelegatesInBlueprint(UBlueprint* Blueprint);

	/**
	 * Whether or not the blueprint should regenerate its class on load or not.  This prevents macros and other BP types not marked for reconstruction from being recompiled all the time
	 */
	static bool ShouldRegenerateBlueprint(UBlueprint* Blueprint);

	/**
	 * Blueprint has structurally changed (added/removed functions, graphs, etc...). Performs the following actions:
	 *  - Recompiles the skeleton class.
	 *  - Notifies any observers.
	 *  - Marks the package as dirty.
	 */
	static void MarkBlueprintAsStructurallyModified(UBlueprint* Blueprint);

	/**
	 * Blueprint has changed in some manner that invalidates the compiled data (link made/broken, default value changed, etc...)
	 *  - Marks the blueprint as status unknown
	 *  - Marks the package as dirty
	 */
	static void MarkBlueprintAsModified(UBlueprint* Blueprint);

	/** See whether or not the specified graph name / entry point name is unique */
	static bool IsGraphNameUnique(UBlueprint* Blueprint, const FName& InName);

	/**
	 * Creates a new empty graph.
	 *
	 * @param	ParentScope		The outer of the new graph (typically a blueprint).
	 * @param	GraphName		Name of the graph to add.
	 * @param	SchemaClass		Schema to use for the new graph.
	 *
	 * @return	null if it fails, else.
	 */
	static class UEdGraph* CreateNewGraph(UObject* ParentScope, const FName& GraphName, TSubclassOf<class UEdGraph> GraphClass, TSubclassOf<class UEdGraphSchema> SchemaClass);

	/** Adds a function graph to this blueprint.  If bIsUserCreated is true, the entry/exit nodes will be editable. SignatureFromClass is used to find signature for entry/exit nodes if using an existing signature. */
	static void AddFunctionGraph(UBlueprint* Blueprint, class UEdGraph* Graph,  bool bIsUserCreated, UClass* SignatureFromClass);

	/** Adds a macro graph to this blueprint.  If bIsUserCreated is true, the entry/exit nodes will be editable. SignatureFromClass is used to find signature for entry/exit nodes if using an existing signature. */
	static void AddMacroGraph(UBlueprint* Blueprint, class UEdGraph* Graph,  bool bIsUserCreated, UClass* SignatureFromClass);

	/** Adds an interface graph to this blueprint */
	static void AddInterfaceGraph(UBlueprint* Blueprint, class UEdGraph* Graph, UClass* InterfaceClass);

	/** Adds an ubergraph page to this blueprint */
	static void AddUbergraphPage(UBlueprint* Blueprint, class UEdGraph* Graph);

	/** Adds a domain-specific graph to this blueprint */
	static void AddDomainSpecificGraph(UBlueprint* Blueprint, class UEdGraph* Graph);

	/**
	 * Remove the supplied set of graphs from the Blueprint.
	 *
	 * @param	GraphsToRemove	The graphs to remove.
	 */
	static void RemoveGraphs( UBlueprint* Blueprint, const TArray<class UEdGraph*>& GraphsToRemove );

	/**
	 * Removes the supplied graph from the Blueprint.
	 *
	 * @param Blueprint			The blueprint containing the graph
	 * @param GraphToRemove		The graph to remove.
	 * @param Flags				Options to control the removal process
	 */
	static void RemoveGraph( UBlueprint* Blueprint, class UEdGraph* GraphToRemove, EGraphRemoveFlags::Type Flags = EGraphRemoveFlags::Default );

	/*
	 * Tries to rename the supplied graph.
	 * Cleans up function entry node if one exists and marks objects for modification
	 *
	 * @param Graph				The graph to rename.
	 * @param NewName			The new name for the graph
	 */
	static void RenameGraph(class UEdGraph* Graph, const FString& NewName );

	/*
	 * Renames the graph of the supplied node with a valid name based off of the suggestion.
	 *
	 * @param GraphNode			The node of the graph to rename.
	 * @param DesiredName		The initial form of the name to try
	 */
	static void RenameGraphWithSuggestion(class UEdGraph* Graph, TSharedPtr<class INameValidatorInterface> NameValidator, const FString& DesiredName );

	/**
	 * Removes the supplied node from the Blueprint.
	 *
	 * @param Node				The node to remove.
	 * @param bDontRecompile	If true, the blueprint will not be marked as modified, and will not be recompiled.  Useful for if you are removing several node at once, and don't want to recompile each time
	 */
	static void RemoveNode (UBlueprint* Blueprint, UEdGraphNode* Node, bool bDontRecompile=false);

	/**
	 * Returns the graph's top level graph (climbing up the hierarchy until there are no more graphs)
	 *
	 * @param InGraph		The graph to find the parent of
	 *
	 * @return				The top level graph
	 */
	static UEdGraph* GetTopLevelGraph(UEdGraph* InGraph);

	/** Look to see if an event already exists to override a particular function */
	static class UK2Node_Event* FindOverrideForFunction(const UBlueprint* Blueprint, const UClass* SignatureClass, FName SignatureName);


	/** Returns all nodes in all graphs of the specified class */
	template< class T > 
	static inline void GetAllNodesOfClass( const UBlueprint* Blueprint, TArray<T*>& OutNodes )
	{
		TArray<UEdGraph*> AllGraphs;
		Blueprint->GetAllGraphs(AllGraphs);
		for(int32 i=0; i<AllGraphs.Num(); i++)
		{
			check(AllGraphs[i] != NULL);
			TArray<T*> GraphNodes;
			AllGraphs[i]->GetNodesOfClass<T>(GraphNodes);
			OutNodes.Append(GraphNodes);
		}
	}

	/** @return true if this Blueprint is dependent on the given blueprint, false otherwise */
	static bool IsBlueprintDependentOn(UBlueprint const* Blueprint, UBlueprint const* TestBlueprint);

	/** Returns a list of loaded Blueprints that are dependent on the given Blueprint. */
	static void GetDependentBlueprints(UBlueprint* Blueprint, TArray<UBlueprint*>& DependentBlueprints);
	
	/** returns if a graph is an intermediate build product */
	static bool IsGraphIntermediate(const UEdGraph* Graph);

	/** Returns whether or not this blueprint is a data-only blueprint (has no code) */
	static bool IsDataOnlyBlueprint(const UBlueprint* Blueprint);

	/** Returns whether or not the blueprint is const during execution */
	static bool IsBlueprintConst(const UBlueprint* Blueprint);

	/**
	 * Whether or not this is an actor-based blueprint, and supports features like the uber-graph, components, etc
	 *
	 * @return	Whether or not this is an actor based blueprint
	 */
	static bool IsActorBased(const UBlueprint* Blueprint);

	/**
	 * Whether or not this blueprint is an interface, used only for defining functions to implement
	 *
	 * @return	Whether or not this is an interface blueprint
	 */
	static bool IsInterfaceBlueprint(const UBlueprint* Blueprint);

	/**
	 * Whether or not this blueprint is an interface, used only for defining functions to implement
	 *
	 * @return	Whether or not this is an interface blueprint
	 */
	static bool IsLevelScriptBlueprint(const UBlueprint* Blueprint);

	/** Do we support construction scripts */
	static bool SupportsConstructionScript(const UBlueprint* Blueprint);

	/** Returns the user construction script, if any */
	static UEdGraph* FindUserConstructionScript(const UBlueprint* Blueprint);

	/** Returns the event graph, if any */
	static UEdGraph* FindEventGraph(const UBlueprint* Blueprint);

	/** See if a class is the one generated by this blueprint */
	static bool DoesBlueprintDeriveFrom(const UBlueprint* Blueprint, UClass* TestClass);

	/** See if a field (property, function etc) is part of the blueprint chain, or  */
	static bool DoesBlueprintContainField(const UBlueprint* Blueprint, UField* TestField);

	/** Returns whether or not the blueprint supports overriding functions */
	static bool DoesSupportOverridingFunctions(const UBlueprint* Blueprint);

	/** Returns whether or not the blueprint supports timelines */
	static bool DoesSupportTimelines(const UBlueprint* Blueprint);

	/** Returns whether or not the blueprint supports event graphs*/
	static bool DoesSupportEventGraphs(const UBlueprint* Blueprint);

	/** Returns whether or not the blueprint supports implementing interfaces */
	static bool DoesSupportImplementingInterfaces(const UBlueprint* Blueprint);

	// Returns a descriptive name of the type of blueprint passed in
	static FString GetBlueprintTypeDescription(const UBlueprint* Blueprint);

	/** Constructs a class picker widget for reparenting the specified blueprint(s) */
	static TSharedRef<SWidget> ConstructBlueprintParentClassPicker( const TArray< UBlueprint* >& Blueprints, const FOnClassPicked& OnPicked);

	/** Try to open reparent menu for specified blueprint */
	static void OpenReparentBlueprintMenu( UBlueprint* Blueprint, const TSharedRef<SWidget>& ParentContent, const FOnClassPicked& OnPicked);
	static void OpenReparentBlueprintMenu( const TArray< UBlueprint* >& Blueprints, const TSharedRef<SWidget>& ParentContent, const FOnClassPicked& OnPicked);

	/** return find first native class in the hierarchy */
	static UClass* FindFirstNativeClass(UClass* Class);

	/**
	 * Gets the names of all graphs in the Blueprint
	 *
	 * @param [in,out]	GraphNames	The graph names will be appended to this array.
	 */
	static void GetAllGraphNames(const UBlueprint* Blueprint, TArray<FName>& GraphNames);


	//////////////////////////////////////////////////////////////////////////
	// Functions

	/**
	 * Gets a list of function names currently in use in the blueprint, based on the skeleton class
	 *
	 * @param			Blueprint		The blueprint to check
	 * @param [in,out]	FunctionNames	List of function names currently in use
	 */
	static void GetFunctionNameList(const UBlueprint* Blueprint, TArray<FName>& FunctionNames);

	/**
	 * Gets a list of delegates names in the blueprint, based on the skeleton class
	 *
	 * @param			Blueprint		The blueprint to check
	 * @param [in,out]	DelegatesNames	List of function names currently in use
	 */
	static void GetDelegateNameList(const UBlueprint* Blueprint, TArray<FName>& DelegatesNames);

	/** 
	 * Get a graph for delegate signature with given name, from given blueprint.
	 * 
	 * @param			Blueprint		Blueprint owning the delegate signature graph
	 * @param			DelegateName	Name of delegate.
	 * @return			Graph of delegate-signature function.
	 */
	static UEdGraph* GetDelegateSignatureGraphByName(UBlueprint* Blueprint, FName DelegateName);

	/** Checks if given graph contains a delegate signature */
	static bool IsDelegateSignatureGraph(const UEdGraph* Graph);
	/**
	 * Gets a list of pins that should hidden for a given function
	 *
	 * @param			CallingContext  The blueprint that you're looking to call the function from (some functions hide different pins depending on the blueprint they're in)
	 * @param			Function		The function to consider
	 * @param [out]		HiddenPins		Set of pins that should be hidden
	 */
	static void GetHiddenPinsForFunction(UBlueprint const* CallingContext, UFunction const* Function, TSet<FString>& HiddenPins);

	/** Makes sure that calls to parent functions are valid, and removes them if not */
	static void ConformCallsToParentFunctions(UBlueprint* Blueprint);

	//////////////////////////////////////////////////////////////////////////
	// Events

	/** Makes sure that all events we handle exist, and replace with custom events if not */
	static void ConformImplementedEvents(UBlueprint* Blueprint);

	//////////////////////////////////////////////////////////////////////////
	// Variables

	/**
	 * Gets the visible class variable list.  This includes both variables introduced here and in all superclasses.
	 *
	 * @param [in,out]	VisibleVariables	The visible variables will be appened to this array.
	 */
	static void GetClassVariableList(const UBlueprint* Blueprint, TArray<FName>& VisibleVariables, bool bIncludePrivateVars=false);

	/**
	 * Gets variables of specified type
	 *
	 * @param 			FEdGraphPinType	 			Type of variables to look for
	 * @param [in,out]	VisibleVariables			The visible variables will be appened to this array.
	 */
	static void GetNewVariablesOfType( const UBlueprint* Blueprint, const FEdGraphPinType& Type, TArray<FName>& OutVars);

	/**
	 * Adds a member variable to the blueprint.  It cannot mask a variable in any superclass.
	 *
	 * @param	NewVarName	Name of the new variable.
	 * @param	NewVarType	Type of the new variable.
	 * @param	DefaultValue	Default value stored as string
	 *
	 * @return	true if it succeeds, false if it fails.
	 */
	static bool AddMemberVariable(UBlueprint* Blueprint, const FName& NewVarName, const FEdGraphPinType& NewVarType, const FString& DefaultValue = FString());

	/**
	 * Removes a member variable if it was declared in this blueprint and not in a base class.
	 *
	 * @param	VarName	Name of the variable to be removed.
	 */
	static void RemoveMemberVariable(UBlueprint* Blueprint, const FName& VarName);
	
	/**
	 * Removes member variables if they were declared in this blueprint and not in a base class.
	 *
	 * @param	VarNames	Names of the variable to be removed.
	 */
	static void BulkRemoveMemberVariables(UBlueprint* Blueprint, const TArray<FName>& VarNames);

	/**
	 * Removes the variable nodes associated with the specified var name
	 *
	 * @param	Blueprint		The blueprint you want variable nodes removed from.
	 * @param	VarName			Name of the variable to be removed.
	 * @param	bForSelfOnly	True if you only want to delete variable nodes that represent ones owned by this blueprint,
	 *							false if you just want everything with the specified name removed (variables from other classes too).
	 */
	static void RemoveVariableNodes(UBlueprint* Blueprint, const FName& VarName, bool const bForSelfOnly = true);

	/**Rename a member variable*/
	static void RenameMemberVariable(UBlueprint* Blueprint, const FName& OldName, const FName& NewName);

	/** Rename a member variable created by a SCS entry */
	static void RenameComponentMemberVariable(UBlueprint* Blueprint, USCS_Node* Node, const FName& NewName);
	
	/** Changes the type of a member variable */
	static void ChangeMemberVariableType(UBlueprint* Blueprint, const FName& VariableName, const FEdGraphPinType& NewPinType);

	/** Replaces all variable references in the specified blueprint */
	static void ReplaceVariableReferences(UBlueprint* Blueprint, const FName& OldName, const FName& NewName);

	/** Replaces all variable references in the specified blueprint */
	static void ReplaceVariableReferences(UBlueprint* Blueprint, const UProperty* OldVariable, const UProperty* NewVariable);

	/** Validate child blueprint component member variables, member variables, and timelines, and function graphs against the given variable name */
	static void ValidateBlueprintChildVariables(UBlueprint* InBlueprint, const FName& InVariableName);

	/** Rename a Timeline. If bRenameNodes is true, will also rename any timeline nodes associated with this timeline */
	static bool RenameTimeline (UBlueprint* Blueprint, const FName& OldVarName, const FName& NewVarName);

	/**
	 * Sets the Blueprint edit-only flag on the variable with the specified name
	 *
	 * @param	VarName				Name of the var to set the flag on
	 * @param	bNewBlueprintOnly	The new value to set the bitflag to
	 */
	static void SetBlueprintOnlyEditableFlag(UBlueprint* Blueprint, const FName& VarName, const bool bNewBlueprintOnly);

	/**
	 * Sets the Interp flag on the variable with the specified name to make available to matinee
	 *
	 * @param	VarName				Name of the var to set the flag on
	 * @param	bInterp	true to make variable available to Matinee, false otherwise
	 */
	static void SetInterpFlag(UBlueprint* Blueprint, const FName& VarName, const bool bInterp);

	/**
	 * Sets the Transient flag on the variable with the specified name
	 *
	 * @param	InVarName				Name of the var to set the flag on
	 * @param	bInIsTransient			The new value to set the bitflag to
	 */
	static void SetVariableTransientFlag(UBlueprint* InBlueprint, const FName& InVarName, const bool bInIsTransient);

	/**
	 * Sets the Save Game flag on the variable with the specified name
	 *
	 * @param	InVarName				Name of the var to set the flag on
	 * @param	bInIsSaveGame			The new value to set the bitflag to
	 */
	static void SetVariableSaveGameFlag(UBlueprint* InBlueprint, const FName& InVarName, const bool bInIsSaveGame);

	/** Sets a metadata key/value on the specified variable */
	static void SetBlueprintVariableMetaData(UBlueprint* Blueprint, const FName& VarName, const FName& MetaDataKey, const FString& MetaDataValue);

	/** Get a metadata key/value on the specified variable, or timeline if it exists, returning false if it does not exist */
	static bool GetBlueprintVariableMetaData(UBlueprint* Blueprint, const FName& VarName, const FName& MetaDataKey, FString& OutMetaDataValue);

	/** Clear metadata key on specified variable, or timeline */
	static void RemoveBlueprintVariableMetaData(UBlueprint* Blueprint, const FName& VarName, const FName& MetaDataKey);

	/**
	 * Sets the custom category on the variable with the specified name.
	 * @note: Will not change the category for variables defined via native classes.
	 *
	 * @param	VarName			Name of the variable
	 * @param	VarCategory		The new value of the custom category for the variable
	 * @param	bDontRecompile	If true, the blueprint will not be marked as modified, and will not be recompiled.  
	 */
	static void SetBlueprintVariableCategory(UBlueprint* Blueprint, const FName& VarName, const FName& NewCategory, bool bDontRecompile=false);

	/**
	 * Gets the custom category on the variable with the specified name.
	 *
	 * @param	VarName			Name of the variable
	 * @return	The custom category (None indicates the name will be the same as the blueprint)
	 */
	static FName GetBlueprintVariableCategory(UBlueprint* Blueprint, const FName& VarName);

	/** Gets pointer to PropertyFlags of variable */
	static uint64* GetBlueprintVariablePropertyFlags(UBlueprint* Blueprint, const FName& VarName);

	/** Get RepNotify function name of variable */
	static FName GetBlueprintVariableRepNotifyFunc(UBlueprint* Blueprint, const FName& VarName);

	/** Set RepNotify function of variable */
	static void SetBlueprintVariableRepNotifyFunc(UBlueprint* Blueprint, const FName& VarName, const FName& RepNotifyFunc);

	/**
	 * Find the index of a variable first declared in this blueprint. Returns INDEX_NONE if not found.
	 *
	 * @param	InName	Name of the variable to find.
	 *
	 * @return	The index of the variable, or INDEX_NONE if it wasn't introduced in this blueprint.
	 */
	static int32 FindNewVariableIndex(const UBlueprint* Blueprint, const FName& InName);

	/** Change the order of variables in the Blueprint */
	static bool MoveVariableBeforeVariable(UBlueprint* Blueprint, FName VarNameToMove, FName TargetVarName, bool bDontRecompile);

	/** Move all vars of the supplied category */
	static bool MoveCategoryBeforeCategory(UBlueprint* Blueprint, FName CategoryToMove, FName TargetCategory);

	/** Find first variable of the supplied category */
	static int32 FindFirstNewVarOfCategory(const UBlueprint* Blueprint, FName Category);

	/**
	 * Find the index of a timeline first declared in this blueprint. Returns INDEX_NONE if not found.
	 *
	 * @param	InName	Name of the variable to find.
	 *
	 * @return	The index of the variable, or INDEX_NONE if it wasn't introduced in this blueprint.
	 */
	static int32 FindTimelineIndex(const UBlueprint* Blueprint, const FName& InName);

	/** 
	 * Gets a list of SCS node variable names for the given blueprint.
	 *
	 * @param [in,out]	VariableNames		The list of variable names for the SCS node array.
	 */
	static void GetSCSVariableNameList(const UBlueprint* Blueprint, TArray<FName>& VariableNames);

	/**
	 * Find the index of a SCS_Node first declared in this blueprint. Returns INDEX_NONE if not found.
	 *
	 * @param	InName	Name of the variable to find.
	 *
	 * @return	The index of the variable, or INDEX_NONE if it wasn't introduced in this blueprint.
	 */
	static int32 FindSCS_Node(const UBlueprint* Blueprint, const FName& InName);
	
	/** Returns whether or not the specified member var is a component */
	static bool IsVariableComponent(const FBPVariableDescription& Variable);

	/** Indicates if the variable is used on any graphs in this Blueprint*/
	static bool IsVariableUsed(const UBlueprint* Blueprint, const FName& Name);

	static void ImportKismetDefaultValueToProperty(UEdGraphPin* SourcePin, UProperty* DestinationProperty, uint8* DestinationAddress, UObject* OwnerObject);
	
	static void ExportPropertyToKismetDefaultValue(UEdGraphPin* TargetPin, UProperty* SourceProperty, uint8* SourceAddress, UObject* OwnerObject);

	static bool PropertyValueFromString(const UProperty* Property, const FString& StrValue, uint8* ContainerMem);

	/** Call PostEditChange() on all Actors based on the given Blueprint */
	static void PostEditChangeBlueprintActors(UBlueprint* Blueprint);

	/** Checks if the property can be modified in given blueprint */
	static bool IsPropertyReadOnlyInCurrentBlueprint(const UBlueprint* Blueprint, const UProperty* Property);

	/** Ensures that the CDO root component reference is valid for Actor-based Blueprints */
	static void UpdateRootComponentReference(UBlueprint* Blueprint);

	//////////////////////////////////////////////////////////////////////////
	// Interface

	/** Add a new interface, and member function graphs to the blueprint */
	static bool ImplementNewInterface(UBlueprint* Blueprint, const FName& InterfaceClassName);

	/** Remove an implemented interface, and its associated member function graphs.  If bPreserveFunctions is true, then the interface will move its functions to be normal implemented blueprint functions */
	static void RemoveInterface(UBlueprint* Blueprint, const FName& InterfaceClassName, bool bPreserveFunctions = false);

	/** Gets the graphs currently in the blueprint associated with the specified interface */
	static void GetInterfaceGraphs(UBlueprint* Blueprint, const FName& InterfaceClassName, TArray<UEdGraph*>& ChildGraphs);

	/** Makes sure that all graphs for all interfaces we implement exist, and add if not */
	static void ConformImplementedInterfaces(UBlueprint* Blueprint);

	/** Makes sure that all NULL graph references are removed from SubGraphs and top-level graph arrays */
	static void PurgeNullGraphs(UBlueprint* Blueprint);

	/** Handle old AnimBlueprints (state machines in the wrong position, transition graphs with the wrong schema, etc...) */
	static void UpdateOutOfDateAnimBlueprints(UBlueprint* Blueprint);

	/* Update old pure functions to be pure using new system*/
	static void UpdateOldPureFunctions(UBlueprint* Blueprint);

	/** Handle fixing up composite nodes within the blueprint*/
	static void UpdateOutOfDateCompositeNodes(UBlueprint* Blueprint);

	/** Handle fixing up composite nodes within the specified Graph of Blueprint, and correctly setting the Outer */
	static void UpdateOutOfDateCompositeWithOuter(UBlueprint* Blueprint, UEdGraph* Outer );

	/** Handle stale components and ensure correct flags are set */
	static void UpdateComponentTemplates(UBlueprint* Blueprint);

	/** Handle stale transactional flags on blueprints */
	static void UpdateTransactionalFlags(UBlueprint* Blueprint);

	/** Handle stale pin watches */
	static void UpdateStalePinWatches( UBlueprint* Blueprint );

	/** Return the first function from implemented interface with given name */
	static UFunction* FindFunctionInImplementedInterfaces(const UBlueprint* Blueprint, const FName& FunctionName, bool* bOutInvalidInterface = NULL );

	/** 
	 * Build a list of all interface classes either implemented by this blueprint or through inheritance
	 * @param		Blueprint				The blueprint to find interfaces for
	 * @param		bGetAllInterfaces		If true, get all the implemented and inherited. False, just gets the interfaces implemented directly.
	 * @param [out]	ImplementedInterfaces	The interfaces implemented by this blueprint
	 */
	static void FindImplementedInterfaces(const UBlueprint* Blueprint, bool bGetAllInterfaces, TArray<UClass*>& ImplementedInterfaces);

	/**
	 * Finds a unique name with a base of the passed in string, appending numbers as needed
	 *
	 * @param InBlueprint		The blueprint the kismet object's name needs to be unique in
	 * @param InBaseName		The base name to use
	 *
	 * @return					A unique name that will not conflict in the Blueprint
	 */
	static FName FindUniqueKismetName(const UBlueprint* InBlueprint, const FString& InBaseName);

	/** Finds a unique and valid name for a custom event */
	static FName FindUniqueCustomEventName(const UBlueprint* Blueprint);

	//////////////////////////////////////////////////////////////////////////
	// Timeline

	/** Finds a name for a timeline that is not already in use */
	static FName FindUniqueTimelineName(const UBlueprint* Blueprint);

	/** Add a new timeline with the supplied name to the blueprint */
	static class UTimelineTemplate* AddNewTimeline(UBlueprint* Blueprint, const FName& TimelineVarName);

	/** Remove the timeline from the blueprint 
	 * @note Just removes the timeline from the associated timelist in the Blueprint. Does not remove the node graph
	 * object representing the timeline itself.
	 * @param Timeline			The timeline to remove
	 * @param bDontRecompile	If true, the blueprint will not be marked as modified, and will not be recompiled.  
	 */
	static void RemoveTimeline(UBlueprint* Blueprint, class UTimelineTemplate* Timeline, bool bDontRecompile=false);

	/** Find the node that owns the supplied timeline template */
	static class UK2Node_Timeline* FindNodeForTimeline(UBlueprint* Blueprint, UTimelineTemplate* Timeline);

	//////////////////////////////////////////////////////////////////////////
	// LevelScriptBlueprint

	/** Find how many nodes reference the supplied actor */
	static int32 FindNumReferencesToActorFromLevelScript(ULevelScriptBlueprint* LevelScriptBlueprint, AActor* InActor);

	/** Function to call modify() on all graph nodes which reference this actor */
	static void  ModifyActorReferencedGraphNodes(ULevelScriptBlueprint* LevelScriptBlueprint, const AActor* InActor);

	/**
	 * Called after a level script blueprint is changed and nodes should be refreshed for it's new level script actor
	 *
	 * @param	LevelScriptActor	The newly-created level script actor that should be (re-)bound to
	 * @param	ScriptBlueprint		The level scripting blueprint that contains the bound events to try and bind delegates to this actor for
	 */
	static bool FixLevelScriptActorBindings(ALevelScriptActor* LevelScriptActor, const class ULevelScriptBlueprint* ScriptBlueprint);

	/**
	 * Find how many actors reference the supplied actor
	 *
	 * @param InActor The Actor to count references to.
	 * @param InClassesToIgnore An array of class types to ignore, even if there is an instance of one that references the InActor
	 * @param OutReferencingActors An array of actors found that reference the specified InActor
	 */
	static void FindActorsThatReferenceActor( AActor* InActor, TArray<UClass*>& InClassesToIgnore, TArray<AActor*>& OutReferencingActors );

	//////////////////////////////////////////////////////////////////////////
	// Diagnostics

	// Diagnostic use only: Lists all of the objects have a direct outer of Package
	static void ListPackageContents(UPackage* Package, FOutputDevice& Ar);

	// Diagnostic exec commands
	static bool KismetDiagnosticExec(const TCHAR* Stream, FOutputDevice& Ar);

	/** Called by FixLevelScriptActorBindings() to allow external modules to update level script actors when they are
	    recreated after a blueprint change */
	DECLARE_EVENT_ThreeParams( FBlueprintEditorUtils, FFixLevelScriptActorBindingsEvent, ALevelScriptActor* /* LevelScriptActor */, const ULevelScriptBlueprint* /* LevelScriptBlueprint */, bool& /* bWasSuccessful */ );
	static FFixLevelScriptActorBindingsEvent& OnFixLevelScriptActorBindings();

	/**
	 * Searches the world for any blueprints that are open and do not have a debug instances set and sets one if possible.
	 * It will favor a selected instance over a non selected one
	 */
	static void FindAndSetDebuggableBlueprintInstances();

	/**
	 * Records new node class and type for analytics submission
	 */
	static void AnalyticsTrackNewNode( UEdGraphNode* NewNode, FName NodeClass, FName NodeType );

	/**
	 * Generates a unique graph name for the supplied blueprint (guaranteed to not 
	 * cause a naming conflict at the time of the call).
	 * 
	 * @param  BlueprintOuter	The blueprint you want a unique graph name for.
	 * @param  ProposedName		The name you want to give the graph (the result will be some permutation of this string).
	 * @return A unique name intended for a new graph.
	 */
	static FName GenerateUniqueGraphName(UBlueprint* const BlueprintOuter, FString const& ProposedName);

	/* Checks if the passed in selection set causes cycling on compile
	 *
	 * @param InSelectionSet		The selection set to check for a cycle within
	 * @param InMessageLog			Log to report cycling errors to
	 *
	 * @return						Returns TRUE if the selection does cause cycling
	 */
	static bool CheckIfSelectionIsCycling(const TSet<UEdGraphNode*>& InSelectionSet, FCompilerResultsLog& InMessageLog);
	
	/**
	 * A utility function intended to aid the construction of a specific blueprint 
	 * palette item. Some items can be renamed, so this method determines if that is 
	 * allowed.
	 * 
	 * @param  ActionIn				The action associated with the palette item you're querying for.
	 * @param  BlueprintEditorIn	The blueprint editor owning the palette item you're querying for.
	 * @return True is the item CANNOT be renamed, false if it can.
	 */
	static bool IsPaletteActionReadOnly(TSharedPtr<FEdGraphSchemaAction> ActionIn, TSharedPtr<class FBlueprintEditor> const BlueprintEditorIn);

protected:
	// Removes all NULL graph references from the SubGraphs array and recurses thru the non-NULL ones
	static void CleanNullGraphReferencesRecursive(UEdGraph* Graph);

	// Removes all NULL graph references in the specified array
	static void CleanNullGraphReferencesInArray(UBlueprint* Blueprint, TArray<UEdGraph*>& GraphArray);

	/** Static global event that is broadcast when a blueprint is changed and FixLevelScriptActorBindings() is called */
	static FFixLevelScriptActorBindingsEvent FixLevelScriptActorBindingsEvent;

	/**
	 * Checks that the actor type matches the blueprint type (or optionally is BASED on the same type. 
	 *
	 * @param InActorObject						The object to check
	 * @param InBlueprint						The blueprint to check against
	 * @param bInDisallowDerivedBlueprints		if true will only allow exact type matches, if false derived types are allowed.
	 */
	static bool IsObjectADebugCandidate( AActor* InActorObject, UBlueprint* InBlueprint , bool bInDisallowDerivedBlueprints );

	/** Validate child blueprint member variables against the given variable name */
	static bool ValidateAllMemberVariables(UBlueprint* InBlueprint, UBlueprint* InParentBlueprint, const FName& InVariableName);

	/** Validate child blueprint component member variables against the given variable name */
	static bool ValidateAllComponentMemberVariables(UBlueprint* InBlueprint, UBlueprint* InParentBlueprint, const FName& InVariableName);

	/** Validates all timelines of the passed blueprint against the given variable name */
	static bool ValidateAllTimelines(UBlueprint* InBlueprint, UBlueprint* InParentBlueprint, const FName& InVariableName);

	/** Validates all function graphs of the passed blueprint against the given variable name */
	static bool ValidateAllFunctionGraphs(UBlueprint* InBlueprint, UBlueprint* InParentBlueprint, const FName& InVariableName);

	/**
	 * Checks if the passed node connects to the selection set
	 *
	 * @param InNode				The node to check
	 * @param InSelectionSet		The selection set to check for a connection to
	 *
	 * @return						Returns TRUE if the node does connect to the selection set
	 */
	static bool CheckIfNodeConnectsToSelection(UEdGraphNode* InNode, const TSet<UEdGraphNode*>& InSelectionSet);

};
