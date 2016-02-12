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

//////////////////////////////////////////////////////////////////////////
// FScriptNodePerfData

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

bool FScriptExecutionNode::operator == (const FScriptExecutionNode& NodeIn) const
{
	return NodeName == NodeIn.NodeName;
}

void FScriptExecutionNode::GetLinearExecutionPath(TArray<FLinearExecPath>& LinearExecutionNodes, const FTracePath& TracePath)
{
	LinearExecutionNodes.Add(FLinearExecPath(AsShared(), TracePath));
	if (GetNumLinkedNodes() == 1)
	{
		FTracePath NewTracePath(TracePath);
		for (auto NodeIter : LinkedNodes)
		{
			NewTracePath.AddExitPin(NodeIter.Key);
			NodeIter.Value->GetLinearExecutionPath(LinearExecutionNodes, NewTracePath);
		}
	}
}

EScriptStatContainerType::Type FScriptExecutionNode::GetStatisticContainerType() const
{
	EScriptStatContainerType::Type Result = EScriptStatContainerType::Standard;
	if (HasFlags(EScriptExecutionNodeFlags::Instance|EScriptExecutionNodeFlags::Event|EScriptExecutionNodeFlags::CallSite))
	{
		Result = EScriptStatContainerType::Container;
	}
	else if (HasFlags(EScriptExecutionNodeFlags::SequentialBranch))
	{
		Result = EScriptStatContainerType::SequentialBranch;
	}
	else if (HasFlags(EScriptExecutionNodeFlags::ExecPin))
	{
		Result = EScriptStatContainerType::NewExecutionPath;
	}
	return Result;
}

void FScriptExecutionNode::RefreshStats(const FTracePath& TracePath)
{
	bool bRefreshChildStats = false;
	bool bRefreshLinkStats = true;
	TArray<TSharedPtr<FScriptPerfData>> BlueprintPooledStats;
	// Update stats based on type
	switch(GetStatisticContainerType())
	{
		case EScriptStatContainerType::Container:
		{
			// Only consider stat data on this path
			GetInstancePerfDataByTracePath(TracePath, BlueprintPooledStats);
			bRefreshChildStats = true;
			break;
		}
		case EScriptStatContainerType::Standard:
		case EScriptStatContainerType::SequentialBranch:
		{
			// Only consider stat data on this path
			GetInstancePerfDataByTracePath(TracePath, BlueprintPooledStats);
			break;
		}
		//case EScriptStatContainerType::SequentialBranch:
		//{
		//	for (auto InstanceIter : InstanceInputPinToPerfDataMap)
		//	{
		//		if (InstanceIter.Key != NAME_None)
		//		{
		//			TSharedPtr<FScriptPerfData> InstancePerfData = GetPerfDataByInstanceAndTracePath(InstanceIter.Key, TracePath);
		//			InstancePerfData->Reset();
		//			BlueprintPooledStats.Add(InstancePerfData);
		//		
		//			for (auto LinkIter : LinkedNodes)
		//			{
		//				FTracePath LinkTracePath(TracePath);
		//				LinkTracePath.AddExitPin(LinkIter.Key);
		//				TSharedPtr<FScriptPerfData> LinkPerfData = LinkIter.Value->GetPerfDataByInstanceAndTracePath(InstanceIter.Key, LinkTracePath);
		//				InstancePerfData->AddData(*LinkPerfData.Get());
		//				LinkIter.Value->RefreshStats(LinkTracePath);
		//			}
		//		}
		//	}
		//	bRefreshLinkStats = false;
		//	break;
		//}
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
		for (auto LinkIter : LinkedNodes)
		{
			FTracePath LinkTracePath(TracePath);
			LinkTracePath.AddExitPin(LinkIter.Key);
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

//////////////////////////////////////////////////////////////////////////
// FScriptExecutionContext

void FScriptExecutionContext::GetAllExecNodes(TMap<FName, TSharedPtr<FScriptExecutionNode>>& ExecNodesOut)
{
	if (ExecutionNode.IsValid())
	{
		ExecutionNode->GetAllExecNodes(ExecNodesOut);
	}
}

const UEdGraphNode* FScriptExecutionContext::GetNodeFromCodeLocation(const int32 CodeLocation, UFunction* FunctionOverride) const
{
	if (BlueprintClass.IsValid())
	{
		if (FunctionOverride)
		{
			return BlueprintClass.Get()->GetDebugData().FindSourceNodeFromCodeLocation(FunctionOverride, CodeLocation, true);
		}
		else if (BlueprintFunction.IsValid())
		{
			return BlueprintClass.Get()->GetDebugData().FindSourceNodeFromCodeLocation(BlueprintFunction.Get(), CodeLocation, true);
		}
	}
	return nullptr;
}

void FScriptExecutionContext::UpdateConnectedStats()
{
	if (ExecutionNode.IsValid())
	{
		FTracePath InitialTracePath;
		ExecutionNode->RefreshStats(InitialTracePath);
	}
}
