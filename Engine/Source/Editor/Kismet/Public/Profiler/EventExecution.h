// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TracePath.h"
#include "ScriptPerfData.h"

/**  Execution node flags */
namespace EScriptExecutionNodeFlags
{
	enum Type
	{
		None						= 0x00000000,	// No Flags
		Class						= 0x00000001,	// Class
		Instance					= 0x00000002,	// Instance
		Event						= 0x00000004,	// Event
		CustomEvent					= 0x00000008,	// Custom Event
		FunctionCall				= 0x00000010,	// Function Call
		MacroCall					= 0x00000020,	// Macro Call
		MacroNode					= 0x00000100,	// Macro Node
		ConditionalBranch			= 0x00000200,	// Node has multiple exit pins using a jump
		SequentialBranch			= 0x00000400,	// Node has multiple exit pins ran in sequence
		Node						= 0x00000800,	// Node timing
		ExecPin						= 0x00001000,	// Exec pin dummy node
		PureNode					= 0x00002000,	// Pure node (no exec pins)
		AsyncTask					= 0x00004000,	// Async task node
		AsyncTaskDelegate			= 0x00008000,	// Async task delegate pin/event
		FunctionTunnel				= 0x00010000,	// Tunnel function
		TunnelEntryPin				= 0x00020000,	// Tunnel entry pin
		TunnelExitPin				= 0x00040000,	// Tunnel exit pin
		ReEntrantTunnelPin			= 0x00080000,	// Re-Entrant tunnel pin
		TunnelFinalExitPin			= 0x00100000,	// Tunnel final exit or exec then pin
		PureChain					= 0x00200000,	// Pure node call chain
		CyclicLinkage				= 0x00400000,	// Marks execution path as cyclic.
		InvalidTrace				= 0x00800000,	// Indicates that node doesn't contain a valid script trace.
									// Groups
		CallSite					= FunctionCall|MacroCall|FunctionTunnel,
		BranchNode					= ConditionalBranch|SequentialBranch,
		TunnelPin					= TunnelEntryPin|TunnelExitPin,
		PureStats					= PureNode|PureChain,
		Container					= Class|Instance|Event|CustomEvent|TunnelPin|ExecPin|PureChain
	};
}

//////////////////////////////////////////////////////////////////////////
// Stat Container Type

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
		FLinearExecPath(TSharedPtr<class FScriptExecutionNode> NodeIn, const FTracePath TracePathIn, const bool bIncludeChildrenIn = false)
			: TracePath(TracePathIn)
			, LinkedNode(NodeIn)
			, bIncludeChildren(bIncludeChildrenIn)
		{
		}

		FTracePath TracePath;
		TSharedPtr<class FScriptExecutionNode> LinkedNode;
		bool bIncludeChildren;
	};

	/** Returns the linked nodes map */
	TMap<int32, TSharedPtr<class FScriptExecutionNode>>& GetLinkedNodes() { return LinkedNodes; }

	/** Returns the number of linked nodes */
	int32 GetNumLinkedNodes() const { return LinkedNodes.Num(); }

	/** Add linked node */
	void AddLinkedNode(const int32 PinScriptOffset, TSharedPtr<class FScriptExecutionNode> LinkedNode);

	/** Returns linked node by matching script offset */
	TSharedPtr<class FScriptExecutionNode> GetLinkedNodeByScriptOffset(const int32 PinScriptOffset);

	/** Returns the child nodes map */
	TArray<TSharedPtr<class FScriptExecutionNode>>& GetChildNodes() { return ChildNodes; }

	/** Returns the filtered child nodes */
	virtual void GetFilteredChildNodes(TArray<FLinearExecPath>& ChildArrayInOut, const FTracePath& TracePath);

	/** Returns the number of children */
	int32 GetNumChildren() const { return ChildNodes.Num(); }

	/** Returns the child node for the specified index */
	TSharedPtr<class FScriptExecutionNode> GetChildByIndex(const int32 ChildIndex) { return ChildNodes[ChildIndex]; }

	/** Add child node */
	void AddChildNode(TSharedPtr<class FScriptExecutionNode> ChildNode) { ChildNodes.Add(ChildNode); }

