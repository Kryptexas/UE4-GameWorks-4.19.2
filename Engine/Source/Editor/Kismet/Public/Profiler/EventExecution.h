// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TracePath.h"

struct FSlateBrush;
class UBlueprintGeneratedClass;

/**  Execution node flags */
namespace EScriptExecutionNodeFlags
{
	enum Type
	{
		None				= 0x00000000,	// No Flags
		Class				= 0x00000001,	// Class
		Instance			= 0x00000002,	// Instance
		Event				= 0x00000004,	// Event
		FunctionCall		= 0x00000008,	// Function Call
		MacroCall			= 0x00000010,	// Macro Call
		CallSite			= 0x00000018,	// Function / Macro call site
		MacroNode			= 0x00000100,	// Macro Node
		ConditionalBranch	= 0x00000200,	// Node has multiple exit pins using a jump
		SequentialBranch	= 0x00000400,	// Node has multiple exit pins ran in sequence
		BranchNode			= 0x00000600,	// BranchNode
		Node				= 0x00000800,	// Node timing
		ExecPin				= 0x00001000	// Exec pin dummy node
	};
}

/** Stat Container Type */
namespace EScriptStatContainerType
{
	enum Type
	{
		Standard = 0,
		Container,
		SequentialBranch,
		NewExecutionPath
	};
}

//////////////////////////////////////////////////////////////////////////
// FScriptNodeExecLinkage

class KISMET_API FScriptNodeExecLinkage
{
public:

	struct FLinearExecPath
	{
		FLinearExecPath(TSharedPtr<class FScriptExecutionNode> NodeIn, const FTracePath TracePathIn)
			: TracePath(TracePathIn)
			, LinkedNode(NodeIn)
		{
		}

		FTracePath TracePath;
		TSharedPtr<class FScriptExecutionNode> LinkedNode;
	};

	/** Returns the parent node */
	TSharedPtr<class FScriptExecutionNode> GetParentNode() { return ParentNode; }

	/** Sets the parent node */
	void SetParentNode(TSharedPtr<class FScriptExecutionNode> InParentNode) { ParentNode = InParentNode; }

	/** Returns the linked nodes map */
	TMap<int32, TSharedPtr<class FScriptExecutionNode>>& GetLinkedNodes() { return LinkedNodes; }

	/** Returns the number of linked nodes */
	int32 GetNumLinkedNodes() const { return LinkedNodes.Num(); }

	/** Returns the linked node for the specified pin script offset */
	TSharedPtr<class FScriptExecutionNode> GetLinkedNodeByPinScriptOffset(const int32 PinScriptOffset) { return LinkedNodes[PinScriptOffset]; }

	/** Add linked node */
	void AddLinkedNode(const int32 PinScriptOffset, TSharedPtr<class FScriptExecutionNode> LinkedNode);

	/** Returns the child nodes map */
	TArray<TSharedPtr<class FScriptExecutionNode>>& GetChildNodes() { return ChildNodes; }

	/** Returns the number of children */
	int32 GetNumChildren() const { return ChildNodes.Num(); }

	/** Returns the child node for the specified index */
	TSharedPtr<class FScriptExecutionNode> GetChildByIndex(const int32 ChildIndex) { return ChildNodes[ChildIndex]; }

	/** Add child node */
	void AddChildNode(TSharedPtr<class FScriptExecutionNode> ChildNode) { ChildNodes.Add(ChildNode); }

protected:

	/** Parent node */
	TSharedPtr<class FScriptExecutionNode> ParentNode;
	/** Linked nodes */
	TMap<int32, TSharedPtr<class FScriptExecutionNode>> LinkedNodes;
	/** Child nodes */
	TArray<TSharedPtr<class FScriptExecutionNode>> ChildNodes;

};

//////////////////////////////////////////////////////////////////////////
// FScriptNodePerfData

class KISMET_API FScriptNodePerfData
{
public:

	/** Get perf data for instance and tracepath */
	TSharedPtr<class FScriptPerfData> GetPerfDataByInstanceAndTracePath(FName InstanceName, const FTracePath& TracePath);

