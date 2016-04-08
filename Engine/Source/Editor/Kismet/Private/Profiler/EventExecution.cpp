// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "BlueprintEditorPrivatePCH.h"
#include "EventExecution.h"
#include "ScriptPerfData.h"

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

//////////////////////////////////////////////////////////////////////////
// FScriptNodePerfData

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
		Result = MakeShareable<FScriptPerfData>(new FScriptPerfData);
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
		Result = MakeShareable<FScriptPerfData>(new FScriptPerfData);
	}
	return Result;
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

void FScriptExecutionNode::GetLinearExecutionPath(TArray<FLinearExecPath>& LinearExecutionNodes, const FTracePath& TracePath)
{
	LinearExecutionNodes.Add(FLinearExecPath(AsShared(), TracePath));
	FTracePath NewTracePath(TracePath);
	if (HasFlags(EScriptExecutionNodeFlags::FunctionTunnel))
	{
		MapTunnelLinearExecution(NewTracePath);
	}
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
				if (NodeIter.Value->HasFlags(EScriptExecutionNodeFlags::FunctionTunnel))
				{
					LinearExecutionNodes.Add(FLinearExecPath(NodeIter.Value, NewTracePath));
					for (auto TunnelExit : NodeIter.Value->LinkedNodes)
					{
						if (TunnelExit.Value->HasFlags(EScriptExecutionNodeFlags::TunnelThenPin))
						{
							MapTunnelLinearExecution(NewTracePath);
							NewTracePath.AddExitPin(TunnelExit.Key);
							TunnelExit.Value->GetLinearExecutionPath(LinearExecutionNodes, NewTracePath);
							break;
						}
					}
				}
				else
				{
					NodeIter.Value->GetLinearExecutionPath(LinearExecutionNodes, NewTracePath);
				}
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
	if (HasFlags(EScriptExecutionNodeFlags::ExecPin|EScriptExecutionNodeFlags::TunnelPin))
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

			// Refresh attached pure chain stats (if present)
			TSharedPtr<FScriptExecutionNode> PureChainNode = GetPureChainNode();
			if (PureChainNode.IsValid())
			{
				PureChainNode->RefreshStats(TracePath);
			}
			break;
		}
		case EScriptStatContainerType::NewExecutionPath:
		{
			TSet<FName> AllInstances;
			// Refresh child stats and copy as branch stats - this is a dummy entry
			for (auto ChildIter : ChildNodes)
			{
				// Fill out instance data that can be missing because its a dummy node, or the code below can fail.
				for (auto InstanceIter : ChildIter->InstanceInputPinToPerfDataMap)
				{
					if (InstanceIter.Key != NAME_None)
					{
						AllInstances.Add(InstanceIter.Key);
					}
				}
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
		if (IsPureChain())
		{
			TMap<int32, TSharedPtr<FScriptExecutionNode>> AllPureNodes;
			GetAllPureNodes(AllPureNodes);

			FTracePath PureTracePath(TracePath);
			for (auto PureIter : AllPureNodes)
			{
				PureTracePath.AddExitPin(PureIter.Key);
				PureIter.Value->RefreshStats(PureTracePath);
			}
		}
		else
		{
			for (auto LinkIter : LinkedNodes)
			{
				FTracePath LinkTracePath(TracePath);
				if (HasFlags(EScriptExecutionNodeFlags::FunctionTunnel))
				{
					MapTunnelLinearExecution(LinkTracePath);
				}
				//if (!LinkIter.Value->HasFlags(EScriptExecutionNodeFlags::DummyTrace))
				LinkTracePath.AddExitPin(LinkIter.Key);
				LinkIter.Value->RefreshStats(LinkTracePath);
			}
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