protected:

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

	/** Returns a TSet containing all valid instance names */
	void GetValidInstanceNames(TSet<FName>& ValidInstances) const;

	/** Test whether or not perf data is available for the given instance/trace path */
	bool HasPerfDataForInstanceAndTracePath(FName InstanceName, const FTracePath& TracePath) const;

	/** Get or add perf data for instance and tracepath */
	TSharedPtr<class FScriptPerfData> GetOrAddPerfDataByInstanceAndTracePath(FName InstanceName, const FTracePath& TracePath);

	/** Get perf data for instance and tracepath */
	TSharedPtr<class FScriptPerfData> GetPerfDataByInstanceAndTracePath(FName InstanceName, const FTracePath& TracePath);

	/** Get all instance perf data for the trace path, excluding the global blueprint data */
	void GetInstancePerfDataByTracePath(const FTracePath& TracePath, TArray<TSharedPtr<FScriptPerfData>>& ResultsOut);

	/** Get or add global blueprint perf data for the trace path */
	TSharedPtr<FScriptPerfData> GetOrAddBlueprintPerfDataByTracePath(const FTracePath& TracePath);

	/** Get global blueprint perf data for the trace path */
	TSharedPtr<FScriptPerfData> GetBlueprintPerfDataByTracePath(const FTracePath& TracePath);

	/** Get global blueprint perf data for all trace paths */
	virtual void GetBlueprintPerfDataForAllTracePaths(FScriptPerfData& OutPerfData);

protected:

	/** Returns performance data type */
	virtual const EScriptPerfDataType GetPerfDataType() const { return EScriptPerfDataType::Node; }

protected:
	
	/** FScriptExeutionPath hash to perf data */
	TMap<FName, TMap<const uint32, TSharedPtr<FScriptPerfData>>> InstanceInputPinToPerfDataMap;

};

//////////////////////////////////////////////////////////////////////////
// FScriptExecutionNodeParams

struct KISMET_API FScriptExecNodeParams
{
	/** Node name */
	FName NodeName;
	/** Owning graph name */
	FName OwningGraphName;
	/** Node flags to describe the source graph node type */
	uint32 NodeFlags;
	/** Oberved object */
	TWeakObjectPtr<const UObject> ObservedObject;
	/** Observed pin */
	FEdGraphPinReference ObservedPin;
	/** Display name for widget UI */
	FText DisplayName;
	/** Tooltip for widget UI */
	FText Tooltip;
	/** Icon color for widget UI */
	FLinearColor IconColor;
	/** Icon for widget UI */
	FSlateBrush* Icon;
};

//////////////////////////////////////////////////////////////////////////
// FScriptExecutionHottestPathParams

struct FScriptExecutionHottestPathParams
{
	FScriptExecutionHottestPathParams(const FName InstanceNameIn, const float EventTimingIn)
		: InstanceName(InstanceNameIn)
		, EventTiming(EventTimingIn)
		, TimeSoFar(0.0)
	{
	}

	/** Instance name */
	const FName InstanceName;
	/** Event total timing */
	const double EventTiming;
	/** Event time so far */
	double TimeSoFar;
	/** Current tracepath */
	FTracePath TracePath;
};

//////////////////////////////////////////////////////////////////////////
// FScriptExecutionNode

class KISMET_API FScriptExecutionNode : public FScriptNodeExecLinkage, public FScriptNodePerfData, public TSharedFromThis<FScriptExecutionNode>
{
public:

	FScriptExecutionNode();
	FScriptExecutionNode(const FScriptExecNodeParams& InitParams);
    virtual ~FScriptExecutionNode() {}

	bool operator == (const FScriptExecutionNode& NodeIn) const;

	/** Get the node's name */
	FName GetName() const { return NodeName; }

	/** Returns the owning graph name */
	FName GetGraphName() const { return OwningGraphName; }

	/** Add to the node's flags */
	void AddFlags(const uint32 NewFlags) { NodeFlags |= NewFlags; }

	/** Remove from the node's flags */
	void RemoveFlags(const uint32 NewFlags) { NodeFlags &= ~NewFlags; }

	/** Does the node contain these flags */
	bool HasFlags(const uint32 Flags) const { return (NodeFlags & Flags) != 0U; }

	/** Returns if this exec node can be safely cyclicly linked */
	bool IsCyclicNode() const { return (NodeFlags & EScriptExecutionNodeFlags::CallSite) == 0U; }