	/** Get all instance perf data for the trace path, excluding the global blueprint data */
	void GetInstancePerfDataByTracePath(const FTracePath& TracePath, TArray<TSharedPtr<FScriptPerfData>>& ResultsOut);

	/** Get global blueprint perf data for the trace path */
	TSharedPtr<FScriptPerfData> GetBlueprintPerfDataByTracePath(const FTracePath& TracePath);

protected:

	/** FScriptExeutionPath hash to perf data */
	TMap<FName, TMap<const uint32, TSharedPtr<FScriptPerfData>>> InstanceInputPinToPerfDataMap;

};

//////////////////////////////////////////////////////////////////////////
// FScriptExecutionNode

class KISMET_API FScriptExecutionNode : public FScriptNodeExecLinkage, public FScriptNodePerfData, public TSharedFromThis<FScriptExecutionNode>
{
public:

	FScriptExecutionNode();
    virtual ~FScriptExecutionNode() {}

	bool operator == (const FScriptExecutionNode& NodeIn) const;

	/** Get the node's name */
	FName GetName() const { return NodeName; }

	/** Set the node's name */
	void SetName(FName NewName) { NodeName = NewName; }

	/** Set the node's name by string */
	void SetNameByString(const FString& NewName) { NodeName = FName(*NewName); }

	/** Returns the owning graph name */
	FName GetGraphName() const { return OwningGraphName; }

	/** Sets the owning graph name */
	void SetGraphName(FName GraphNameIn) { OwningGraphName = GraphNameIn; }

	/** Set the node's flags */
	void SetFlags(const uint32 NewFlags) { NodeFlags = NewFlags; }

	/** Add to the node's flags */
	void AddFlags(const uint32 NewFlags) { NodeFlags |= NewFlags; }

	/** Remove from the node's flags */
	void RemoveFlags(const uint32 NewFlags) { NodeFlags &= ~NewFlags; }

	/** Does the node contain these flags */
	bool HasFlags(const uint32 Flags) const { return (NodeFlags & Flags) != 0U; }

	/** Returns if this exec event represents a change in class/blueprint */
	bool IsClass() const { return (NodeFlags & EScriptExecutionNodeFlags::Class) != 0U; }

	/** Returns if this exec event represents a change in instance */
	bool IsInstance() const { return (NodeFlags & EScriptExecutionNodeFlags::Instance) != 0U; }

	/** Returns if this exec event represents the start of an event execution path */
	bool IsEvent() const { return (NodeFlags & EScriptExecutionNodeFlags::Event) != 0U; }

	/** Returns if this event is a function callsite event */
	bool IsFunctionCallSite() const { return (NodeFlags & EScriptExecutionNodeFlags::FunctionCall) != 0U; }

	/** Returns if this event is a macro callsite event */
	bool IsMacroCallSite() const { return (NodeFlags & EScriptExecutionNodeFlags::MacroCall) != 0U; }

	/** Returns if this event happened inside a macro instance */
	bool IsMacroNode() const { return (NodeFlags & EScriptExecutionNodeFlags::MacroNode) != 0U; }

	/** Returns if this event potentially multiple exit sites */
	bool IsBranch() const { return (NodeFlags & EScriptExecutionNodeFlags::BranchNode) != 0U; }

	/** Returns the display name for widget UI */
	const FText& GetDisplayName() const { return DisplayName; }

	/** Sets the display name for widget UI */
	void SetDisplayName(const FText InDisplayName) { DisplayName = InDisplayName; }

	/** Returns the tooltip for widget UI */
	const FText& GetToolTipText() const { return Tooltip; }

	/** Sets the tooltip for widget UI */
	void SetToolTipText(const FText InTooltip) { Tooltip = InTooltip; }

	/** Returns the icon color for widget UI */
	const FLinearColor& GetIconColor() const { return IconColor; }

	/** Sets the icon color for widget UI */
	void SetIconColor(FLinearColor InIconColor) { IconColor = InIconColor; }

	/** Returns the icon for widget UI */
	const FSlateBrush* GetIcon() const { return Icon; }

