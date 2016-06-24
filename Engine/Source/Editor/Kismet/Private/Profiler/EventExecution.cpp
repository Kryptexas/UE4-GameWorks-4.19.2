// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"
#include "EventExecution.h"

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

TSharedPtr<FScriptPerfData> FScriptNodePerfData::GetPerfDataByInstanceAndTracePath(FName InstanceName, const FTracePath& TracePath)
{ 
	TMap<const uint32, TSharedPtr<FScriptPerfData>>& InstanceMap = InstanceInputPinToPerfDataMap.FindOrAdd(InstanceName);
	TSharedPtr<FScriptPerfData>& Result = InstanceMap.FindOrAdd(TracePath);
	if (!Result.IsValid())
	{
		Result = MakeShareable<FScriptPerfData>(new FScriptPerfData(GetPerfDataType()));
	}
	return Result;
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

TSharedPtr<FScriptPerfData> FScriptNodePerfData::GetBlueprintPerfDataByTracePath(const FTracePath& TracePath)
{
	TMap<const uint32, TSharedPtr<FScriptPerfData>>& BlueprintMap = InstanceInputPinToPerfDataMap.FindOrAdd(NAME_None);
	TSharedPtr<FScriptPerfData>& Result = BlueprintMap.FindOrAdd(TracePath);
	if (!Result.IsValid())
	{
		Result = MakeShareable<FScriptPerfData>(new FScriptPerfData(GetPerfDataType()));
	}
	return Result;
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
			for (auto ChildIter : ChildNodes)
			{
				ChildIter->GetValidInstanceNames(AllInstances);
				ChildIter->RefreshStats(TracePath);
			}
			for (auto InstanceIter : AllInstances)
			{
				if (InstanceIter != NAME_None)
				{
					TSharedPtr<FScriptPerfData> InstancePerfData = GetPerfDataByInstanceAndTracePath(InstanceIter, TracePath);
					InstancePerfData->Reset();
					BlueprintPooledStats.Add(InstancePerfData);

					for (auto ChildIter : ChildNodes)
					{
						TSharedPtr<FScriptPerfData> ChildPerfData = ChildIter->GetPerfDataByInstanceAndTracePath(InstanceIter, TracePath);
						InstancePerfData->AddBranchData(*ChildPerfData.Get());
					}
				}
			}
			// Update Blueprint stats as branches
			TSharedPtr<FScriptPerfData> BlueprintData = GetBlueprintPerfDataByTracePath(TracePath);
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
		TSharedPtr<FScriptPerfData> BlueprintData = GetBlueprintPerfDataByTracePath(TracePath);
		BlueprintData->Reset();

		for (auto BlueprintChildDataIter : BlueprintPooledStats)
		{
			BlueprintData->AddData(*BlueprintChildDataIter.Get());
		}
	}
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
	if (ObservedObject.IsValid())
	{
		FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(ObservedObject.Get());
	}
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
		FTracePath TunnelTracePath(TracePath, SharedThis<const FScriptExecutionTunnelEntry>(this));
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
			FTracePath ChildTrace(TracePath);
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
			FTracePath ChildTrace(TracePath);
			for (auto Child : ChildNodes)
			{
				ChildArrayInOut.Add(FLinearExecPath(Child, ChildTrace));
			}
			break;
		}
		case MultiIOTunnel:
		{
			TArray<FLinearExecPath> Results;
			FTracePath ChildTrace(TracePath);
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
	// Refresh Child Links
	for (auto ChildIter : ChildNodes)
	{
		ChildIter->RefreshStats(TracePath);
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
		BlueprintData->Reset();
		for (auto BlueprintChildDataIter : BlueprintPooledStats)
		{
			BlueprintData->AddData(*BlueprintChildDataIter.Get());
		}
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

void FScriptExecutionTunnelExit::NavigateToObject() const
{
	if (ObservedObject.IsValid())
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
		BlueprintData->Reset();

		for (auto BlueprintChildDataIter : BlueprintPooledStats)
		{
			BlueprintData->AddData(*BlueprintChildDataIter.Get());
		}
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
	// Update instance stats
	for (auto InstanceIter : Instances)
	{
		TSharedPtr<FScriptPerfData> InstancePerfData = InstanceIter->GetPerfDataByInstanceAndTracePath(InstanceIter->GetName(), TracePath);
		InstancePerfData->Reset();
		// Update all top level instance stats now the events are up to date.
		for (auto InstanceEventIter : InstanceIter->GetChildNodes())
		{
			TSharedPtr<FScriptPerfData> InstanceEventPerfData = InstanceEventIter->GetPerfDataByInstanceAndTracePath(InstanceIter->GetName(), TracePath);
			InstancePerfData->AddData(*InstanceEventPerfData.Get());
		}
		// Add for consolidation at the bottom.
		InstanceData.Add(InstancePerfData);
	}
	// Finally... update the root stats for the owning blueprint.
	if (InstanceData.Num() > 0)
	{
		TSharedPtr<FScriptPerfData> BlueprintData = GetBlueprintPerfDataByTracePath(TracePath);
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
	if (ObservedObject.IsValid())
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
	if (ObservedObject.IsValid())
	{
		if (const AActor* Actor = Cast<AActor>(ObservedObject.Get()))
		{
			if (const FWorldContext* PIEWorldContext = GEditor->GetPIEWorldContext())
			{
				if (const UWorld* PIEWorld = PIEWorldContext->World())
				{
					for (auto LevelIter : PIEWorld->GetLevels())
					{
						if (AActor* PIEActor = Cast<AActor>(FindObject<UObject>(LevelIter, *Actor->GetName())))
						{
							GEditor->SelectNone(false, false);
							GEditor->SelectActor(const_cast<AActor*>(PIEActor), true, true, true);
							break;
						}
					}
				}
			}
			else
			{
				GEditor->SelectNone(false, false);
				GEditor->SelectActor(const_cast<AActor*>(Actor), true, true, true);
			}
		}
	}
}