	/** Returns if this exec event represents a change in class/blueprint */
	bool IsClass() const { return (NodeFlags & EScriptExecutionNodeFlags::Class) != 0U; }

	/** Returns if this exec event represents a change in instance */
	bool IsInstance() const { return (NodeFlags & EScriptExecutionNodeFlags::Instance) != 0U; }

	/** Returns if this exec event represents the start of an event execution path */
	bool IsEvent() const { return (NodeFlags & EScriptExecutionNodeFlags::Event) != 0U; }

	/** Returns if this exec event represents the start of a custon event execution path */
	bool IsCustomEvent() const { return (NodeFlags & EScriptExecutionNodeFlags::CustomEvent) != 0U; }
	
	/** Returns if this event is a function callsite event */
	bool IsFunctionCallSite() const { return (NodeFlags & EScriptExecutionNodeFlags::FunctionCall) != 0U; }

	/** Returns if this event is a macro callsite event */
	bool IsMacroCallSite() const { return (NodeFlags & EScriptExecutionNodeFlags::MacroCall) != 0U; }

	/** Returns if this event happened inside a macro instance */
	bool IsMacroNode() const { return (NodeFlags & EScriptExecutionNodeFlags::MacroNode) != 0U; }

	/** Returns if this node is also a pure node (i.e. no exec input pin) */
	bool IsPureNode() const { return (NodeFlags & EScriptExecutionNodeFlags::PureNode) != 0U; }

	/** Returns if this node is a pure chain node */
	bool IsPureChain() const { return (NodeFlags & EScriptExecutionNodeFlags::PureChain) != 0U; }

	/** Returns if this event potentially multiple exit sites */
	bool IsBranch() const { return (NodeFlags & EScriptExecutionNodeFlags::BranchNode) != 0U; }

	/** Gets the observed object context */
	const UObject* GetObservedObject() { return ObservedObject.Get(); }

	/** Gets the observed object context */
	const UEdGraphPin* GetObservedPin() const { return ObservedPin.Get(); }

	/** Navigate to object */
	virtual void NavigateToObject() const;

	/** Returns the display name for widget UI */
	const FText& GetDisplayName() const { return DisplayName; }

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

	/** Returns the current expansion state for widget UI */
	bool IsExpanded() const { return bExpansionState; }

	/** Sets the current expansion state for widget UI */
	void SetExpanded(bool bIsExpanded) { bExpansionState = bIsExpanded; }

	/** Returns the pure chain node associated with this exec node (if one exists) */
	TSharedPtr<FScriptExecutionNode> GetPureChainNode();

	/** Returns pure node script code range */
	FInt32Range GetPureNodeScriptCodeRange() const { return PureNodeScriptCodeRange; }

	/** Sets the pure node script code range */
	void SetPureNodeScriptCodeRange(FInt32Range InScriptCodeRange) { PureNodeScriptCodeRange = InScriptCodeRange; }

	/** Return the linear execution path from this node */
	virtual void GetLinearExecutionPath(TArray<FLinearExecPath>& LinearExecutionNodes, const FTracePath& TracePath, const bool bIncludeChildren = false);

	/** Get statistic container type */
	virtual EScriptStatContainerType::Type GetStatisticContainerType() const;

	/** Refresh Stats */
	virtual void RefreshStats(const FTracePath& TracePath);

	/** Calculate Hottest Path Stats */
	virtual float CalculateHottestPathStats(FScriptExecutionHottestPathParams HotPathParams);

	/** Get all exec nodes */
	virtual void GetAllExecNodes(TMap<FName, TSharedPtr<FScriptExecutionNode>>& ExecNodesOut);

	/** Get all pure nodes associated with the given trace path */
	virtual void GetAllPureNodes(TMap<int32, TSharedPtr<FScriptExecutionNode>>& PureNodesOut);

	/** Creates and returns the slate icon widget */
	virtual TSharedRef<SWidget> GetIconWidget();

	/** Creates and returns the slate hyperlink widget */
	virtual TSharedRef<SWidget> GetHyperlinkWidget();

protected:

	/** Returns if the observed object is valid */
	virtual bool IsObservedObjectValid() const;