	/** Sets the icon for widget UI */
	void SetIcon(FSlateBrush* InIcon) { Icon = InIcon; }

	/** Returns the profiler heat color */
	FLinearColor GetHeatColor(const FTracePath& TracePath) const;

	/** Returns the current expansion state for widget UI */
	bool IsExpanded() const { return bExpansionState; }

	/** Sets the current expansion state for widget UI */
	void SetExpanded(bool bIsExpanded) { bExpansionState = bIsExpanded; }

	/** Returns if this node represents a blueprint */
	virtual bool IsRootNode() const { return false; }

	/** Return the linear execution path from this node */
	void GetLinearExecutionPath(TArray<FLinearExecPath>& LinearExecutionNodes, const FTracePath& TracePath);

	/** Get statistic container type */
	virtual EScriptStatContainerType::Type GetStatisticContainerType() const;

	/** Refresh Stats */
	virtual void RefreshStats(const FTracePath& TracePath);

	/** Get all exec nodes */
	virtual void GetAllExecNodes(TMap<FName, TSharedPtr<FScriptExecutionNode>>& ExecNodesOut);

protected:

	/** Node name */
	FName NodeName;
	/** Owning graph name */
	FName OwningGraphName;
	/** Node flags to describe the source graph node type */
	uint32 NodeFlags;
	/** Display name for widget UI */
	FText DisplayName;
	/** Tooltip for widget UI */
	FText Tooltip;
	/** Icon color for widget UI */
	FLinearColor IconColor;
	/** Icon for widget UI */
	FSlateBrush* Icon;
	/** Expansion state */
	bool bExpansionState;

};

//////////////////////////////////////////////////////////////////////////
// FScriptExecutionBlueprint

class KISMET_API FScriptExecutionBlueprint : public FScriptExecutionNode
{
public:

	/** Adds new blueprint instance node */
	void AddInstance(TSharedPtr<class FScriptExecutionNode> Instance) { Instances.Add(Instance); }

	/** Returns the current number of instance nodes */
	int32 GetInstanceCount() { return Instances.Num(); }

	/** Returns the instance node specified by Index */
	TSharedPtr<class FScriptExecutionNode> GetInstanceByIndex(const int32 Index) { return Instances[Index]; }

	/** Returns the instance that matches the supplied name if present */
	TSharedPtr<FScriptExecutionNode> GetInstanceByName(FName InstanceName);

	// ~Begin FScriptExecutionNode
	virtual bool IsRootNode() const override { return true; }
	virtual EScriptStatContainerType::Type GetStatisticContainerType() const override { return EScriptStatContainerType::Container; }
	virtual void RefreshStats(const FTracePath& TracePath) override;
	virtual void GetAllExecNodes(TMap<FName, TSharedPtr<FScriptExecutionNode>>& ExecNodesOut) override;
	// ~End FScriptExecutionNode

private:

	/** Exec nodes representing all instances based on this blueprint */
	TArray<TSharedPtr<class FScriptExecutionNode>> Instances;

};

//////////////////////////////////////////////////////////////////////////
// FScriptExecutionContext

class KISMET_API FScriptExecutionContext : public TSharedFromThis<FScriptExecutionContext>
{
public:

	bool IsContextValid() const { return ContextObject.IsValid(); }
	const class UEdGraphNode* GetGraphNode() const { return ContextObject.IsValid() ? Cast<UEdGraphNode>(ContextObject.Get()) : nullptr; }
	const class UEdGraphNode* GetNodeFromCodeLocation(const int32 CodeLocation, UFunction* FunctionOverride = nullptr) const;
	void GetAllExecNodes(TMap<FName, TSharedPtr<FScriptExecutionNode>>& ExecNodesOut);
	void UpdateConnectedStats();

public:

	TSharedPtr<FScriptExecutionNode> ExecutionNode;
	TWeakObjectPtr<const UObject> ContextObject;
	TWeakObjectPtr<const UObject> MacroContextObject;
	TWeakObjectPtr<UBlueprintGeneratedClass> BlueprintClass;
	TWeakObjectPtr<UFunction> BlueprintFunction;
};
