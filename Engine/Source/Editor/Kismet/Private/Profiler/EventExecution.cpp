// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"
#include "EventExecution.h"
#include "BlueprintProfilerModule.h"
#include "SHyperlink.h"

// This forces checking against correct tracepaths when querying data
#define STRICT_PERFDATA_CREATION 0

#define LOCTEXT_NAMESPACE "BlueprintEventUI"

//////////////////////////////////////////////////////////////////////////
// FScriptNodeExecLinkage

void FScriptNodeExecLinkage::AddLinkedNode(const int32 PinScriptOffset, TSharedPtr<class FScriptExecutionNode> LinkedNode)
{
	TSharedPtr<class FScriptExecutionNode>& Result = LinkedNodes.FindOrAdd(PinScriptOffset);
	Result = LinkedNode;
}

TSharedPtr<FScriptExecutionNode> FScriptNodeExecLinkage::GetLinkedNodeByScriptOffset(const int32 PinScriptOffset)
{
	TSharedPtr<FScriptExecutionNode> Result;
	if (TSharedPtr<FScriptExecutionNode>* SearchResult = LinkedNodes.Find(PinScriptOffset))
	{
		Result = *SearchResult;
	}
	return Result;
}

void FScriptNodeExecLinkage::GetFilteredChildNodes(TArray<FLinearExecPath>& ChildArrayInOut, const FTracePath& TracePath)
{
	for (auto Child : ChildNodes)
	{
		ChildArrayInOut.Add(FLinearExecPath(Child, TracePath));
	}
}

//////////////////////////////////////////////////////////////////////////
// FScriptNodePerfData

void FScriptNodePerfData::GetValidInstanceNames(TSet<FName>& ValidInstances) const
{
	for (auto InstanceIter : InstanceInputPinToPerfDataMap)
	{
		if (InstanceIter.Key != NAME_None)
		{
			ValidInstances.Add(InstanceIter.Key);
		}
	}
}

bool FScriptNodePerfData::HasPerfDataForInstanceAndTracePath(FName InstanceName, const FTracePath& TracePath) const
{
	const TMap<const uint32, TSharedPtr<FScriptPerfData>>* InstanceMapPtr = InstanceInputPinToPerfDataMap.Find(InstanceName);
	if (InstanceMapPtr)
	{
		return InstanceMapPtr->Contains(TracePath);
	}

	return false;
}

TSharedPtr<FScriptPerfData> FScriptNodePerfData::GetOrAddPerfDataByInstanceAndTracePath(FName InstanceName, const FTracePath& TracePath)
{ 
	TMap<const uint32, TSharedPtr<FScriptPerfData>>& InstanceMap = InstanceInputPinToPerfDataMap.FindOrAdd(InstanceName);
	TSharedPtr<FScriptPerfData>& Result = InstanceMap.FindOrAdd(TracePath);
	if (!Result.IsValid())
	{
		Result = MakeShareable<FScriptPerfData>(new FScriptPerfData(GetPerfDataType()));
	}
	return Result;
}

TSharedPtr<FScriptPerfData> FScriptNodePerfData::GetPerfDataByInstanceAndTracePath(FName InstanceName, const FTracePath& TracePath)
{ 
	TMap<const uint32, TSharedPtr<FScriptPerfData>>& InstanceMap = InstanceInputPinToPerfDataMap.FindOrAdd(InstanceName);
	#if STRICT_PERFDATA_CREATION
	TSharedPtr<FScriptPerfData>* Result = InstanceMap.Find(TracePath);
	check (Result != nullptr);
	return *Result;
	#else
	TSharedPtr<FScriptPerfData>& Result = InstanceMap.FindOrAdd(TracePath);
	if (!Result.IsValid())
	{
		Result = MakeShareable<FScriptPerfData>(new FScriptPerfData(GetPerfDataType()));
	}
	return Result;
	#endif
}

void FScriptNodePerfData::GetInstancePerfDataByTracePath(const FTracePath& TracePath, TArray<TSharedPtr<FScriptPerfData>>& ResultsOut)
{
	for (auto InstanceIter : InstanceInputPinToPerfDataMap)
	{
		if (InstanceIter.Key != NAME_None)
		{
			TSharedPtr<FScriptPerfData> Result = InstanceIter.Value.FindOrAdd(TracePath);
			if (Result.IsValid())
			{
				ResultsOut.Add(Result);
			}
		}
	}
}

TSharedPtr<FScriptPerfData> FScriptNodePerfData::GetOrAddBlueprintPerfDataByTracePath(const FTracePath& TracePath)
{
	TMap<const uint32, TSharedPtr<FScriptPerfData>>& BlueprintMap = InstanceInputPinToPerfDataMap.FindOrAdd(NAME_None);
	TSharedPtr<FScriptPerfData>& Result = BlueprintMap.FindOrAdd(TracePath);
	if (!Result.IsValid())
	{
		Result = MakeShareable<FScriptPerfData>(new FScriptPerfData(GetPerfDataType()));
	}
	return Result;
}