	/** Returns Tunnel Linear Execution Trace */
	void MapTunnelLinearExecution(FTracePath& TraceInOut) const;

	/** Get all pure nodes - private implementation */
	void GetAllPureNodes_Internal(TMap<int32, TSharedPtr<FScriptExecutionNode>>& PureNodesOut, const FInt32Range& ScriptCodeRange);

	// FScriptNodePerfData
	virtual const EScriptPerfDataType GetPerfDataType() const override;
	// ~FScriptNodePerfData

protected:

	/** Node name */
	FName NodeName;
	/** Owning graph name */
	FName OwningGraphName;
	/** Node flags to describe the source graph node type */
	uint32 NodeFlags;
	/** Observed object */
	TWeakObjectPtr<const UObject> ObservedObject;
	/** Observed pin */
	FEdGraphPinReference ObservedPin;
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
	/** Script code range for pure node linkage */
	FInt32Range PureNodeScriptCodeRange;
};

//////////////////////////////////////////////////////////////////////////
// FScriptExecutionTunnelInstance

class KISMET_API FScriptExecutionTunnelInstance : public FScriptExecutionNode
{
public:

	FScriptExecutionTunnelInstance(const FScriptExecNodeParams& InitParams)
		: FScriptExecutionNode(InitParams)
	{
	}

	// FScriptNodePerfData
	virtual void GetBlueprintPerfDataForAllTracePaths(FScriptPerfData& OutPerfData) override;
	// ~FScriptNodePerfData

	/** Adds a custom child entry point */
	void AddCustomEntryPoint(TSharedPtr<class FScriptExecutionTunnelEntry> CustomEntryPoint) { CustomEntryPoints.Add(CustomEntryPoint); }

private:

	/** List of custom entry points */
	TArray<TSharedPtr<class FScriptExecutionTunnelEntry>> CustomEntryPoints;

};

//////////////////////////////////////////////////////////////////////////
// FScriptExecutionTunnelEntry

class KISMET_API FScriptExecutionTunnelEntry : public FScriptExecutionNode
{
public:

	/** Tunnel execution type */
	enum ETunnelType
	{
		SimpleTunnel = 0,
		SinglePathTunnel,
		MultiIOTunnel
	};

	FScriptExecutionTunnelEntry(const FScriptExecNodeParams& InitParams, const class UK2Node_Tunnel* TunnelInstanceIn, const ETunnelType TypeIn)
		: FScriptExecutionNode(InitParams)
		, TunnelType(TypeIn)
		, TunnelInstance(TunnelInstanceIn)
	{
	}

	// FScriptExecutionNode
	virtual void GetLinearExecutionPath(TArray<FLinearExecPath>& LinearExecutionNodes, const FTracePath& TracePath, const bool bIncludeChildren = false) override;
	virtual void GetFilteredChildNodes(TArray<FLinearExecPath>& ChildArrayInOut, const FTracePath& TracePath) override;
	virtual void RefreshStats(const FTracePath& TracePath) override;
	// ~FScriptExecutionNode

	/** Add tunnel timing */
	void AddTunnelTiming(const FName InstanceName, const FTracePath& TracePath, const double Time);

	/** Update tunnel exit site */
	void UpdateTunnelExit(const FName InstanceName, const FTracePath& TracePath, const int32 ExitScriptOffset);

	/** Returns true if the script offset is valid for this tunnel */
	bool IsPathValidForTunnel(const int32 ScriptOffset) const;

	/** Returns the tunnel instance graph node */
	const ETunnelType GetTunnelType() const { return TunnelType; }

	/** Returns the tunnel instance graph node */
	const UK2Node_Tunnel* GetTunnelInstance() const { return TunnelInstance.Get(); }

	/** Adds a valid exit script offset */
	void AddExitSite(TSharedPtr<FScriptExecutionNode> ExitSite, const int32 ExitScriptOffset);

	/** Returns the exit point associated with the script offset */
	TSharedPtr<FScriptExecutionNode> GetExitSite(const int32 ScriptOffset) const;

	/** Returns if the exit point associated with the script offset is the final exit site */
	bool IsFinalExitSite(const int32 ScriptOffset) const;

	/** Builds exit sites for instance */
	void BuildExitBranches();

private:

	/** The tunnel type */
	ETunnelType TunnelType;
	/** The tunnel instance this node represents */
	TWeakObjectPtr<const UK2Node_Tunnel> TunnelInstance;
	/** List of valid exit scripts offsets */
	TMap<int32, TSharedPtr<FScriptExecutionNode>> ValidExitPoints;
	/** Branched exit sites */
	TArray<FLinearExecPath> BranchedExitSites;

};

//////////////////////////////////////////////////////////////////////////
// FScriptExecutionTunnelExit

class KISMET_API FScriptExecutionTunnelExit : public FScriptExecutionNode
{
public:

	FScriptExecutionTunnelExit(const FScriptExecNodeParams& InitParams)
		: FScriptExecutionNode(InitParams)
	{
	}

	// FScriptExecutionNode
	virtual void GetLinearExecutionPath(TArray<FLinearExecPath>& LinearExecutionNodes, const FTracePath& TracePath, const bool bIncludeChildren = false) override;
	virtual void RefreshStats(const FTracePath& TracePath) override;
	virtual void NavigateToObject() const;
	// ~FScriptExecutionNode

};

//////////////////////////////////////////////////////////////////////////
// FScriptExecutionPureNode

class KISMET_API FScriptExecutionPureNode : public FScriptExecutionNode
{
public:

	FScriptExecutionPureNode(const FScriptExecNodeParams& InitParams)
		: FScriptExecutionNode(InitParams)
	{
	}

	// FScriptExecutionNode
	virtual void GetLinearExecutionPath(TArray<FLinearExecPath>& LinearExecutionNodes, const FTracePath& TracePath, const bool bIncludeChildren = false) override;
	virtual void RefreshStats(const FTracePath& TracePath) override;
	// ~FScriptExecutionNode

};

//////////////////////////////////////////////////////////////////////////
// FScriptExecutionInstance

class KISMET_API FScriptExecutionInstance : public FScriptExecutionNode
{
public:

	FScriptExecutionInstance(const FScriptExecNodeParams& InitParams)
		: FScriptExecutionNode(InitParams)
	{
	}

	// FScriptExecutionNode
	virtual void NavigateToObject() const override;
	virtual TSharedRef<SWidget> GetIconWidget() override;
	virtual TSharedRef<SWidget> GetHyperlinkWidget() override;
	virtual bool IsObservedObjectValid() const override;
	// ~FScriptExecutionNode

	/** Sets the current PIE Instance */
	void SetPIEInstance(const UObject* PIEInstanceIn) { PIEInstance = PIEInstanceIn; }

private:

	/** Return the current instance icon color */
	FSlateColor GetInstanceIconColor() const;

	/** Return the current instance icon color */
	FText GetInstanceTooltip() const;

private:

	/** PIE instance */
	TWeakObjectPtr<const UObject> PIEInstance;

};

//////////////////////////////////////////////////////////////////////////
// FScriptExecutionBlueprint

class KISMET_API FScriptExecutionBlueprint : public FScriptExecutionNode
{
public:

	FScriptExecutionBlueprint(const FScriptExecNodeParams& InitParams)
		: FScriptExecutionNode(InitParams)
	{
	}

	/** Adds new blueprint instance node */
	void AddInstance(TSharedPtr<FScriptExecutionNode> Instance) { Instances.Add(Instance); }

	/** Returns the current number of instance nodes */
	int32 GetInstanceCount() { return Instances.Num(); }

	/** Returns the instance node specified by Index */
	TSharedPtr<FScriptExecutionNode> GetInstanceByIndex(const int32 Index) { return Instances[Index]; }

	/** Returns the instance that matches the supplied name if present */
	TSharedPtr<FScriptExecutionNode> GetInstanceByName(FName InstanceName);

	// FScriptExecutionNode
	virtual EScriptStatContainerType::Type GetStatisticContainerType() const override { return EScriptStatContainerType::Container; }
	virtual void RefreshStats(const FTracePath& TracePath) override;
	virtual void GetAllExecNodes(TMap<FName, TSharedPtr<FScriptExecutionNode>>& ExecNodesOut) override;
	virtual void NavigateToObject() const override;
	// ~FScriptExecutionNode

private:

	/** Exec nodes representing all instances based on this blueprint */
	TArray<TSharedPtr<FScriptExecutionNode>> Instances;

};

