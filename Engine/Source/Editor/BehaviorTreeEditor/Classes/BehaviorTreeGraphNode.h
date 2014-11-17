// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTreeEditorTypes.h"
#include "DiffResults.h"
#include "BehaviorTreeGraphNode.generated.h"

UCLASS()
class UBehaviorTreeGraphNode : public UEdGraphNode
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	UObject* NodeInstance;

	/** parent node if node is not a standalone node*/
	UPROPERTY()
	UBehaviorTreeGraphNode* ParentNode;

	/** only some of behavior tree nodes support decorators */
	UPROPERTY()
	TArray<UBehaviorTreeGraphNode*> Decorators;

	/** only some of behavior tree nodes support services */
	UPROPERTY()
	TArray<UBehaviorTreeGraphNode*> Services;

	// Begin UEdGraphNode Interface
	virtual class UBehaviorTreeGraph* GetBehaviorTreeGraph();
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual void AutowireNewNode(UEdGraphPin* FromPin) OVERRIDE;
	virtual void PostPlacedNewNode() OVERRIDE;
	virtual void PrepareForCopying() OVERRIDE;
	virtual void DestroyNode() OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual void NodeConnectionListChanged() OVERRIDE;
	virtual bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* DesiredSchema) const OVERRIDE;
	virtual void FindDiffs(class UEdGraphNode* OtherNode, struct FDiffResults& Results) OVERRIDE;
	// End UEdGraphNode Interface

	// Begin UObject Interface
	virtual void PostEditImport() OVERRIDE;
	// End UObject

	// @return the input pin for this state
	virtual UEdGraphPin* GetInputPin(int32 InputIndex=0) const;
	// @return the output pin for this state
	virtual UEdGraphPin* GetOutputPin(int32 InputIndex=0) const;
	//
	virtual UEdGraph* GetBoundGraph() const { return NULL; }

	virtual FString	GetDescription() const;
	/** check if node can accept breakpoints */
	virtual bool CanPlaceBreakpoints() const { return false; }
	virtual void PostCopyNode();

	virtual bool IsSubNode() const { return false; }

	/*
	 * Finds the difference in Behavior Tree properties
	 * 
	 * @param Struct The struct of the class we are looking at
	 * @param Data The raw data for the UObject we are comparing LHS
	 * @param OtherData The raw data for the UObject we are comparing RHS
	 * @param Results The Results where differences are stored
	 * @param Diff The single result with default parameters setup
	 */
	void DiffProperties(UStruct* Struct, void* Data, void* OtherData, FDiffResults& Results, FDiffSingleResult& Diff);

	void ClearDebuggerState();

	/** add subnode */
	void AddSubNode(UBehaviorTreeGraphNode* NodeTemplate, class UEdGraph* ParentGraph);

	/** gets icon resource name for title bar */
	virtual FName GetNameIcon() const;

	/** instance class */
	struct FClassData ClassData;

	/** highlighting nodes in abort range for more clarity when setting up decorators */
	uint32 bHighlightInAbortRange0 : 1;

	/** highlighting nodes in abort range for more clarity when setting up decorators */
	uint32 bHighlightInAbortRange1 : 1;

	/** highlighting connections in search range for more clarity when setting up decorators */
	uint32 bHighlightInSearchRange0 : 1;

	/** highlighting connections in search range for more clarity when setting up decorators */
	uint32 bHighlightInSearchRange1 : 1;

	/** highlighting nodes during quick find */
	uint32 bHighlightInSearchTree : 1;

	/** debugger flag: breakpoint exists */
	uint32 bHasBreakpoint : 1;

	/** debugger flag: breakpoint is enabled */
	uint32 bIsBreakpointEnabled : 1;

	/** debugger flag: mark node as active (current state) */
	uint32 bDebuggerMarkCurrentlyActive : 1;

	/** debugger flag: mark node as active (browsing previous states) */
	uint32 bDebuggerMarkPreviouslyActive : 1;

	/** debugger flag: briefly flash active node */
	uint32 bDebuggerMarkFlashActive : 1;

	/** debugger flag: mark as succeeded search path */
	uint32 bDebuggerMarkSearchSucceeded : 1;

	/** debugger flag: mark as failed on search path */
	uint32 bDebuggerMarkSearchFailed : 1;

	/** debugger flag: mark as optional on search path */
	uint32 bDebuggerMarkSearchOptional : 1;

	/** debugger flag: mark as trigger of search path */
	uint32 bDebuggerMarkSearchTrigger : 1;

	/** debugger flag: mark as trigger of discarded search path */
	uint32 bDebuggerMarkSearchFailedTrigger : 1;

	/** debugger flag: mark as going to parent */
	uint32 bDebuggerMarkSearchReverseConnection : 1;

	/** debugger flag: mark stopped on this breakpoint */
	uint32 bDebuggerMarkBreakpointTrigger : 1;

	/** debugger variable: index on search path */
	int32 DebuggerSearchPathIndex;

	/** debugger variable: number of nodes on search path */
	int32 DebuggerSearchPathSize;

	/** debugger variable: incremented on change of debugger flags for render updates */
	int32 DebuggerUpdateCounter;

	/** used to show node's runtime description rather than static one */
	FString DebuggerRuntimeDescription;

	/** message for deprecated classes */
	FString DeprecationMessage;

	/** subnode index assigned during copy operation to connect nodes again on paste */
	UPROPERTY()
	int32 CopySubNodeIndex;

protected:

	/** creates add decorator... submenu */
	void CreateAddDecoratorSubMenu(class FMenuBuilder& MenuBuilder, UEdGraph* Graph) const;

	/** creates add service... submenu */
	void CreateAddServiceSubMenu(class FMenuBuilder& MenuBuilder, UEdGraph* Graph) const;

	/** add right click menu to create subnodes: Decorators */
	void AddContextMenuActionsDecorators(const FGraphNodeContextMenuBuilder& Context) const;

	/** add right click menu to create subnodes: Services */
	void AddContextMenuActionsServices(const FGraphNodeContextMenuBuilder& Context) const;

	void ResetNodeOwner();
};