TSharedPtr<FScriptPerfData> FScriptNodePerfData::GetBlueprintPerfDataByTracePath(const FTracePath& TracePath)
{
	#if STRICT_PERFDATA_CREATION
	TMap<const uint32, TSharedPtr<FScriptPerfData>>& BlueprintMap = InstanceInputPinToPerfDataMap.FindOrAdd(NAME_None);
	TSharedPtr<FScriptPerfData>* Result = BlueprintMap.Find(TracePath);
	check (Result != nullptr);
	return *Result;
	#else
	TMap<const uint32, TSharedPtr<FScriptPerfData>>& BlueprintMap = InstanceInputPinToPerfDataMap.FindOrAdd(NAME_None);
	TSharedPtr<FScriptPerfData>& Result = BlueprintMap.FindOrAdd(TracePath);
	if (!Result.IsValid())
	{
		Result = MakeShareable<FScriptPerfData>(new FScriptPerfData(GetPerfDataType()));
	}
	return Result;
	#endif
}

void FScriptNodePerfData::GetBlueprintPerfDataForAllTracePaths(FScriptPerfData& OutPerfData)
{
	OutPerfData.Reset();

	TMap<const uint32, TSharedPtr<FScriptPerfData>>& BlueprintMap = InstanceInputPinToPerfDataMap.FindOrAdd(NAME_None);
	for (auto MapIt = BlueprintMap.CreateIterator(); MapIt; ++MapIt)
	{
		TSharedPtr<FScriptPerfData> CurDataPtr = MapIt.Value();
		if (CurDataPtr.IsValid())
		{
			OutPerfData.AddData(*CurDataPtr);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// FScriptExecutionNode

FScriptExecutionNode::FScriptExecutionNode()
	: NodeFlags(EScriptExecutionNodeFlags::None)
	, bExpansionState(false)
{
}

FScriptExecutionNode::FScriptExecutionNode(const FScriptExecNodeParams& InitParams)
	: NodeName(InitParams.NodeName)
	, OwningGraphName(InitParams.OwningGraphName)
	, NodeFlags(InitParams.NodeFlags)
	, ObservedObject(InitParams.ObservedObject)
	, ObservedPin(InitParams.ObservedPin)
	, DisplayName(InitParams.DisplayName)
	, Tooltip(InitParams.Tooltip)
	, IconColor(InitParams.IconColor)
	, Icon(InitParams.Icon)
	, bExpansionState(false)
{
}

bool FScriptExecutionNode::operator == (const FScriptExecutionNode& NodeIn) const
{
	return NodeName == NodeIn.NodeName;
}

TSharedRef<SWidget> FScriptExecutionNode::GetIconWidget()
{
	return SNew(SImage)
			.Image(Icon)
			.ColorAndOpacity(IconColor);
}

TSharedRef<SWidget> FScriptExecutionNode::GetHyperlinkWidget()
{
	return SNew(SHyperlink)
		#if TRACEPATH_DEBUG
		.Text(FText::FromName(NodeName))
		#else
		.Text(DisplayName)
		#endif
		.Style(FEditorStyle::Get(), "HoverOnlyHyperlink")
		.ToolTipText(Tooltip)
		.OnNavigate(this, &FScriptExecutionNode::NavigateToObject);
}

void FScriptExecutionNode::GetLinearExecutionPath(TArray<FLinearExecPath>& LinearExecutionNodes, const FTracePath& TracePath, const bool bIncludeChildren)
{
	LinearExecutionNodes.Add(FLinearExecPath(AsShared(), TracePath));
	if (bIncludeChildren)
	{
		for (auto Child : ChildNodes)
		{
			FTracePath ChildTracePath(TracePath);
			Child->GetLinearExecutionPath(LinearExecutionNodes, ChildTracePath, bIncludeChildren);
		}
	}
	if (bIncludeChildren || GetNumLinkedNodes() == 1)
	{
		for (auto NodeIter : LinkedNodes)
		{
			if (HasFlags(EScriptExecutionNodeFlags::PureStats))
			{
				continue;
			}
			else
			{
				FTracePath NewTracePath(TracePath);
				if (NodeIter.Value->HasFlags(EScriptExecutionNodeFlags::AsyncTaskDelegate))
				{
					NewTracePath.ResetPath();
				}
				if (NodeIter.Key != INDEX_NONE && !HasFlags(EScriptExecutionNodeFlags::InvalidTrace))
				{
					NewTracePath.AddExitPin(NodeIter.Key);
				}
				NodeIter.Value->GetLinearExecutionPath(LinearExecutionNodes, NewTracePath, bIncludeChildren);
			}
		}
	}
}

void FScriptExecutionNode::MapTunnelLinearExecution(FTracePath& Trace) const
{
	if (HasFlags(EScriptExecutionNodeFlags::FunctionTunnel))
	{
		for (auto ChildIter : ChildNodes)
		{
			ChildIter->MapTunnelLinearExecution(Trace);
		}
	}
	else if (GetNumLinkedNodes() == 1)
	{
		for (auto NodeIter : LinkedNodes)
		{
			if (!NodeIter.Value->HasFlags(EScriptExecutionNodeFlags::InvalidTrace))
			{
				Trace.AddExitPin(NodeIter.Key);
			}
			NodeIter.Value->MapTunnelLinearExecution(Trace);
		}
	}
}

EScriptStatContainerType::Type FScriptExecutionNode::GetStatisticContainerType() const
{
	EScriptStatContainerType::Type Result = EScriptStatContainerType::Standard;
	if (HasFlags(EScriptExecutionNodeFlags::ExecPin))
	{
		Result = EScriptStatContainerType::NewExecutionPath;
	}
	else if (HasFlags(EScriptExecutionNodeFlags::Container))
	{
		Result = EScriptStatContainerType::Container;
	}
	else if (HasFlags(EScriptExecutionNodeFlags::SequentialBranch))
	{
		Result = EScriptStatContainerType::SequentialBranch;
	}
	return Result;
}

void FScriptExecutionNode::RefreshStats(const FTracePath& TracePath)
{
	// Process stat update
	bool bRefreshChildStats = false;
	bool bRefreshLinkStats = true;
	TArray<TSharedPtr<FScriptPerfData>> BlueprintPooledStats;
	// Update stats based on type
	switch(GetStatisticContainerType())
	{
		case EScriptStatContainerType::Container:
		{
			GetInstancePerfDataByTracePath(TracePath, BlueprintPooledStats);
			bRefreshChildStats = true;
			break;
		}
		case EScriptStatContainerType::Standard:
		case EScriptStatContainerType::SequentialBranch:
		{
			GetInstancePerfDataByTracePath(TracePath, BlueprintPooledStats);
			break;
		}
		case EScriptStatContainerType::NewExecutionPath:
		{
			// Refresh child stats and copy as branch stats - this is a dummy entry
			TSet<FName> AllInstances;
			TArray<FLinearExecPath> ChildExecPaths;
			for (auto ChildIter : ChildNodes)
			{
				ChildIter->GetValidInstanceNames(AllInstances);
				ChildIter->RefreshStats(TracePath);
				ChildIter->GetLinearExecutionPath(ChildExecPaths, TracePath, false);
			}
			for (auto InstanceIter : AllInstances)
			{
				if (InstanceIter != NAME_None)
				{
					TSharedPtr<FScriptPerfData> InstancePerfData = GetPerfDataByInstanceAndTracePath(InstanceIter, TracePath);
					InstancePerfData->Reset();
					BlueprintPooledStats.Add(InstancePerfData);

					for (auto ChildIter : ChildExecPaths)
					{
						TSharedPtr<FScriptPerfData> ChildPerfData = ChildIter.LinkedNode->GetPerfDataByInstanceAndTracePath(InstanceIter, ChildIter.TracePath);
						InstancePerfData->AddBranchData(*ChildPerfData.Get());
					}
				}
			}
			// Update Blueprint stats as branches
			TSharedPtr<FScriptPerfData> BlueprintData = GetOrAddBlueprintPerfDataByTracePath(TracePath);
			BlueprintData->Reset();

			for (auto BlueprintChildDataIter : BlueprintPooledStats)
			{
				BlueprintData->AddBranchData(*BlueprintChildDataIter.Get());
			}
			BlueprintPooledStats.Reset(0);
			bRefreshLinkStats = false;
			break;
		}
	}
	// Refresh Child Links
	if (bRefreshChildStats)
	{
		for (auto ChildIter : ChildNodes)
		{
			ChildIter->RefreshStats(TracePath);
		}
	}
	// Refresh All links
	if (bRefreshLinkStats)
	{
		for (auto LinkIter : LinkedNodes)
		{
			FTracePath LinkTracePath(TracePath);
			if (LinkIter.Value->HasFlags(EScriptExecutionNodeFlags::AsyncTaskDelegate))
			{
				LinkTracePath.ResetPath();
			}
			if (!LinkIter.Value->HasFlags(EScriptExecutionNodeFlags::InvalidTrace))
			{
				LinkTracePath.AddExitPin(LinkIter.Key);
			}
			LinkIter.Value->RefreshStats(LinkTracePath);
		}
	}
	// Update the owning blueprint stats
	if (BlueprintPooledStats.Num() > 0)
	{
		TSharedPtr<FScriptPerfData> BlueprintData = GetOrAddBlueprintPerfDataByTracePath(TracePath);
		BlueprintData->InitialiseFromDataSet(BlueprintPooledStats);
	}
}

float FScriptExecutionNode::CalculateHottestPathStats(FScriptExecutionHottestPathParams HotPathParams)
{
	// Grab local perf data
	TSharedPtr<FScriptPerfData> LocalPerfData = GetOrAddPerfDataByInstanceAndTracePath(HotPathParams.InstanceName, HotPathParams.TracePath);
	float AccumulatedTime = 0.f;

	if (!IsEvent())
	{
		// Subtract local inclusive cost from parent inclusive cost
		const double NodeTime = IsBranch() ? LocalPerfData->GetExclusiveTiming() : LocalPerfData->GetInclusiveTiming();
		HotPathParams.TimeSoFar += NodeTime;
		AccumulatedTime += NodeTime;
		const float HottestEndpointValue = static_cast<float>(HotPathParams.TimeSoFar / HotPathParams.EventTiming);
		LocalPerfData->SetHottestEndpointHeatLevel(HottestEndpointValue);
	}
	// Update children
	for (auto ChildIter : ChildNodes)
	{
		AccumulatedTime += ChildIter->CalculateHottestPathStats(HotPathParams);
	}
	// Update Links
	for (auto LinkIter : LinkedNodes)
	{
		FScriptExecutionHottestPathParams LinkedHotPathParams(HotPathParams);
		if (!LinkIter.Value->HasFlags(EScriptExecutionNodeFlags::InvalidTrace))
		{
			LinkedHotPathParams.TracePath.AddExitPin(LinkIter.Key);
		}
		AccumulatedTime += LinkIter.Value->CalculateHottestPathStats(LinkedHotPathParams);
	}
	const float HottestPathValue = static_cast<float>(AccumulatedTime / HotPathParams.EventTiming);
	LocalPerfData->SetHottestPathHeatLevel(HottestPathValue);

	return AccumulatedTime;
}

void FScriptExecutionNode::GetAllExecNodes(TMap<FName, TSharedPtr<FScriptExecutionNode>>& ExecNodesOut)
{
	for (auto ChildIter : ChildNodes)
	{
		ChildIter->GetAllExecNodes(ExecNodesOut);
		ExecNodesOut.Add(ChildIter->GetName()) = ChildIter;
	}
	for (auto LinkIter : LinkedNodes)
	{
		LinkIter.Value->GetAllExecNodes(ExecNodesOut);
		ExecNodesOut.Add(LinkIter.Value->GetName()) = LinkIter.Value;
	}
}

void FScriptExecutionNode::GetAllPureNodes(TMap<int32, TSharedPtr<class FScriptExecutionNode>>& PureNodesOut)
{
	GetAllPureNodes_Internal(PureNodesOut, PureNodeScriptCodeRange);

	// Sort pure nodes by script offset (execution order).
	PureNodesOut.KeySort(TLess<int32>());
}

void FScriptExecutionNode::GetAllPureNodes_Internal(TMap<int32, TSharedPtr<class FScriptExecutionNode>>& PureNodesOut, const FInt32Range& ScriptCodeRange)
{
	for (auto LinkIter : LinkedNodes)
	{
		LinkIter.Value->GetAllPureNodes_Internal(PureNodesOut, ScriptCodeRange);

		if (LinkIter.Value->IsPureNode() && ScriptCodeRange.Contains(LinkIter.Key))
		{
			PureNodesOut.Add(LinkIter.Key, LinkIter.Value);
		}
	}
}

TSharedPtr<FScriptExecutionNode> FScriptExecutionNode::GetPureChainNode()
{
	TSharedPtr<FScriptExecutionNode> Result;

	for (auto ChildIter : ChildNodes)
	{
		if (ChildIter->IsPureChain())
		{
			Result = ChildIter;
		}
	}

	return Result;
}

void FScriptExecutionNode::NavigateToObject() const
{
	if (IsObservedObjectValid())
	{
		if (UEdGraphPin* Pin = ObservedPin.Get())
		{
			FKismetEditorUtilities::BringKismetToFocusAttentionOnPin(Pin);
		}
		else
		{
			FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(ObservedObject.Get());
		}
	}
}

bool FScriptExecutionNode::IsObservedObjectValid() const
{
	return ObservedObject.IsValid() && !ObservedObject.Get()->IsPendingKill();
}

const EScriptPerfDataType FScriptExecutionNode::GetPerfDataType() const
{
	return IsEvent() ? EScriptPerfDataType::Event : EScriptPerfDataType::Node;
}

//////////////////////////////////////////////////////////////////////////
// FScriptExecutionTunnelInstance

void FScriptExecutionTunnelInstance::GetBlueprintPerfDataForAllTracePaths(FScriptPerfData& OutPerfData)
{
	OutPerfData.Reset();

	for (auto CustomEntryPoint : CustomEntryPoints)
	{
		CustomEntryPoint->GetBlueprintPerfDataForAllTracePaths(OutPerfData);
	}
}

//////////////////////////////////////////////////////////////////////////
// FScriptExecutionTunnelEntry

void FScriptExecutionTunnelEntry::GetLinearExecutionPath(TArray<FLinearExecPath>& LinearExecutionNodes, const FTracePath& TracePath, const bool bIncludeChildren)
{
	LinearExecutionNodes.Add(FLinearExecPath(AsShared(), TracePath));
	if (TunnelType != MultiIOTunnel)
	{
		FTracePath TunnelTracePath(TracePath, SharedThis<FScriptExecutionTunnelEntry>(this));
		for (auto TunnelExit : LinkedNodes)
		{
			if (IsPathValidForTunnel(TunnelExit.Key))
			{
				TunnelTracePath.AddExitPin(TunnelExit.Key);
				TunnelExit.Value->GetLinearExecutionPath(LinearExecutionNodes, TunnelTracePath);
			}
		}
	}
}

void FScriptExecutionTunnelEntry::GetFilteredChildNodes(TArray<FLinearExecPath>& ChildArrayInOut, const FTracePath& TracePath)
{
	ChildArrayInOut.Reset();
	switch(TunnelType)
	{
		case SimpleTunnel:
		{
			FTracePath ChildTrace(TracePath, SharedThis<FScriptExecutionTunnelEntry>(this));
			for (auto Child : ChildNodes)
			{
				if (Child->HasFlags(EScriptExecutionNodeFlags::TunnelEntryPin))
				{
					for (auto NodeToAdd : Child->GetChildNodes())
					{
						FTracePath NewTrace(ChildTrace);
						ChildArrayInOut.Add(FLinearExecPath(NodeToAdd, NewTrace));
					}
				}
				else if (!Child->HasFlags(EScriptExecutionNodeFlags::TunnelPin))
				{
					ChildArrayInOut.Add(FLinearExecPath(Child, ChildTrace));
				}
			}
			break;
		}
		case SinglePathTunnel:
		{
			FTracePath ChildTrace(TracePath, SharedThis<FScriptExecutionTunnelEntry>(this));
			for (auto Child : ChildNodes)
			{
				ChildArrayInOut.Add(FLinearExecPath(Child, ChildTrace));
			}
			break;
		}
		case MultiIOTunnel:
		{
			TArray<FLinearExecPath> Results;
			FTracePath ChildTrace(TracePath, SharedThis<FScriptExecutionTunnelEntry>(this));
			for (auto Child : ChildNodes)
			{
				const bool bIgnoreChildren = Child->HasFlags(EScriptExecutionNodeFlags::TunnelExitPin);
				ChildArrayInOut.Add(FLinearExecPath(Child, ChildTrace, bIgnoreChildren));
				Child->GetLinearExecutionPath(Results, ChildTrace, true);
			}
			for (auto Node : Results)
			{
				if (Node.LinkedNode->HasFlags(EScriptExecutionNodeFlags::TunnelExitPin))
				{
					ChildArrayInOut.Add(FLinearExecPath(Node.LinkedNode, TracePath, true));
				}
			}
			break;
		}
	}
}

void FScriptExecutionTunnelEntry::AddTunnelTiming(const FName InstanceName, const FTracePath& TracePath, const double Time)
{
	TSharedPtr<FScriptPerfData> PerfData = GetPerfDataByInstanceAndTracePath(InstanceName, TracePath);
	PerfData->AddEventTiming(Time);
	for (auto EntrySite : ChildNodes)
	{
		PerfData = EntrySite->GetPerfDataByInstanceAndTracePath(InstanceName, TracePath);
		PerfData->TickSamples();
	}
}

void FScriptExecutionTunnelEntry::UpdateTunnelExit(const FName InstanceName, const FTracePath& TracePath, const int32 ExitScriptOffset)
{
	// Tick exit site
	TSharedPtr<FScriptExecutionNode> TunnelExitNode = GetExitSite(ExitScriptOffset);
	if (TunnelExitNode.IsValid())
	{
		TSharedPtr<FScriptPerfData> PerfData = TunnelExitNode->GetPerfDataByInstanceAndTracePath(InstanceName, TracePath);
		PerfData->TickSamples();
	}
}

bool FScriptExecutionTunnelEntry::IsPathValidForTunnel(const int32 ScriptOffset) const
{
	return ValidExitPoints.Contains(ScriptOffset);
}

void FScriptExecutionTunnelEntry::AddExitSite(TSharedPtr<FScriptExecutionNode> ExitSite, const int32 ExitScriptOffset)
{
	ValidExitPoints.FindOrAdd(ExitScriptOffset) = ExitSite;
}

TSharedPtr<FScriptExecutionNode> FScriptExecutionTunnelEntry::GetExitSite(const int32 ScriptOffset) const
{
	TSharedPtr<FScriptExecutionNode> ExitSite;
	if (const TSharedPtr<FScriptExecutionNode>* SearchResult = ValidExitPoints.Find(ScriptOffset))
	{
		ExitSite = *SearchResult;
	}
	return ExitSite;
}

bool FScriptExecutionTunnelEntry::IsFinalExitSite(const int32 ScriptOffset) const
{
	bool bResult = false;
	if (const TSharedPtr<FScriptExecutionNode>* SearchResult = ValidExitPoints.Find(ScriptOffset))
	{
		bResult = (*SearchResult)->HasFlags(EScriptExecutionNodeFlags::TunnelFinalExitPin);
	}
	return bResult;
}

void FScriptExecutionTunnelEntry::RefreshStats(const FTracePath& TracePath)
{
	// Process stat update
	TArray<TSharedPtr<FScriptPerfData>> BlueprintPooledStats;
	// Update stats based on type
	GetInstancePerfDataByTracePath(TracePath, BlueprintPooledStats);
	// Create new trace
	FTracePath TunnelTracePath(TracePath, SharedThis<FScriptExecutionTunnelEntry>(this));
	// Refresh Child Links
	for (auto ChildIter : ChildNodes)
	{
		ChildIter->RefreshStats(TunnelTracePath);
	}
	// Refresh All links
	for (auto LinkIter : LinkedNodes)
	{
		FTracePath LinkTracePath(TracePath);
		LinkTracePath.AddExitPin(LinkIter.Key);
		LinkIter.Value->RefreshStats(LinkTracePath);
	}
	// Update the owning blueprint stats
	if (BlueprintPooledStats.Num() > 0)
	{
		TSharedPtr<FScriptPerfData> BlueprintData = GetBlueprintPerfDataByTracePath(TracePath);
		BlueprintData->InitialiseFromDataSet(BlueprintPooledStats);
	}
}

void FScriptExecutionTunnelEntry::BuildExitBranches()
{
	if (TunnelType == ETunnelType::MultiIOTunnel)
	{
		BranchedExitSites.Reset();
		TArray<FLinearExecPath> Results;
		FTracePath TracePath(SharedThis<FScriptExecutionTunnelEntry>(this));
		for (auto Child : ChildNodes)
		{
			Child->GetLinearExecutionPath(Results, TracePath, true);
		}
		for (auto Node : Results)
		{
			if (Node.LinkedNode->HasFlags(EScriptExecutionNodeFlags::TunnelExitPin))
			{
				BranchedExitSites.Add(Node);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// FScriptExecutionTunnelExit

void FScriptExecutionTunnelExit::GetLinearExecutionPath(TArray<FLinearExecPath>& LinearExecutionNodes, const FTracePath& TracePath, const bool bIncludeExitLinks)
{
	TSharedPtr<const FScriptExecutionTunnelEntry> ActiveTunnel = TracePath.GetTunnel();
	if (ActiveTunnel.IsValid())
	{
		if (ActiveTunnel->GetTunnelType() != FScriptExecutionTunnelEntry::SimpleTunnel)
		{
			LinearExecutionNodes.Add(FLinearExecPath(AsShared(), TracePath));
			FTracePath NewTracePath(TracePath);
			for (auto TunnelExit : LinkedNodes)
			{
				if (bIncludeExitLinks && ActiveTunnel->IsPathValidForTunnel(TunnelExit.Key))
				{
					NewTracePath.AddExitPin(TunnelExit.Key);
					TunnelExit.Value->GetLinearExecutionPath(LinearExecutionNodes, NewTracePath);
				}
			}
		}
	}
}

void FScriptExecutionTunnelExit::RefreshStats(const FTracePath& TracePath)
{
	// Process stat update
	TArray<TSharedPtr<FScriptPerfData>> BlueprintPooledStats;
	TSharedPtr<const FScriptExecutionTunnelEntry> TunnelNode = TracePath.GetTunnel();
	check (TunnelNode.IsValid());
	// Update stats based on type
	GetInstancePerfDataByTracePath(TracePath, BlueprintPooledStats);
	// Refresh all valid tunnel links
	for (auto LinkIter : LinkedNodes)
	{
		if (TunnelNode->IsPathValidForTunnel(LinkIter.Key))
		{
			FTracePath LinkTracePath(TracePath);
			if (LinkIter.Value->HasFlags(EScriptExecutionNodeFlags::AsyncTaskDelegate))
			{
				LinkTracePath.ResetPath();
			}
			if (!LinkIter.Value->HasFlags(EScriptExecutionNodeFlags::InvalidTrace))
			{
				LinkTracePath.AddExitPin(LinkIter.Key);
			}
			LinkIter.Value->RefreshStats(LinkTracePath);
		}
	}
	// Update the owning blueprint stats
	if (BlueprintPooledStats.Num() > 0)
	{
		TSharedPtr<FScriptPerfData> BlueprintData = GetOrAddBlueprintPerfDataByTracePath(TracePath);
		BlueprintData->InitialiseFromDataSet(BlueprintPooledStats);
	}
}

void FScriptExecutionTunnelExit::NavigateToObject() const
{
	if (IsObservedObjectValid())
	{
		FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(ObservedObject.Get());
	}
}

//////////////////////////////////////////////////////////////////////////
// FScriptExecutionPureNode

void FScriptExecutionPureNode::GetLinearExecutionPath(TArray<FLinearExecPath>& LinearExecutionNodes, const FTracePath& TracePath, const bool bIncludeChildren)
{
	LinearExecutionNodes.Add(FLinearExecPath(AsShared(), TracePath));
	FTracePath NewTracePath(TracePath);
	if (GetNumLinkedNodes() == 1)
	{
		for (auto NodeIter : LinkedNodes)
		{
			if (HasFlags(EScriptExecutionNodeFlags::PureStats))
			{
				continue;
			}
			else
			{
				if (NodeIter.Key != INDEX_NONE && !HasFlags(EScriptExecutionNodeFlags::InvalidTrace))
				{
					NewTracePath.AddExitPin(NodeIter.Key);
				}
				NodeIter.Value->GetLinearExecutionPath(LinearExecutionNodes, NewTracePath);
			}
		}
	}
}

void FScriptExecutionPureNode::RefreshStats(const FTracePath& TracePath)
{
	// Process stat update
	TArray<TSharedPtr<FScriptPerfData>> BlueprintPooledStats;
	// Refresh all pure nodes and accumulate for blueprint stat.
	if (HasFlags(EScriptExecutionNodeFlags::PureChain))
	{
		//GetInstancePerfDataByTracePath
		TSet<FName> ValidInstances;
		FTracePath PureTracePath(TracePath);
		TMap<int32, TSharedPtr<FScriptExecutionNode>> AllPureNodes;
		GetAllPureNodes(AllPureNodes);
		for (auto PureIter : AllPureNodes)
		{
			GetValidInstanceNames(ValidInstances);
			PureTracePath.AddExitPin(PureIter.Key);
			PureIter.Value->RefreshStats(PureTracePath);
		}
		for (auto InstanceName : ValidInstances)
		{
			TSharedPtr<FScriptPerfData> InstancePerfData = GetPerfDataByInstanceAndTracePath(InstanceName, TracePath);
			InstancePerfData->Reset();
			BlueprintPooledStats.Add(InstancePerfData);

			for (auto ChildIter : ChildNodes)
			{
				TSharedPtr<FScriptPerfData> ChildPerfData = ChildIter->GetPerfDataByInstanceAndTracePath(InstanceName, TracePath);
				InstancePerfData->AddBranchData(*ChildPerfData.Get());
			}
		}
	}
	// Grab all instance stats to accumulate into the blueprint stat.
	GetInstancePerfDataByTracePath(TracePath, BlueprintPooledStats);
	// Update the owning blueprint stats
	if (BlueprintPooledStats.Num() > 0)
	{
		TSharedPtr<FScriptPerfData> BlueprintData = GetBlueprintPerfDataByTracePath(TracePath);
		BlueprintData->InitialiseFromDataSet(BlueprintPooledStats);
	}
}

//////////////////////////////////////////////////////////////////////////
// FScriptExecutionBlueprint

TSharedPtr<FScriptExecutionNode> FScriptExecutionBlueprint::GetInstanceByName(FName InstanceName)
{
	TSharedPtr<FScriptExecutionNode> Result;
	for (auto Iter : Instances)
	{
		if (Iter->GetName() == InstanceName)
		{
			Result = Iter;
			break;
		}
	}
	return Result;
}

void FScriptExecutionBlueprint::RefreshStats(const FTracePath& TracePath)
{
	TArray<TSharedPtr<FScriptPerfData>> InstanceData;
	// Update event stats
	for (auto BlueprintEventIter : ChildNodes)
	{
		// This crawls through and updates all instance stats and pools the results into the blueprint node stats
		// as an overall blueprint performance representation.
		BlueprintEventIter->RefreshStats(TracePath);
	}
	// Determine if we require the hottest path's updating
	IBlueprintProfilerInterface& ProfilerModule = FModuleManager::LoadModuleChecked<IBlueprintProfilerInterface>("BlueprintProfiler");
	const bool bUpdateHottestPaths = ProfilerModule.GetWireHeatMapDisplayMode() != EBlueprintProfilerHeatMapDisplayMode::None;
	// Update instance stats
	for (auto InstanceIter : Instances)
	{
		TSharedPtr<FScriptPerfData> InstancePerfData = InstanceIter->GetOrAddPerfDataByInstanceAndTracePath(InstanceIter->GetName(), TracePath);
		InstancePerfData->Reset();
		// Update all top level instance stats now the events are up to date.
		for (auto InstanceEventIter : InstanceIter->GetChildNodes())
		{
			TSharedPtr<FScriptPerfData> InstanceEventPerfData = InstanceEventIter->GetOrAddPerfDataByInstanceAndTracePath(InstanceIter->GetName(), TracePath);
			InstancePerfData->AddData(*InstanceEventPerfData.Get());
			// Update the hottest path stats
			if (bUpdateHottestPaths)
			{
				FScriptExecutionHottestPathParams HotPathParams(InstanceIter->GetName(), InstanceEventPerfData->GetExclusiveTiming());
				InstanceEventIter->CalculateHottestPathStats(HotPathParams);
			}
		}
		// Add for consolidation at the bottom.
		InstanceData.Add(InstancePerfData);
	}
	// Finally... update the root stats for the owning blueprint.
	if (InstanceData.Num() > 0)
	{
		TSharedPtr<FScriptPerfData> BlueprintData = GetOrAddBlueprintPerfDataByTracePath(TracePath);
		BlueprintData->Reset();

		for (auto InstanceIter : InstanceData)
		{
			BlueprintData->AddData(*InstanceIter.Get());
		}
	}
}

void FScriptExecutionBlueprint::GetAllExecNodes(TMap<FName, TSharedPtr<FScriptExecutionNode>>& ExecNodesOut)
{
	for (auto BlueprintEventIter : ChildNodes)
	{
		BlueprintEventIter->GetAllExecNodes(ExecNodesOut);
		ExecNodesOut.Add(BlueprintEventIter->GetName()) = BlueprintEventIter;
	}
	for (auto InstanceIter : Instances)
	{
		InstanceIter->GetAllExecNodes(ExecNodesOut);
		ExecNodesOut.Add(InstanceIter->GetName()) = InstanceIter;
	}
	ExecNodesOut.Add(GetName()) = AsShared();
}

void FScriptExecutionBlueprint::NavigateToObject() const
{
	if (IsObservedObjectValid())
	{
		if (const UBlueprint* Blueprint = Cast<UBlueprint>(ObservedObject.Get()))
		{
			TArray<FAssetData> BlueprintAssets;
			BlueprintAssets.Add(Blueprint);
			GEditor->SyncBrowserToObjects(BlueprintAssets);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// FScriptExecutionInstance

void FScriptExecutionInstance::NavigateToObject() const
{
	if (const FWorldContext* PIEWorldContext = GEditor->GetPIEWorldContext())
	{
		if (const AActor* PIEActor = Cast<AActor>(PIEInstance.Get()))
		{
			GEditor->SelectNone(false, false);
			GEditor->SelectActor(const_cast<AActor*>(PIEActor), true, true, true);
		}
	}
	else
	{
		if (const AActor* EditorActor = Cast<AActor>(ObservedObject.Get()))
		{
			GEditor->SelectNone(false, false);
			GEditor->SelectActor(const_cast<AActor*>(EditorActor), true, true, true);
		}
	}
}

bool FScriptExecutionInstance::IsObservedObjectValid() const
{
	if (const FWorldContext* PIEWorldContext = GEditor->GetPIEWorldContext())
	{
		return PIEInstance.IsValid() && !PIEInstance.Get()->IsPendingKill();
	}
	else
	{
		return ObservedObject.IsValid() && !ObservedObject.Get()->IsPendingKill();
	}
}

TSharedRef<SWidget> FScriptExecutionInstance::GetIconWidget()
{
	return SNew(SImage)
			.Image(Icon)
			.ColorAndOpacity(this, &FScriptExecutionInstance::GetInstanceIconColor);
}

TSharedRef<SWidget> FScriptExecutionInstance::GetHyperlinkWidget()
{
	return SNew(SHyperlink)
		#if TRACEPATH_DEBUG
		.Text(FText::FromName(NodeName))
		#else
		.Text(DisplayName)
		#endif
		.Style(FEditorStyle::Get(), "HoverOnlyHyperlink")
		.ToolTipText(this, &FScriptExecutionInstance::GetInstanceTooltip)
		.OnNavigate(this, &FScriptExecutionInstance::NavigateToObject);
}

FSlateColor FScriptExecutionInstance::GetInstanceIconColor() const
{
	return FSlateColor(IsObservedObjectValid() ? IconColor : FLinearColor(1.f, 1.f, 1.f, 0.3f));
}

FText FScriptExecutionInstance::GetInstanceTooltip() const
{
	return IsObservedObjectValid() ? Tooltip : LOCTEXT("ActorInstanceInvalid_Tooltip", "Actor instance no longer exists");
}

#undef LOCTEXT_NAMESPACE