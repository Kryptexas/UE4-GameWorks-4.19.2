// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTreeDelegates.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/BehaviorTreeManager.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

//----------------------------------------------------------------------//
// UBehaviorTreeComponent
//----------------------------------------------------------------------//

UBehaviorTreeComponent::UBehaviorTreeComponent(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	ActiveInstanceIdx = 0;
	bLoopExecution = false;
	bWaitingForAbortingTasks = false;
	bRequestedFlowUpdate = false;
	bAutoActivate = true;
	bWantsInitializeComponent = true; 
	bIsRunning = false;
	bIsPaused = false;
	
	SearchData.OwnerComp = this;
}

void UBehaviorTreeComponent::RestartLogic()
{
	UE_VLOG(GetOwner(), LogBehaviorTree, Log, TEXT("UBehaviorTreeComponent::RestartLogic"));

	bIsRunning = true;
	RestartTree();
}

void UBehaviorTreeComponent::StopLogic(const FString& Reason) 
{
	UE_VLOG(GetOwner(), LogBehaviorTree, Log, TEXT("Stopping BT, reason: \'%s\'"), *Reason);
	bIsRunning = false;
}

void UBehaviorTreeComponent::PauseLogic(const FString& Reason)
{
	UE_VLOG(GetOwner(), LogBehaviorTree, Log, TEXT("Execution updates: PAUSED (%s)"), *Reason);
	bIsPaused = true;

	if (BlackboardComp)
	{
		BlackboardComp->PauseUpdates();
	}
}

EAILogicResuming::Type UBehaviorTreeComponent::ResumeLogic(const FString& Reason)
{
	const EAILogicResuming::Type SuperResumeResult = Super::ResumeLogic(Reason);
	if (!!bIsPaused)
	{
		bIsPaused = false;

		if (SuperResumeResult == EAILogicResuming::Continue)
		{
			if (BlackboardComp)
			{
				BlackboardComp->ResumeUpdates();
			}

			if (ExecutionRequest.ExecuteNode)
			{
				ScheduleExecutionUpdate();
			}

			return EAILogicResuming::Continue;
		}
	}

	return SuperResumeResult;
}

bool UBehaviorTreeComponent::TreeHasBeenStarted() const
{
	return bIsRunning && InstanceStack.Num();
}

bool UBehaviorTreeComponent::IsRunning() const
{ 
	return bIsPaused == false && TreeHasBeenStarted() == true;
}

bool UBehaviorTreeComponent::IsPaused() const
{
	return bIsPaused;
}

bool UBehaviorTreeComponent::StartTree(class UBehaviorTree* TreeAsset, EBTExecutionMode::Type ExecuteMode)
{
	if (UBehaviorTreeManager::IsBehaviorTreeUsageEnabled() == false)
	{
		UE_VLOG(GetOwner(), LogBehaviorTree, Error, TEXT("Behavior tree usage is not enabled"));
		return false;
	}

	// clear instance stack, start should always run new tree from root
	class UBehaviorTree* CurrentRoot = GetRootTree();
	
	if (CurrentRoot == TreeAsset && TreeHasBeenStarted())
	{
		UE_VLOG(GetOwner(), LogBehaviorTree, Log, TEXT("Skipping behavior start request - it's already running"));
		return true;
	}
	else if (CurrentRoot)
	{
		UE_VLOG(GetOwner(), LogBehaviorTree, Log, TEXT("Abandoning behavior %s to start new one (%s)"),
			*GetNameSafe(CurrentRoot), *GetNameSafe(TreeAsset));
	}

	StopTree();
	bLoopExecution = (ExecuteMode == EBTExecutionMode::Looped);
	bIsRunning = true;

#if USE_BEHAVIORTREE_DEBUGGER
	DebuggerSteps.Reset();
#endif

	// push new instance
	const bool bPushed = PushInstance(TreeAsset);
	return bPushed;
}

void UBehaviorTreeComponent::StopTree()
{
	// clear current state, don't touch debugger data
	for (int32 i = 0; i < InstanceStack.Num(); i++)
	{
		InstanceStack[i].Cleanup(this);
	}

	InstanceStack.Reset();
	TaskMessageObservers.Reset();
	ExecutionRequest = FBTNodeExecutionInfo();
	ActiveInstanceIdx = 0;
	GetOwner()->GetWorldTimerManager().ClearTimer(this, &UBehaviorTreeComponent::ProcessExecutionRequest);
	// make sure to allow new execution requests since we just removed last request timer
	bRequestedFlowUpdate = false;
}

void UBehaviorTreeComponent::RestartTree()
{
	UE_VLOG(GetOwner(), LogBehaviorTree, Log, TEXT("UBehaviorTreeComponent::RestartTree"));
	
	if (InstanceStack.Num())
	{
		FBehaviorTreeInstance& TopInstance = InstanceStack[0];
		RequestExecution(TopInstance.RootNode, 0, TopInstance.RootNode, -1, EBTNodeResult::Aborted);
	}
}

void UBehaviorTreeComponent::OnTaskFinished(const class UBTTaskNode* TaskNode, EBTNodeResult::Type TaskResult)
{
	if (TaskNode == NULL || InstanceStack.Num() == 0 || IsPendingKill())
	{
		return;
	}

	UE_VLOG(GetOwner(), LogBehaviorTree, Log, TEXT("Task %s finished: %s"), 
		*UBehaviorTreeTypes::DescribeNodeHelper(TaskNode), *UBehaviorTreeTypes::DescribeNodeResult(TaskResult));

	// notify parent node
	UBTCompositeNode* ParentNode = TaskNode->GetParentNode();
	const int32 TaskInstanceIdx = FindInstanceContainingNode(TaskNode);
	uint8* ParentMemory = ParentNode->GetNodeMemory<uint8>(InstanceStack[TaskInstanceIdx]);

	const bool bWasWaitingForAbort = bWaitingForAbortingTasks;
	ParentNode->ConditionalNotifyChildExecution(this, ParentMemory, TaskNode, TaskResult);
	
	if (TaskResult != EBTNodeResult::InProgress)
	{
		StoreDebuggerSearchStep(TaskNode, TaskInstanceIdx, TaskResult);

		// cleanup task observers
		UnregisterMessageObserversFrom(TaskNode);

		// update execution when active task is finished
		FBehaviorTreeInstance& ActiveInstance = InstanceStack[ActiveInstanceIdx];
		if (ActiveInstance.ActiveNode == TaskNode)
		{
			const bool bWasAborting = (ActiveInstance.ActiveNodeType == EBTActiveNode::AbortingTask);
			ActiveInstance.ActiveNodeType = EBTActiveNode::InactiveTask;

			// request execution from parent
			if (!bWasAborting)
			{
				RequestExecution(TaskResult);
			}
		}

		// update state of aborting tasks after currently finished one was set to Inactive
		UpdateAbortingTasks();

		// make sure that we continue execution after all pending latent aborts finished
		if (!bWaitingForAbortingTasks && bWasWaitingForAbort)
		{
			ScheduleExecutionUpdate();
		}
	}
	else
	{
		// always update state of aborting tasks
		UpdateAbortingTasks();
	}
}

void UBehaviorTreeComponent::OnTreeFinished()
{
	UE_VLOG(GetOwner(), LogBehaviorTree, Verbose, TEXT("Ran out of nodes to check, %s tree."),
		bLoopExecution ? TEXT("looping") : TEXT("stopping"));

	ActiveInstanceIdx = 0;
	StoreDebuggerExecutionStep(EBTExecutionSnap::OutOfNodes);

	if (bLoopExecution)
	{
		// it should be already deactivated (including root)
		// set active node to initial state: root activation
		FBehaviorTreeInstance& TopInstance = InstanceStack[0];
		TopInstance.ActiveNode = NULL;
		TopInstance.ActiveNodeType = EBTActiveNode::Composite;

		// result doesn't really matter, root node will be reset and start iterating child nodes from scratch
		// although it shouldn't be set to Aborted, as it has special meaning in RequestExecution (switch to higher priority)
		RequestExecution(TopInstance.RootNode, 0, TopInstance.RootNode, 0, EBTNodeResult::InProgress);
	}
	else
	{
		StopTree();
	}
}

bool UBehaviorTreeComponent::IsExecutingBranch(const class UBTNode* Node, int32 ChildIndex) const
{
	const int32 TestInstanceIdx = FindInstanceContainingNode(Node);
	if (TestInstanceIdx == INDEX_NONE || InstanceStack[TestInstanceIdx].ActiveNode == NULL)
	{
		return false;
	}

	// check if branch is currently being restarted:
	const FBTNodeIndex TestIndex(TestInstanceIdx, Node->GetExecutionIndex());
	if (ExecutionRequest.ExecuteNode)
	{
		// - search takes priority (won't work on decorators above)
		if (ExecutionRequest.SearchStart.TakesPriorityOver(TestIndex) ||
			ExecutionRequest.SearchStart == TestIndex)
		{
			return false;
		}

		// - special case for decorators skipped by previous check
		const UBTDecorator* TestDecorator = Cast<const UBTDecorator>(Node);
		if (TestDecorator && TestDecorator->GetParentNode() == ExecutionRequest.ExecuteNode)
		{
			return false;
		}
	}

	// is it active node or root of tree?
	const FBehaviorTreeInstance& TestInstance = InstanceStack[TestInstanceIdx];
	if (Node == TestInstance.RootNode || Node == TestInstance.ActiveNode)
	{
		return true;
	}

	// compare with index of next child
	const uint16 ActiveExecutionIndex = TestInstance.ActiveNode->GetExecutionIndex();
	const uint16 NextChildExecutionIndex = Node->GetParentNode()->GetChildExecutionIndex(ChildIndex + 1);
	return (ActiveExecutionIndex >= Node->GetExecutionIndex()) && (ActiveExecutionIndex < NextChildExecutionIndex);
}

bool UBehaviorTreeComponent::IsAuxNodeActive(const class UBTAuxiliaryNode* AuxNode) const
{
	if (AuxNode == NULL)
	{
		return false;
	}

	const uint16 AuxExecutionIndex = AuxNode->GetExecutionIndex();
	for (int32 i = 0; i < InstanceStack.Num(); i++)
	{
		const FBehaviorTreeInstance& InstanceInfo = InstanceStack[i];
		for (int32 iAux = 0; iAux < InstanceInfo.ActiveAuxNodes.Num(); iAux++)
		{
			const UBTAuxiliaryNode* TestAuxNode = InstanceInfo.ActiveAuxNodes[iAux];

			// check template version
			if (TestAuxNode == AuxNode)
			{
				return true;
			}

			// check instanced version
			if (AuxNode->IsInstanced() && TestAuxNode && TestAuxNode->GetExecutionIndex() == AuxExecutionIndex)
			{
				const uint8* NodeMemory = TestAuxNode->GetNodeMemory<uint8>(InstanceInfo);
				UBTNode* NodeInstance = TestAuxNode->GetNodeInstance(this, (uint8*)NodeMemory);

				if (NodeInstance == AuxNode)
				{
					return true;
				}
			}
		}
	}

	return false;
}

EBTTaskStatus::Type UBehaviorTreeComponent::GetTaskStatus(const class UBTTaskNode* TaskNode) const
{
	EBTTaskStatus::Type Status = EBTTaskStatus::Inactive;
	const int32 InstanceIdx = FindInstanceContainingNode(TaskNode);

	if (InstanceIdx != INDEX_NONE)
	{
		const uint16 ExecutionIndex = TaskNode->GetExecutionIndex();
		const FBehaviorTreeInstance& InstanceInfo = InstanceStack[InstanceIdx];

		// always check parallel execution first, it takes priority over ActiveNodeType
		for (int32 i = 0; i < InstanceInfo.ParallelTasks.Num(); i++)
		{
			const FBehaviorTreeParallelTask& ParallelInfo = InstanceInfo.ParallelTasks[i];
			if (ParallelInfo.TaskNode == TaskNode ||
				(TaskNode->IsInstanced() && ParallelInfo.TaskNode && ParallelInfo.TaskNode->GetExecutionIndex() == ExecutionIndex))
			{
				Status = ParallelInfo.Status;
				break;
			}
		}

		if (Status == EBTTaskStatus::Inactive)
		{
			if (InstanceInfo.ActiveNode == TaskNode ||
				(TaskNode->IsInstanced() && InstanceInfo.ActiveNode && InstanceInfo.ActiveNode->GetExecutionIndex() == ExecutionIndex))
			{
				Status =
					(InstanceInfo.ActiveNodeType == EBTActiveNode::ActiveTask) ? EBTTaskStatus::Active :
					(InstanceInfo.ActiveNodeType == EBTActiveNode::AbortingTask) ? EBTTaskStatus::Aborting :
					EBTTaskStatus::Inactive;
			}
		}
	}

	return Status;
}

void UBehaviorTreeComponent::RequestExecution(const class UBTDecorator* RequestedBy)
{
	// search range depends on decorator's FlowAbortMode:
	//
	// - LowerPri: try entering branch = search only nodes under decorator
	//
	// - Self: leave execution = from node under decorator to end of tree
	//         UNLESS branch is no longer executing (pending restart), in which case it will restart parent
	//
	// - Both: check if active node is within inner child nodes and choose Self or LowerPri
	//

	EBTFlowAbortMode::Type AbortMode = RequestedBy->GetFlowAbortMode();
	if (AbortMode == EBTFlowAbortMode::None)
	{
		return;
	}

	const int32 InstanceIdx = FindInstanceContainingNode(RequestedBy->GetParentNode());
	if (InstanceIdx == INDEX_NONE)
	{
		return;
	}

	if (AbortMode == EBTFlowAbortMode::Both || AbortMode == EBTFlowAbortMode::Self)
	{
		const bool bIsExecutingChildNodes = IsExecutingBranch(RequestedBy, RequestedBy->GetChildIndex());
		AbortMode = bIsExecutingChildNodes ? EBTFlowAbortMode::Self : EBTFlowAbortMode::LowerPriority;
	}

	EBTNodeResult::Type ContinueResult = (AbortMode == EBTFlowAbortMode::Self) ? EBTNodeResult::Failed : EBTNodeResult::Aborted;
	RequestExecution(RequestedBy->GetParentNode(), InstanceIdx, RequestedBy, RequestedBy->GetChildIndex(), ContinueResult);
}

void UBehaviorTreeComponent::RequestExecution(EBTNodeResult::Type LastResult)
{
	// task helpers can't continue with InProgress or Aborted result, it should be handled 
	// either by decorator helper or regular RequestExecution() (6 param version)

	if (LastResult != EBTNodeResult::Aborted && LastResult != EBTNodeResult::InProgress)
	{
		const FBehaviorTreeInstance& ActiveInstance = InstanceStack[ActiveInstanceIdx];
		UBTCompositeNode* ExecuteParent = (ActiveInstance.ActiveNode == NULL) ? ActiveInstance.RootNode :
			(ActiveInstance.ActiveNodeType == EBTActiveNode::Composite) ? (UBTCompositeNode*)ActiveInstance.ActiveNode :
			ActiveInstance.ActiveNode->GetParentNode();

		RequestExecution(ExecuteParent, InstanceStack.Num() - 1,
			ActiveInstance.ActiveNode ? ActiveInstance.ActiveNode : ActiveInstance.RootNode, -1,
			LastResult, false);
	}
}

static void FindCommonParent(const TArray<FBehaviorTreeInstance>& Instances, const TArray<FBehaviorTreeInstanceId>& KnownInstances,
							 class UBTCompositeNode* InNodeA, uint16 InstanceIdxA,
							 class UBTCompositeNode* InNodeB, uint16 InstanceIdxB,
							 class UBTCompositeNode*& CommonParentNode, uint16& CommonInstanceIdx)
{
	// find two nodes in the same instance (choose lower index = closer to root)
	CommonInstanceIdx = (InstanceIdxA <= InstanceIdxB) ? InstanceIdxA : InstanceIdxB;

	class UBTCompositeNode* NodeA = (CommonInstanceIdx == InstanceIdxA) ? InNodeA : Instances[CommonInstanceIdx].ActiveNode->GetParentNode();
	class UBTCompositeNode* NodeB = (CommonInstanceIdx == InstanceIdxB) ? InNodeB : Instances[CommonInstanceIdx].ActiveNode->GetParentNode();

	// special case: node was taken from CommonInstanceIdx, but it had ActiveNode set to root (no parent)
	if (NodeA == NULL && CommonInstanceIdx != InstanceIdxA)
	{
		NodeA = Instances[CommonInstanceIdx].RootNode;
	}
	if (NodeB == NULL && CommonInstanceIdx != InstanceIdxB)
	{
		NodeB = Instances[CommonInstanceIdx].RootNode;
	}

	// if one of nodes is still empty, we have serious problem with execution flow - crash and log details
	if (NodeA == NULL || NodeB == NULL)
	{
		FString AssetAName = Instances.IsValidIndex(InstanceIdxA) && KnownInstances.IsValidIndex(Instances[InstanceIdxA].InstanceIdIndex) ? GetNameSafe(KnownInstances[Instances[InstanceIdxA].InstanceIdIndex].TreeAsset) : TEXT("unknown");
		FString AssetBName = Instances.IsValidIndex(InstanceIdxB) && KnownInstances.IsValidIndex(Instances[InstanceIdxB].InstanceIdIndex) ? GetNameSafe(KnownInstances[Instances[InstanceIdxB].InstanceIdIndex].TreeAsset) : TEXT("unknown");
		FString AssetCName = Instances.IsValidIndex(CommonInstanceIdx) && KnownInstances.IsValidIndex(Instances[CommonInstanceIdx].InstanceIdIndex) ? GetNameSafe(KnownInstances[Instances[CommonInstanceIdx].InstanceIdIndex].TreeAsset) : TEXT("unknown");

		UE_LOG(LogBehaviorTree, Fatal, TEXT("Fatal error in FindCommonParent() call.\nInNodeA: %s, InstanceIdxA: %d (%s), NodeA: %s\nInNodeB: %s, InstanceIdxB: %d (%s), NodeB: %s\nCommonInstanceIdx: %d (%s), ActiveNode: %s%s"),
			*UBehaviorTreeTypes::DescribeNodeHelper(InNodeA), InstanceIdxA, *AssetAName, *UBehaviorTreeTypes::DescribeNodeHelper(NodeA),
			*UBehaviorTreeTypes::DescribeNodeHelper(InNodeB), InstanceIdxB, *AssetBName, *UBehaviorTreeTypes::DescribeNodeHelper(NodeB),
			CommonInstanceIdx, *AssetCName, *UBehaviorTreeTypes::DescribeNodeHelper(Instances[CommonInstanceIdx].ActiveNode),
			(Instances[CommonInstanceIdx].ActiveNode == Instances[CommonInstanceIdx].RootNode) ? TEXT(" (root)") : TEXT(""));
	}

	// find common parent of two nodes
	int32 NodeADepth = NodeA->GetTreeDepth();
	int32 NodeBDepth = NodeB->GetTreeDepth();

	while (NodeADepth > NodeBDepth)
	{
		NodeA = NodeA->GetParentNode();
		NodeADepth = NodeA->GetTreeDepth();
	}

	while (NodeBDepth > NodeADepth)
	{
		NodeB = NodeB->GetParentNode();
		NodeBDepth = NodeB->GetTreeDepth();
	}

	while (NodeA != NodeB)
	{
		NodeA = NodeA->GetParentNode();
		NodeB = NodeB->GetParentNode();
	}

	CommonParentNode = NodeA;
}

void UBehaviorTreeComponent::ScheduleExecutionUpdate()
{
	if (!bRequestedFlowUpdate)
	{
		bRequestedFlowUpdate = true;

		GetOwner()->GetWorldTimerManager().SetTimer(this, &UBehaviorTreeComponent::ProcessExecutionRequest, 0.001f, false);
	}
}

void UBehaviorTreeComponent::RequestExecution(class UBTCompositeNode* RequestedOn, int32 InstanceIdx, const class UBTNode* RequestedBy,
											  int32 RequestedByChildIndex, EBTNodeResult::Type ContinueWithResult, bool bStoreForDebugger)
{
	SCOPE_CYCLE_COUNTER(STAT_AI_BehaviorTree_SearchTime);

	UE_VLOG(GetOwner(), LogBehaviorTree, Log, TEXT("Execution request by %s (result: %s)"),
		*UBehaviorTreeTypes::DescribeNodeHelper(RequestedBy),
		*UBehaviorTreeTypes::DescribeNodeResult(ContinueWithResult));

	if (!bIsRunning || GetOwner() == NULL || GetOwner()->IsPendingKillPending())
	{
		UE_VLOG(GetOwner(), LogBehaviorTree, Log, TEXT("> skip: tree is not running"));
		return;
	}

	const bool bSwitchToHigherPriority = (ContinueWithResult == EBTNodeResult::Aborted);
	const UBTNode* DebuggerNode = bStoreForDebugger ? RequestedBy : NULL;

	FBTNodeIndex ExecutionIdx;
	ExecutionIdx.InstanceIndex = InstanceIdx;
	ExecutionIdx.ExecutionIndex = RequestedBy->GetExecutionIndex();

	if (bSwitchToHigherPriority)
	{
		RequestedByChildIndex = FMath::Max(0, RequestedByChildIndex);
		ExecutionIdx.ExecutionIndex = RequestedOn->GetChildExecutionIndex(RequestedByChildIndex, EBTChildIndex::FirstNode);
	}

	// check if it's more important than currently requested
	if (ExecutionRequest.ExecuteNode && ExecutionRequest.SearchStart.TakesPriorityOver(ExecutionIdx))
	{
		UE_VLOG(GetOwner(), LogBehaviorTree, Log, TEXT("> skip: already has request with higher priority"));
		StoreDebuggerRestart(DebuggerNode, InstanceIdx, true);
		return;
	}

	// when it's aborting and moving to higher priority node:
	if (bSwitchToHigherPriority)
	{
		// check if decorators allow execution on requesting link
		const bool bShouldCheckDecorators = (RequestedByChildIndex >= 0);
		const bool bCanExecute = !bShouldCheckDecorators || RequestedOn->DoDecoratorsAllowExecution(this, InstanceIdx, RequestedByChildIndex);
		if (!bCanExecute)
		{
			UE_VLOG(GetOwner(), LogBehaviorTree, Log, TEXT("> skip: decorators are not allowing execution"));
			StoreDebuggerRestart(DebuggerNode, InstanceIdx, false);
			return;
		}

		// update common parent: requesting node with prev common/active node
		UBTCompositeNode* CurrentNode = ExecutionRequest.ExecuteNode;
		uint16 CurrentInstanceIdx = ExecutionRequest.ExecuteInstanceIdx;
		if (ExecutionRequest.ExecuteNode == NULL)
		{
			FBehaviorTreeInstance& ActiveInstance = InstanceStack[ActiveInstanceIdx];
			CurrentNode = (ActiveInstance.ActiveNode == NULL) ? ActiveInstance.RootNode :
				(ActiveInstance.ActiveNodeType == EBTActiveNode::Composite) ? (UBTCompositeNode*)ActiveInstance.ActiveNode :
				ActiveInstance.ActiveNode->GetParentNode();

			CurrentInstanceIdx = InstanceStack.Num() - 1;
		}

		if (ExecutionRequest.ExecuteNode != RequestedOn)
		{
			UBTCompositeNode* CommonParent = NULL;
			uint16 CommonInstanceIdx = MAX_uint16;

			FindCommonParent(InstanceStack, KnownInstances, RequestedOn, InstanceIdx, CurrentNode, CurrentInstanceIdx, CommonParent, CommonInstanceIdx);

			// check decorators between common parent and restart parent
			// it's always on the same stack level, because only tasks can push new subtrees
			for (UBTCompositeNode* It = RequestedOn; It && It != CommonParent;)
			{
				UBTCompositeNode* ParentNode = It->GetParentNode();
				const int32 ChildIdx = ParentNode->GetChildIndex(It);
				const bool bCanExecuteTest = ParentNode->DoDecoratorsAllowExecution(this, CommonInstanceIdx, ChildIdx);
				if (!bCanExecuteTest)
				{
					UE_VLOG(GetOwner(), LogBehaviorTree, Log, TEXT("> skip: decorators are not allowing execution"));
					StoreDebuggerRestart(DebuggerNode, InstanceIdx, false);
					return;
				}

				It = ParentNode;
			}

			ExecutionRequest.ExecuteNode = CommonParent;
			ExecutionRequest.ExecuteInstanceIdx = CommonInstanceIdx;
		}
	}
	else
	{
		// check if decorators allow execution on requesting link (only when restart comes from composite decorator)
		const bool bShouldCheckDecorators = RequestedOn->Children.IsValidIndex(RequestedByChildIndex) &&
			(RequestedOn->Children[RequestedByChildIndex].DecoratorOps.Num() > 0) &&
			RequestedBy->IsA(UBTDecorator::StaticClass());

		const bool bCanExecute = bShouldCheckDecorators && RequestedOn->DoDecoratorsAllowExecution(this, InstanceIdx, RequestedByChildIndex);
		if (bCanExecute)
		{
			UE_VLOG(GetOwner(), LogBehaviorTree, Log, TEXT("> skip: decorators are still allowing execution"));
			StoreDebuggerRestart(DebuggerNode, InstanceIdx, false);
			return;
		}

		ExecutionRequest.ExecuteNode = RequestedOn;
		ExecutionRequest.ExecuteInstanceIdx = InstanceIdx;
	}

	// store it
	StoreDebuggerRestart(DebuggerNode, InstanceIdx, true);

	ExecutionRequest.ContinueWithResult = ContinueWithResult;
	ExecutionRequest.SearchStart = ExecutionIdx;
	ExecutionRequest.bTryNextChild = !bSwitchToHigherPriority;

	ScheduleExecutionUpdate();
}

void UBehaviorTreeComponent::ApplySearchUpdates(const TArray<struct FBehaviorTreeSearchUpdate>& UpdateList, int32 UpToIdx, int32 NewNodeExecutionIndex, bool bPostUpdate)
{
	for (int32 i = 0; i < UpToIdx; i++)
	{
		const FBehaviorTreeSearchUpdate& UpdateInfo = UpdateList[i];
		const UBTNode* UpdateNode = UpdateInfo.AuxNode ? (const UBTNode*)UpdateInfo.AuxNode : (const UBTNode*)UpdateInfo.TaskNode;
		FBehaviorTreeInstance& UpdateInstance = InstanceStack[UpdateInfo.InstanceIndex];
		int32 ParallelTaskIdx = INDEX_NONE;
		bool bIsActive = false;

		if (UpdateInfo.AuxNode)
		{
			bIsActive = UpdateInstance.ActiveAuxNodes.Contains(UpdateInfo.AuxNode);
		}
		else if (UpdateInfo.TaskNode)
		{
			ParallelTaskIdx = UpdateInstance.ParallelTasks.IndexOfByKey(UpdateInfo.TaskNode);
			bIsActive = (ParallelTaskIdx != INDEX_NONE && UpdateInstance.ParallelTasks[ParallelTaskIdx].Status == EBTTaskStatus::Active);
		}

		if ((UpdateInfo.Mode == EBTNodeUpdateMode::Remove && !bIsActive) ||
			(UpdateInfo.Mode == EBTNodeUpdateMode::Add && bIsActive) ||
			(UpdateInfo.Mode == EBTNodeUpdateMode::AddForLowerPri && (bIsActive || UpdateNode->GetExecutionIndex() > NewNodeExecutionIndex)) ||
			(UpdateInfo.bPostUpdate != bPostUpdate))
		{
			continue;
		}

		UE_VLOG(GetOwner(), LogBehaviorTree, Log, TEXT("Update: %s for %s: %s"),
			*UBehaviorTreeTypes::DescribeNodeUpdateMode(UpdateInfo.Mode),
			UpdateInfo.AuxNode ? TEXT("auxiliary node") : TEXT("parallel's main task"),
			*UBehaviorTreeTypes::DescribeNodeHelper(UpdateNode));

		if (UpdateInfo.AuxNode)
		{
			// special case: service node at root of top most subtree - don't remove/re-add them when tree is in looping mode
			// don't bother with decorators parent == root means that they are on child branches
			if (bLoopExecution && UpdateInfo.AuxNode->GetParentNode() == InstanceStack[0].RootNode &&
				UpdateInfo.AuxNode->IsA(UBTService::StaticClass()))
			{
				if (UpdateInfo.Mode == EBTNodeUpdateMode::Remove ||
					InstanceStack[0].ActiveAuxNodes.Contains(UpdateInfo.AuxNode))
				{
					UE_VLOG(GetOwner(), LogBehaviorTree, Verbose, TEXT("> skip [looped execution]"));
					continue;
				}
			}

			uint8* NodeMemory = (uint8*)UpdateNode->GetNodeMemory<uint8>(UpdateInstance);
			if (UpdateInfo.Mode == EBTNodeUpdateMode::Remove)
			{
				UpdateInstance.ActiveAuxNodes.RemoveSingleSwap(UpdateInfo.AuxNode);
				UpdateInfo.AuxNode->WrappedOnCeaseRelevant(this, NodeMemory);
			}
			else
			{
				UpdateInstance.ActiveAuxNodes.Add(UpdateInfo.AuxNode);
				UpdateInfo.AuxNode->WrappedOnBecomeRelevant(this, NodeMemory);
			}
		}
		else if (UpdateInfo.TaskNode)
		{
			if (UpdateInfo.Mode == EBTNodeUpdateMode::Remove)
			{
				// remove all message observers from node to abort to avoid calling OnTaskFinished from AbortTask
				UnregisterMessageObserversFrom(UpdateInfo.TaskNode);

				uint8* NodeMemory = (uint8*)UpdateNode->GetNodeMemory<uint8>(UpdateInstance);
				EBTNodeResult::Type NodeResult = UpdateInfo.TaskNode->AbortTask(this, NodeMemory);

				UE_VLOG(GetOwner(), LogBehaviorTree, Log, TEXT("Parallel task aborted: %s (%s)"),
					*UBehaviorTreeTypes::DescribeNodeHelper(UpdateInfo.TaskNode),
					(NodeResult == EBTNodeResult::InProgress) ? TEXT("in progress") : TEXT("instant"));

				// mark as pending abort
				if (NodeResult == EBTNodeResult::InProgress)
				{
					UpdateInstance.ParallelTasks[ParallelTaskIdx].Status = EBTTaskStatus::Aborting;
					bWaitingForAbortingTasks = true;
				}

				OnTaskFinished(UpdateInfo.TaskNode, NodeResult);
			}
			else
			{
				UE_VLOG(GetOwner(), LogBehaviorTree, Verbose, TEXT("Parallel task: %s added to active list"),
					*UBehaviorTreeTypes::DescribeNodeHelper(UpdateInfo.TaskNode));

				UpdateInstance.ParallelTasks.Add(FBehaviorTreeParallelTask(UpdateInfo.TaskNode, EBTTaskStatus::Active));
			}
		}
	}
}

void UBehaviorTreeComponent::ApplySearchData(class UBTNode* NewActiveNode, int32 UpToIdx)
{
	const bool bFullUpdate = (UpToIdx < 0);
	const int32 MaxIdx = bFullUpdate ? SearchData.PendingUpdates.Num() : (UpToIdx + 1);
	const int32 NewNodeExecutionIndex = NewActiveNode ? NewActiveNode->GetExecutionIndex() : 0;
	ApplySearchUpdates(SearchData.PendingUpdates, MaxIdx, NewNodeExecutionIndex);

	// go though post updates (services) and clear both arrays if function was called without limit
	if (bFullUpdate)
	{
		ApplySearchUpdates(SearchData.PendingUpdates, SearchData.PendingUpdates.Num(), NewNodeExecutionIndex, true);
		SearchData.PendingUpdates.Reset();
	}
	else
	{
		SearchData.PendingUpdates.RemoveAt(0, UpToIdx + 1, false);
	}
}

void UBehaviorTreeComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	SCOPE_CYCLE_COUNTER(STAT_AI_BehaviorTree_Tick);

	check(this && this->IsPendingKill() == false);
		
	if (InstanceStack.Num() == 0 || !bIsRunning)
	{
		return;
	}
	
	// tick active auxiliary nodes and parallel tasks (in execution order, before task)
	for (int32 i = 0; i < InstanceStack.Num(); i++)
	{
		FBehaviorTreeInstance& InstanceInfo = InstanceStack[i];		
		for (int32 iAux = 0; iAux < InstanceInfo.ActiveAuxNodes.Num(); iAux++)
		{
			const UBTAuxiliaryNode* AuxNode = InstanceInfo.ActiveAuxNodes[iAux];
			uint8* NodeMemory = AuxNode->GetNodeMemory<uint8>(InstanceInfo);
			AuxNode->WrappedTickNode(this, NodeMemory, DeltaTime);
		}

		for (int32 iTask = 0; iTask < InstanceInfo.ParallelTasks.Num(); iTask++)
		{
			const UBTTaskNode* ParallelTask = InstanceInfo.ParallelTasks[iTask].TaskNode;
			uint8* NodeMemory = ParallelTask->GetNodeMemory<uint8>(InstanceInfo);
			ParallelTask->WrappedTickTask(this, NodeMemory, DeltaTime);
		}
	}

	// tick active task
	FBehaviorTreeInstance& ActiveInstance = InstanceStack[ActiveInstanceIdx];
	if (ActiveInstance.ActiveNodeType == EBTActiveNode::ActiveTask ||
		ActiveInstance.ActiveNodeType == EBTActiveNode::AbortingTask)
	{
		UBTTaskNode* ActiveTask = (UBTTaskNode*)ActiveInstance.ActiveNode;
		uint8* NodeMemory = ActiveTask->GetNodeMemory<uint8>(ActiveInstance);
		ActiveTask->WrappedTickTask(this, NodeMemory, DeltaTime);
	}
}

void UBehaviorTreeComponent::ProcessExecutionRequest()
{
	bRequestedFlowUpdate = false;
	if (bIsPaused)
	{
		UE_VLOG(GetOwner(), LogBehaviorTree, Verbose, TEXT("Ignoring ProcessExecutionRequest call due to BTComponent still being paused"));
		return;
	}

	EBTNodeResult::Type NodeResult = ExecutionRequest.ContinueWithResult;
	UBTTaskNode* NextTask = NULL;

	{
	SCOPE_CYCLE_COUNTER(STAT_AI_BehaviorTree_SearchTime);
	
	// abort task if needed
	if (InstanceStack[ActiveInstanceIdx].ActiveNodeType == EBTActiveNode::ActiveTask)
	{
		AbortCurrentTask();
	}

	// can't continue if current task is still aborting
	// it means, that all services and main tasks of parallels will be active!
	if (bWaitingForAbortingTasks)
	{
		return;
	}

	// deactivate up to ExecuteNode
	if (InstanceStack[ActiveInstanceIdx].ActiveNode != ExecutionRequest.ExecuteNode)
	{
		// DeactivateUpTo() call modifies ActiveNode & ActiveNodeType data, make sure that active task finished aborting first!
		const bool bDeactivated = DeactivateUpTo(ExecutionRequest.ExecuteNode, ExecutionRequest.ExecuteInstanceIdx, NodeResult);
		if (!bDeactivated)
		{
			return;
		}

		// find pending abort request with highest priority (search from end to optimize)
		// and apply all changes below, to be sure that tree is not executing
		// inactive tasks before searching for new one
		for (int32 i = SearchData.PendingUpdates.Num() - 1; i >= 0; i--)
		{
			FBehaviorTreeSearchUpdate& UpdateInfo = SearchData.PendingUpdates[i];
			if (UpdateInfo.TaskNode && UpdateInfo.Mode == EBTNodeUpdateMode::Remove)
			{
				// NewActiveNode param doesn't matter here, it's only for AddForLowerPri mode
				ApplySearchData(NULL, i);
				break;
			}
		}
	}

	// can't continue if parallel tasks are still aborting
	if (bWaitingForAbortingTasks)
	{
		return;
	}

	// make sure that we don't have any additional instances on stack
	// (ApplySearchData doesn't remove them, so parallels can abort latently)
	if (InstanceStack.Num() > (ActiveInstanceIdx + 1))
	{
		for (int32 i = ActiveInstanceIdx + 1; i < InstanceStack.Num(); i++)
		{
			InstanceStack[i].Cleanup(this);
		}

		InstanceStack.SetNum(ActiveInstanceIdx + 1);
	}

	FBehaviorTreeInstance& ActiveInstance = InstanceStack[ActiveInstanceIdx];
	UBTCompositeNode* TestNode = ExecutionRequest.ExecuteNode;

	// activate root node if needed (can't be handled by parent composite...)
	if (ActiveInstance.ActiveNode == NULL)
	{
		ActiveInstance.ActiveNode = InstanceStack[ActiveInstanceIdx].RootNode;
		ActiveInstance.RootNode->OnNodeActivation(SearchData);
		BT_SEARCHLOG(SearchData, Verbose, TEXT("Activated root node: %s"), *UBehaviorTreeTypes::DescribeNodeHelper(ActiveInstance.RootNode));
	}

	// additional operations for restarting:
	if (!ExecutionRequest.bTryNextChild)
	{
		// mark all decorators less important than current search start for removal
		UnregisterAuxNodesUpTo(ExecutionRequest.SearchStart);

		// reactivate top search node, so it could use search range correctly
		BT_SEARCHLOG(SearchData, Verbose, TEXT("Reactivate node: %s [restart]"), *UBehaviorTreeTypes::DescribeNodeHelper(TestNode));
		ExecutionRequest.ExecuteNode->OnNodeRestart(SearchData);

		SearchData.SearchStart = ExecutionRequest.SearchStart;
	}
	else
	{
		// make sure it's reset before starting new search
		SearchData.SearchStart = FBTNodeIndex();
	}

	// store blackboard values from search start (can be changed by aux node removal/adding)
#if USE_BEHAVIORTREE_DEBUGGER
	StoreDebuggerBlackboard(SearchStartBlackboard);
#endif

	// start looking for next task
	while (TestNode && NextTask == NULL)
	{
		BT_SEARCHLOG(SearchData, Verbose, TEXT("Testing node: %s"),	*UBehaviorTreeTypes::DescribeNodeHelper(TestNode));
		const int32 ChildBranchIdx = TestNode->FindChildToExecute(SearchData, NodeResult);
		UBTNode* StoreNode = TestNode;

		if (ChildBranchIdx == BTSpecialChild::ReturnToParent)
		{
			UBTCompositeNode* ChildNode = TestNode;
			TestNode = TestNode->GetParentNode();

			// does it want to move up the tree?
			if (TestNode == NULL)
			{
				// special case for leaving instance: deactivate root manually
				ChildNode->OnNodeDeactivation(SearchData, NodeResult);

				// don't remove top instance from stack, so it could be looped
				if (ActiveInstanceIdx > 0)
				{
					StoreDebuggerSearchStep(InstanceStack[ActiveInstanceIdx].ActiveNode, ActiveInstanceIdx, NodeResult);
					StoreDebuggerRemovedInstance(ActiveInstanceIdx);

					// apply pending aux changes (don't give any NewActiveNode param, AddForLowerPri updates should be skipped)
					ApplySearchData(NULL);

					// and leave subtree
					InstanceStack[ActiveInstanceIdx].Cleanup(this);
					InstanceStack.Pop();
					ActiveInstanceIdx--;

					StoreDebuggerSearchStep(InstanceStack[ActiveInstanceIdx].ActiveNode, ActiveInstanceIdx, NodeResult);
					TestNode = InstanceStack[ActiveInstanceIdx].ActiveNode->GetParentNode();
				}
			}

			if (TestNode)
			{
				TestNode->OnChildDeactivation(SearchData, ChildNode, NodeResult);
			}
		}
		else
		{
			// was new task found?
			NextTask = TestNode->Children[ChildBranchIdx].ChildTask;

			// or it wants to move down the tree?
			TestNode = TestNode->Children[ChildBranchIdx].ChildComposite;
		}

		// store after node deactivation had chance to modify result
		StoreDebuggerSearchStep(StoreNode, ActiveInstanceIdx, NodeResult);
	}

	// clear pending execution variables (applying search data can request new abort)
	ExecutionRequest = FBTNodeExecutionInfo();

	ApplySearchData(NextTask);

	// finish timer scope
	}

	// execute next task / notify out of nodes
	if (NextTask)
	{
		ExecuteTask(NextTask);
	}
	else
	{
		OnTreeFinished();
	}
}

bool UBehaviorTreeComponent::DeactivateUpTo(class UBTCompositeNode* Node, uint16 NodeInstanceIdx, EBTNodeResult::Type& NodeResult)
{
	UBTNode* DeactivatedChild = InstanceStack[ActiveInstanceIdx].ActiveNode;
	bool bDeactivateRoot = true;

	if (DeactivatedChild == NULL && ActiveInstanceIdx > NodeInstanceIdx)
	{
		// use tree's root node if instance didn't activated itself yet
		DeactivatedChild = InstanceStack[ActiveInstanceIdx].RootNode;
		bDeactivateRoot = false;
	}

	while (DeactivatedChild)
	{
		UBTCompositeNode* NotifyParent = DeactivatedChild->GetParentNode();
		if (NotifyParent)
		{
			NotifyParent->OnChildDeactivation(SearchData, DeactivatedChild, NodeResult);

			BT_SEARCHLOG(SearchData, Verbose, TEXT("Deactivate node: %s"), *UBehaviorTreeTypes::DescribeNodeHelper(DeactivatedChild));
			StoreDebuggerSearchStep(DeactivatedChild, ActiveInstanceIdx, NodeResult);
			DeactivatedChild = NotifyParent;

			FBehaviorTreeInstance& InstanceInfo = InstanceStack[ActiveInstanceIdx];
			InstanceInfo.ActiveNode = NotifyParent;
			InstanceInfo.ActiveNodeType = EBTActiveNode::Composite;
		}
		else
		{
			// special case for leaving instance: deactivate root manually
			if (bDeactivateRoot)
			{
				InstanceStack[ActiveInstanceIdx].RootNode->OnNodeDeactivation(SearchData, NodeResult);
			}

			BT_SEARCHLOG(SearchData, Verbose, TEXT("%s node: %s [leave subtree]"),
				bDeactivateRoot ? TEXT("Deactivate") : TEXT("Skip over"),
				*UBehaviorTreeTypes::DescribeNodeHelper(InstanceStack[ActiveInstanceIdx].RootNode));

			// clear flag, it's valid only for newest instance
			bDeactivateRoot = true;

			// shouldn't happen, but it's better to have built in failsafe just in case
			if (ActiveInstanceIdx == 0)
			{
				BT_SEARCHLOG(SearchData, Error, TEXT("Execution path does NOT contain common parent node, restarting tree! AI:%s"),
					*GetNameSafe(SearchData.OwnerComp->GetOwner()));

				RestartTree();
				return false;
			}

			ActiveInstanceIdx--;
			DeactivatedChild = InstanceStack[ActiveInstanceIdx].ActiveNode;

			// apply aux node changes from removed subtree
			// (don't give any NewActiveNode param, AddForLowerPri updates should be skipped)
			ApplySearchData(NULL);
		}

		if (DeactivatedChild == Node)
		{
			break;
		}
	}

	return true;
}

void UBehaviorTreeComponent::UnregisterAuxNodesUpTo(const FBTNodeIndex& Index)
{
	for (int32 i = 0; i < InstanceStack.Num(); i++)
	{
		FBehaviorTreeInstance& InstanceInfo = InstanceStack[i];
		for (int32 iAux = 0; iAux < InstanceInfo.ActiveAuxNodes.Num(); iAux++)
		{
			FBTNodeIndex AuxIdx(i, InstanceInfo.ActiveAuxNodes[iAux]->GetExecutionIndex());
			if (Index.TakesPriorityOver(AuxIdx))
			{
				SearchData.AddUniqueUpdate(FBehaviorTreeSearchUpdate(InstanceInfo.ActiveAuxNodes[iAux], i, EBTNodeUpdateMode::Remove));
			}
		}
	}
}

void UBehaviorTreeComponent::ExecuteTask(UBTTaskNode* TaskNode)
{
	SCOPE_CYCLE_COUNTER(STAT_AI_BehaviorTree_ExecutionTime);

	FBehaviorTreeInstance& ActiveInstance = InstanceStack[ActiveInstanceIdx];
	ActiveInstance.ActiveNode = TaskNode;
	ActiveInstance.ActiveNodeType = EBTActiveNode::ActiveTask;

	// make a snapshot for debugger
	StoreDebuggerExecutionStep(EBTExecutionSnap::Regular);

	UE_VLOG(GetOwner(), LogBehaviorTree, Log, TEXT("Execute task: %s"), *UBehaviorTreeTypes::DescribeNodeHelper(TaskNode));

	// store instance before execution, it could result in pushing a subtree
	uint16 InstanceIdx = ActiveInstanceIdx;

	uint8* NodeMemory = (uint8*)(TaskNode->GetNodeMemory<uint8>(ActiveInstance));
	EBTNodeResult::Type TaskResult = TaskNode->WrappedExecuteTask(this, NodeMemory);

	// update task's runtime values after it had a chance to initialize memory
	UpdateDebuggerAfterExecution(TaskNode, InstanceIdx);

	OnTaskFinished(TaskNode, TaskResult);
}

void UBehaviorTreeComponent::AbortCurrentTask()
{
	FBehaviorTreeInstance& ActiveInstance = InstanceStack[ActiveInstanceIdx];
	UBTTaskNode* ActiveTask = (UBTTaskNode*)ActiveInstance.ActiveNode;

	// remove all observers before requesting abort
	UnregisterMessageObserversFrom(ActiveTask);

	UE_VLOG(GetOwner(), LogBehaviorTree, Log, TEXT("Abort task: %s"), *UBehaviorTreeTypes::DescribeNodeHelper(ActiveTask));

	// abort task using current state of tree
	uint8* NodeMemory = (uint8*)(ActiveTask->GetNodeMemory<uint8>(ActiveInstance));
	EBTNodeResult::Type TaskResult = ActiveTask->WrappedAbortTask(this, NodeMemory);

	ActiveInstance.ActiveNodeType = EBTActiveNode::AbortingTask;

	OnTaskFinished(ActiveTask, TaskResult);
}

void UBehaviorTreeComponent::RegisterMessageObserver(const class UBTTaskNode* TaskNode, FName MessageType)
{
	if (TaskNode)
	{
		FBTNodeIndex NodeIdx;
		NodeIdx.ExecutionIndex = TaskNode->GetExecutionIndex();
		NodeIdx.InstanceIndex = InstanceStack.Num() - 1;

		TaskMessageObservers.Add(NodeIdx,
			FAIMessageObserver::Create(this, MessageType, FOnAIMessage::CreateUObject(TaskNode, &UBTTaskNode::ReceivedMessage))
			);

		UE_VLOG(GetOwner(), LogBehaviorTree, Log, TEXT("Message[%s] observer added for %s"),
			*MessageType.ToString(), *UBehaviorTreeTypes::DescribeNodeHelper(TaskNode));
	}
}

void UBehaviorTreeComponent::RegisterMessageObserver(const class UBTTaskNode* TaskNode, FName MessageType, FAIRequestID RequestID)
{
	if (TaskNode)
	{
		FBTNodeIndex NodeIdx;
		NodeIdx.ExecutionIndex = TaskNode->GetExecutionIndex();
		NodeIdx.InstanceIndex = InstanceStack.Num() - 1;

		TaskMessageObservers.Add(NodeIdx,
			FAIMessageObserver::Create(this, MessageType, RequestID, FOnAIMessage::CreateUObject(TaskNode, &UBTTaskNode::ReceivedMessage))
			);

		UE_VLOG(GetOwner(), LogBehaviorTree, Log, TEXT("Message[%s:%d] observer added for %s"),
			*MessageType.ToString(), RequestID, *UBehaviorTreeTypes::DescribeNodeHelper(TaskNode));
	}
}

void UBehaviorTreeComponent::UnregisterMessageObserversFrom(const FBTNodeIndex& TaskIdx)
{
	const int32 NumRemoved = TaskMessageObservers.Remove(TaskIdx);
	if (NumRemoved)
	{
		UE_VLOG(GetOwner(), LogBehaviorTree, Log, TEXT("Message observers removed for task[%d:%d] (num:%d)"),
			TaskIdx.InstanceIndex, TaskIdx.ExecutionIndex, NumRemoved);
	}
}

void UBehaviorTreeComponent::UnregisterMessageObserversFrom(const class UBTTaskNode* TaskNode)
{
	if (TaskNode)
	{
		const FBehaviorTreeInstance& ActiveInstance = InstanceStack.Last();

		FBTNodeIndex NodeIdx;
		NodeIdx.ExecutionIndex = TaskNode->GetExecutionIndex();
		NodeIdx.InstanceIndex = FindInstanceContainingNode(TaskNode);
		
		UnregisterMessageObserversFrom(NodeIdx);
	}
}

#if STATS
#define AUX_NODE_WRAPPER(cmd) \
	DEC_MEMORY_STAT_BY(STAT_AI_BehaviorTree_InstanceMemory, InstanceInfo.GetAllocatedSize()); \
	cmd; \
	INC_MEMORY_STAT_BY(STAT_AI_BehaviorTree_InstanceMemory, InstanceInfo.GetAllocatedSize());

#else
#define AUX_NODE_WRAPPER(cmd) cmd;
#endif // STATS

void UBehaviorTreeComponent::RegisterParallelTask(const class UBTTaskNode* TaskNode)
{
	FBehaviorTreeInstance& InstanceInfo = InstanceStack[ActiveInstanceIdx];
	AUX_NODE_WRAPPER(InstanceInfo.ParallelTasks.Add( FBehaviorTreeParallelTask(TaskNode, EBTTaskStatus::Active) ));
	
	UE_VLOG(GetOwner(), LogBehaviorTree, Verbose, TEXT("Parallel task: %s added to active list"),
		*UBehaviorTreeTypes::DescribeNodeHelper(TaskNode));

	if (InstanceInfo.ActiveNode == TaskNode)
	{
		// switch to inactive state, so it could start background tree
		InstanceInfo.ActiveNodeType = EBTActiveNode::InactiveTask;
	}
}

void UBehaviorTreeComponent::UnregisterParallelTask(const class UBTTaskNode* TaskNode, uint16 InstanceIdx)
{
	bool bShouldUpdate = false;
	if (InstanceStack.IsValidIndex(InstanceIdx))
	{
		FBehaviorTreeInstance& InstanceInfo = InstanceStack[InstanceIdx];
		for (int32 iTask = InstanceInfo.ParallelTasks.Num() - 1; iTask >= 0; iTask--)
		{
			FBehaviorTreeParallelTask& ParallelInfo = InstanceInfo.ParallelTasks[iTask];
			if (ParallelInfo.TaskNode == TaskNode)
			{
				UE_VLOG(GetOwner(), LogBehaviorTree, Verbose, TEXT("Parallel task: %s removed from active list"),
					*UBehaviorTreeTypes::DescribeNodeHelper(TaskNode));

				InstanceInfo.ParallelTasks.RemoveAt(iTask);
				bShouldUpdate = true;
				break;
			}
		}
	}

	if (bShouldUpdate)
	{
		UpdateAbortingTasks();
	}
}

#undef AUX_NODE_WRAPPER

void UBehaviorTreeComponent::UpdateAbortingTasks()
{
	bWaitingForAbortingTasks = InstanceStack.Num() ? (InstanceStack.Last().ActiveNodeType == EBTActiveNode::AbortingTask) : false;

	for (int32 i = 0; i < InstanceStack.Num() && !bWaitingForAbortingTasks; i++)
	{
		FBehaviorTreeInstance& InstanceInfo = InstanceStack[i];
		for (int32 iTask = InstanceInfo.ParallelTasks.Num() - 1; iTask >= 0; iTask--)
		{
			FBehaviorTreeParallelTask& ParallelInfo = InstanceInfo.ParallelTasks[iTask];
			if (ParallelInfo.Status == EBTTaskStatus::Aborting)
			{
				bWaitingForAbortingTasks = true;
				break;
			}
		}
	}
}

bool UBehaviorTreeComponent::PushInstance(class UBehaviorTree* TreeAsset)
{
	if (TreeAsset == NULL)
	{
		return false;
	}

	// check if blackboard class match
	if (TreeAsset->BlackboardAsset && BlackboardComp && !BlackboardComp->IsCompatibileWith(TreeAsset->BlackboardAsset))
	{
		UE_VLOG(GetOwner(), LogBehaviorTree, Warning, TEXT("Failed to execute tree %s: blackboard %s is not compatibile with current: %s!"),
			*GetNameSafe(TreeAsset), *GetNameSafe(TreeAsset->BlackboardAsset), *GetNameSafe(BlackboardComp->GetBlackboardAsset()));

		return false;
	}

	UBehaviorTreeManager* BTManager = GetWorld()->GetBehaviorTreeManager();
	if (BTManager == NULL)
	{
		UE_VLOG(GetOwner(), LogBehaviorTree, Warning, TEXT("Failed to execute tree %s: behavior tree manager not found!"), *GetNameSafe(TreeAsset));
		return false;
	}

	// check if parent node allows it
	const UBTNode* ActiveNode = GetActiveNode();
	const UBTCompositeNode* ActiveParent = ActiveNode ? ActiveNode->GetParentNode() : NULL;
	if (ActiveParent)
	{
		uint8* ParentMemory = GetNodeMemory((UBTNode*)ActiveParent, InstanceStack.Num() - 1);
		int32 ChildIdx = ActiveParent->GetChildIndex(ActiveNode);

		const bool bIsAllowed = ActiveParent->CanPushSubtree(this, ParentMemory, ChildIdx);
		if (!bIsAllowed)
		{
			UE_VLOG(GetOwner(), LogBehaviorTree, Warning, TEXT("Failed to execute tree %s: parent of active node does not allow it! (%s)"),
				*GetNameSafe(TreeAsset), *UBehaviorTreeTypes::DescribeNodeHelper(ActiveParent));
			return false;
		}
	}

	UBTCompositeNode* RootNode = NULL;
	uint16 InstanceMemorySize = 0;

	const bool bLoaded = BTManager->LoadTree(TreeAsset, RootNode, InstanceMemorySize);
	if (bLoaded)
	{
		FBehaviorTreeInstance NewInstance(InstanceMemorySize);
		NewInstance.InstanceIdIndex = UpdateInstanceId(TreeAsset, ActiveNode, InstanceStack.Num() - 1);
		NewInstance.RootNode = RootNode;
		NewInstance.ActiveNode = NULL;
		NewInstance.ActiveNodeType = EBTActiveNode::Composite;

		// initialize memory and node instances
		FBehaviorTreeInstanceId& InstanceInfo = KnownInstances[NewInstance.InstanceIdIndex];
		int32 NodeInstanceIndex = InstanceInfo.FirstNodeInstance;
		NewInstance.Initialize(this, RootNode, NodeInstanceIndex);
		NewInstance.InjectNodes(this, RootNode, NodeInstanceIndex);

		INC_DWORD_STAT(STAT_AI_BehaviorTree_NumInstances);
		InstanceStack.Push(NewInstance);
		ActiveInstanceIdx = InstanceStack.Num() - 1;

		// start root level services now (they won't be removed on looping tree anyway)
		for (int32 i = 0; i < RootNode->Services.Num(); i++)
		{
			UBTService* ServiceNode = RootNode->Services[i];
			uint8* NodeMemory = (uint8*)ServiceNode->GetNodeMemory<uint8>(InstanceStack[ActiveInstanceIdx]);

			InstanceStack[ActiveInstanceIdx].ActiveAuxNodes.Add(ServiceNode);
			ServiceNode->WrappedOnBecomeRelevant(this, NodeMemory);
		}

		FBehaviorTreeDelegates::OnTreeStarted.Broadcast(this, TreeAsset);

		// start new task
		RequestExecution(RootNode, ActiveInstanceIdx, RootNode, 0, EBTNodeResult::InProgress);
		return true;
	}

	return false;
}

uint8 UBehaviorTreeComponent::UpdateInstanceId(class UBehaviorTree* TreeAsset, const UBTNode* OriginNode, int32 OriginInstanceIdx)
{
	FBehaviorTreeInstanceId InstanceId;
	InstanceId.TreeAsset = TreeAsset;

	// build path from origin node
	{
		const uint16 ExecutionIndex = OriginNode ? OriginNode->GetExecutionIndex() : MAX_uint16;
		InstanceId.Path.Add(ExecutionIndex);
	}

	for (int32 i = OriginInstanceIdx - 1; i >= 0; i--)
	{
		const uint16 ExecutionIndex = InstanceStack[i].ActiveNode ? InstanceStack[i].ActiveNode->GetExecutionIndex() : MAX_uint16;
		InstanceId.Path.Add(ExecutionIndex);
	}

	// try to find matching existing Id
	for (int32 i = 0; i < KnownInstances.Num(); i++)
	{
		if (KnownInstances[i] == InstanceId)
		{
			return i;
		}
	}

	// add new one
	InstanceId.FirstNodeInstance = NodeInstances.Num();

	const int32 NewIndex = KnownInstances.Add(InstanceId);
	check(NewIndex < MAX_uint8);
	return NewIndex;
}

int32 UBehaviorTreeComponent::FindInstanceContainingNode(const UBTNode* Node) const
{
	int32 InstanceIdx = INDEX_NONE;

	const UBTNode* TemplateNode = FindTemplateNode(Node);
	if (TemplateNode && InstanceStack.Num())
	{
		if (InstanceStack[ActiveInstanceIdx].ActiveNode != TemplateNode)
		{
			const UBTNode* RootNode = TemplateNode;
			while (RootNode->GetParentNode())
			{
				RootNode = RootNode->GetParentNode();
			}

			for (int32 i = 0; i < InstanceStack.Num(); i++)
			{
				if (InstanceStack[i].RootNode == RootNode)
				{
					InstanceIdx = i;
					break;
				}
			}
		}
		else
		{
			InstanceIdx = ActiveInstanceIdx;
		}
	}

	return InstanceIdx;
}

class UBTNode* UBehaviorTreeComponent::FindTemplateNode(const class UBTNode* Node) const
{
	if (Node == NULL || !Node->IsInstanced() || Node->GetParentNode() == NULL)
	{
		return (UBTNode*)Node;
	}

	UBTCompositeNode* ParentNode = Node->GetParentNode();
	for (int32 iChild = 0; iChild < ParentNode->Children.Num(); iChild++)
	{
		FBTCompositeChild& ChildInfo = ParentNode->Children[iChild];

		if (ChildInfo.ChildTask && ChildInfo.ChildTask->GetExecutionIndex() == Node->GetExecutionIndex())
		{
			return ChildInfo.ChildTask;
		}

		for (int32 i = 0; i < ChildInfo.Decorators.Num(); i++)
		{
			if (ChildInfo.Decorators[i]->GetExecutionIndex() == Node->GetExecutionIndex())
			{
				return ChildInfo.Decorators[i];
			}
		}
	}

	for (int32 i = 0; i < ParentNode->Services.Num(); i++)
	{
		if (ParentNode->Services[i]->GetExecutionIndex() == Node->GetExecutionIndex())
		{
			return ParentNode->Services[i];
		}
	}

	return NULL;
}

uint8* UBehaviorTreeComponent::GetNodeMemory(UBTNode* Node, int32 InstanceIdx) const
{
	return InstanceStack.IsValidIndex(InstanceIdx) ? (uint8*)Node->GetNodeMemory<uint8>(InstanceStack[InstanceIdx]) : NULL;
}

FString UBehaviorTreeComponent::GetDebugInfoString() const 
{ 
	FString DebugInfo;
	for (int32 i = 0; i < InstanceStack.Num(); i++)
	{
		const FBehaviorTreeInstance& InstanceData = InstanceStack[i];
		const FBehaviorTreeInstanceId& InstanceInfo = KnownInstances[InstanceData.InstanceIdIndex];
		DebugInfo += FString::Printf(TEXT("Behavior tree: %s\n"), *GetNameSafe(InstanceInfo.TreeAsset));

		UBTNode* Node = InstanceData.ActiveNode;
		FString NodeTrace;

		while (Node)
		{
			uint8* NodeMemory = (uint8*)(Node->GetNodeMemory<uint8>(InstanceData));
			NodeTrace = FString::Printf(TEXT("  %s\n"), *Node->GetRuntimeDescription(this, NodeMemory, EBTDescriptionVerbosity::Basic)) + NodeTrace;
			Node = Node->GetParentNode();
		}

		DebugInfo += NodeTrace;
	}

	return DebugInfo;
}

FString UBehaviorTreeComponent::DescribeActiveTasks() const
{
	FString ActiveTask(TEXT("None"));
	if (InstanceStack.Num())
	{
		const FBehaviorTreeInstance& LastInstance = InstanceStack.Last();
		if (LastInstance.ActiveNodeType == EBTActiveNode::ActiveTask)
		{
			ActiveTask = UBehaviorTreeTypes::DescribeNodeHelper(LastInstance.ActiveNode);
		}
	}

	FString ParallelTasks;
	for (int32 i = 0; i < InstanceStack.Num(); i++)
	{
		const FBehaviorTreeInstance& InstanceInfo = InstanceStack[i];
		for (int32 iParallel = 0; iParallel < InstanceInfo.ParallelTasks.Num(); iParallel++)
		{
			const FBehaviorTreeParallelTask& ParallelInfo = InstanceInfo.ParallelTasks[iParallel];
			if (ParallelInfo.Status == EBTTaskStatus::Active)
			{
				ParallelTasks += UBehaviorTreeTypes::DescribeNodeHelper(ParallelInfo.TaskNode);
				ParallelTasks += TEXT(", ");
			}
		}
	}

	if (ParallelTasks.Len() > 0)
	{
		ActiveTask += TEXT(" (");
		ActiveTask += ParallelTasks.LeftChop(2);
		ActiveTask += TEXT(')');
	}

	return ActiveTask;
}

FString UBehaviorTreeComponent::DescribeActiveTrees() const
{
	FString Assets;
	for (int32 i = 0; i < InstanceStack.Num(); i++)
	{
		const FBehaviorTreeInstanceId& InstanceInfo = KnownInstances[InstanceStack[i].InstanceIdIndex];
		Assets += InstanceInfo.TreeAsset->GetName();
		Assets += TEXT(", ");
	}

	return Assets.Len() ? Assets.LeftChop(2) : TEXT("None");
}

#if ENABLE_VISUAL_LOG
void UBehaviorTreeComponent::DescribeSelfToVisLog(struct FVisLogEntry* Snapshot) const
{
	if (IsPendingKill())
	{
		return;
	}
	
	Super::DescribeSelfToVisLog(Snapshot);

	for (int32 i = 0; i < InstanceStack.Num(); i++)
	{
		const FBehaviorTreeInstance& InstanceInfo = InstanceStack[i];
		const FBehaviorTreeInstanceId& InstanceId = KnownInstances[InstanceInfo.InstanceIdIndex];

		FVisLogEntry::FStatusCategory StatusCategory;
		StatusCategory.Category = FString::Printf(TEXT("BehaviorTree %d (asset: %s)"), i, *GetNameSafe(InstanceId.TreeAsset));
		
		TArray<FString> Descriptions;
		UBTNode* Node = InstanceInfo.ActiveNode;
		while (Node)
		{
			uint8* NodeMemory = (uint8*)(Node->GetNodeMemory<uint8>(InstanceInfo));
			Descriptions.Add(Node->GetRuntimeDescription(this, NodeMemory, EBTDescriptionVerbosity::Detailed));
		
			Node = Node->GetParentNode();
		}

		for (int32 iDesc = Descriptions.Num() - 1; iDesc >= 0; iDesc--)
		{
			int32 SplitIdx = INDEX_NONE;
			if (Descriptions[iDesc].FindChar(TEXT(','), SplitIdx))
			{
				const FString KeyDesc = Descriptions[iDesc].Left(SplitIdx);
				const FString ValueDesc = Descriptions[iDesc].Mid(SplitIdx + 1).Trim();

				StatusCategory.Add(KeyDesc, ValueDesc);
			}
			else
			{
				StatusCategory.Add(Descriptions[iDesc], TEXT(""));
			}
		}

		if (StatusCategory.Data.Num() == 0)
		{
			StatusCategory.Add(TEXT("root"), TEXT("not initialized"));
		}

		Snapshot->Status.Add(StatusCategory);
	}
}
#endif // ENABLE_VISUAL_LOG

void UBehaviorTreeComponent::StoreDebuggerExecutionStep(EBTExecutionSnap::Type SnapType)
{
#if USE_BEHAVIORTREE_DEBUGGER
	FBehaviorTreeExecutionStep CurrentStep;
	CurrentStep.StepIndex = DebuggerSteps.Num() ? DebuggerSteps.Last().StepIndex + 1 : 0;
	CurrentStep.TimeStamp = GetWorld()->GetTimeSeconds();
	CurrentStep.BlackboardValues = SearchStartBlackboard;

	for (int32 i = 0; i < InstanceStack.Num(); i++)
	{
		const FBehaviorTreeInstance& ActiveInstance = InstanceStack[i];
		
		FBehaviorTreeDebuggerInstance StoreInfo;
		StoreDebuggerInstance(StoreInfo, i, SnapType);
		CurrentStep.InstanceStack.Add(StoreInfo);
	}

	for (int32 i = RemovedInstances.Num() - 1; i >= 0; i--)
	{
		CurrentStep.InstanceStack.Add(RemovedInstances[i]);
	}

	CurrentSearchFlow.Reset();
	CurrentRestarts.Reset();
	RemovedInstances.Reset();

	UBehaviorTreeManager* ManagerCDO = (UBehaviorTreeManager*)UBehaviorTreeManager::StaticClass()->GetDefaultObject();
	while (DebuggerSteps.Num() >= ManagerCDO->MaxDebuggerSteps)
	{
		DebuggerSteps.RemoveAt(0);
	}
	DebuggerSteps.Add(CurrentStep);
#endif
}

void UBehaviorTreeComponent::StoreDebuggerInstance(FBehaviorTreeDebuggerInstance& InstanceInfo, uint16 InstanceIdx, EBTExecutionSnap::Type SnapType) const
{
#if USE_BEHAVIORTREE_DEBUGGER
	if (!InstanceStack.IsValidIndex(InstanceIdx))
	{
		return;
	}

	const FBehaviorTreeInstance& ActiveInstance = InstanceStack[InstanceIdx];
	const FBehaviorTreeInstanceId& ActiveInstanceInfo = KnownInstances[ActiveInstance.InstanceIdIndex];
	InstanceInfo.TreeAsset = ActiveInstanceInfo.TreeAsset;
	InstanceInfo.RootNode = ActiveInstance.RootNode;

	if (SnapType == EBTExecutionSnap::Regular)
	{
		// traverse execution path
		UBTNode* StoreNode = ActiveInstance.ActiveNode ? ActiveInstance.ActiveNode : ActiveInstance.RootNode;
		while (StoreNode)
		{
			InstanceInfo.ActivePath.Add(StoreNode->GetExecutionIndex());
			StoreNode = StoreNode->GetParentNode();
		}

		// add aux nodes
		for (int32 i = 0; i < ActiveInstance.ActiveAuxNodes.Num(); i++)
		{
			InstanceInfo.AdditionalActiveNodes.Add(ActiveInstance.ActiveAuxNodes[i]->GetExecutionIndex());
		}

		// add active parallels
		for (int32 i = 0; i < ActiveInstance.ParallelTasks.Num(); i++)
		{
			const FBehaviorTreeParallelTask& TaskInfo = ActiveInstance.ParallelTasks[i];
			InstanceInfo.AdditionalActiveNodes.Add(TaskInfo.TaskNode->GetExecutionIndex());
		}

		// runtime values
		StoreDebuggerRuntimeValues(InstanceInfo.RuntimeDesc, ActiveInstance.RootNode, InstanceIdx);
	}

	// handle restart triggers
	if (CurrentRestarts.IsValidIndex(InstanceIdx))
	{
		InstanceInfo.PathFromPrevious = CurrentRestarts[InstanceIdx];
	}

	// store search flow, but remove nodes on execution path
	if (CurrentSearchFlow.IsValidIndex(InstanceIdx))
	{
		for (int32 i = 0; i < CurrentSearchFlow[InstanceIdx].Num(); i++)
		{
			if (!InstanceInfo.ActivePath.Contains(CurrentSearchFlow[InstanceIdx][i].ExecutionIndex))
			{
				InstanceInfo.PathFromPrevious.Add(CurrentSearchFlow[InstanceIdx][i]);
			}
		}
	}
#endif
}

void UBehaviorTreeComponent::StoreDebuggerRemovedInstance(uint16 InstanceIdx) const
{
#if USE_BEHAVIORTREE_DEBUGGER
	FBehaviorTreeDebuggerInstance StoreInfo;
	StoreDebuggerInstance(StoreInfo, InstanceIdx, EBTExecutionSnap::OutOfNodes);

	RemovedInstances.Add(StoreInfo);
#endif
}

void UBehaviorTreeComponent::StoreDebuggerSearchStep(const class UBTNode* Node, uint16 InstanceIdx, EBTNodeResult::Type NodeResult) const
{
#if USE_BEHAVIORTREE_DEBUGGER
	if (Node && NodeResult != EBTNodeResult::InProgress && NodeResult != EBTNodeResult::Aborted)
	{
		FBehaviorTreeDebuggerInstance::FNodeFlowData FlowInfo;
		FlowInfo.ExecutionIndex = Node->GetExecutionIndex();
		FlowInfo.bPassed = (NodeResult == EBTNodeResult::Succeeded);
		FlowInfo.bOptional = (NodeResult == EBTNodeResult::Optional);
		
		if (CurrentSearchFlow.Num() < (InstanceIdx + 1))
		{
			CurrentSearchFlow.SetNum(InstanceIdx + 1);
		}

		if (CurrentSearchFlow[InstanceIdx].Num() == 0 || CurrentSearchFlow[InstanceIdx].Last().ExecutionIndex != FlowInfo.ExecutionIndex)
		{
			CurrentSearchFlow[InstanceIdx].Add(FlowInfo);
		}
	}
#endif
}

void UBehaviorTreeComponent::StoreDebuggerSearchStep(const class UBTNode* Node, uint16 InstanceIdx, bool bPassed) const
{
#if USE_BEHAVIORTREE_DEBUGGER
	if (Node && !bPassed)
	{
		FBehaviorTreeDebuggerInstance::FNodeFlowData FlowInfo;
		FlowInfo.ExecutionIndex = Node->GetExecutionIndex();
		FlowInfo.bPassed = bPassed;

		if (CurrentSearchFlow.Num() < (InstanceIdx + 1))
		{
			CurrentSearchFlow.SetNum(InstanceIdx + 1);
		}

		CurrentSearchFlow[InstanceIdx].Add(FlowInfo);
	}
#endif
}

void UBehaviorTreeComponent::StoreDebuggerRestart(const class UBTNode* Node, uint16 InstanceIdx, bool bAllowed)
{
#if USE_BEHAVIORTREE_DEBUGGER
	if (Node)
	{
		FBehaviorTreeDebuggerInstance::FNodeFlowData FlowInfo;
		FlowInfo.ExecutionIndex = Node->GetExecutionIndex();
		FlowInfo.bTrigger = bAllowed;
		FlowInfo.bDiscardedTrigger = !bAllowed;

		if (CurrentRestarts.Num() < (InstanceIdx + 1))
		{
			CurrentRestarts.SetNum(InstanceIdx + 1);
		}

		CurrentRestarts[InstanceIdx].Add(FlowInfo);
	}
#endif
}

void UBehaviorTreeComponent::StoreDebuggerRuntimeValues(TArray<FString>& RuntimeDescriptions, class UBTNode* RootNode, uint16 InstanceIdx) const
{
#if USE_BEHAVIORTREE_DEBUGGER
	if (!InstanceStack.IsValidIndex(InstanceIdx))
	{
		return;
	}

	const FBehaviorTreeInstance& InstanceInfo = InstanceStack[InstanceIdx];

	TArray<FString> RuntimeValues;
	for (UBTNode* Node = RootNode; Node; Node = Node->GetNextNode())
	{
		uint8* NodeMemory = (uint8*)Node->GetNodeMemory<uint8>(InstanceInfo);

		RuntimeValues.Reset();
		Node->DescribeRuntimeValues(this, NodeMemory, EBTDescriptionVerbosity::Basic, RuntimeValues);

		FString ComposedDesc;
		for (int32 i = 0; i < RuntimeValues.Num(); i++)
		{
			if (ComposedDesc.Len())
			{
				ComposedDesc.AppendChar(TEXT('\n'));
			}

			ComposedDesc += RuntimeValues[i];
		}

		RuntimeDescriptions.SetNum(Node->GetExecutionIndex() + 1);
		RuntimeDescriptions[Node->GetExecutionIndex()] = ComposedDesc;
	}
#endif
}

void UBehaviorTreeComponent::UpdateDebuggerAfterExecution(const class UBTTaskNode* TaskNode, uint16 InstanceIdx) const
{
#if USE_BEHAVIORTREE_DEBUGGER
	FBehaviorTreeExecutionStep& CurrentStep = DebuggerSteps.Last();

	// store runtime values
	TArray<FString> RuntimeValues;
	const FBehaviorTreeInstance& InstanceToUpdate = InstanceStack[InstanceIdx];
	uint8* NodeMemory = (uint8*)TaskNode->GetNodeMemory<uint8>(InstanceToUpdate);
	TaskNode->DescribeRuntimeValues(this, NodeMemory, EBTDescriptionVerbosity::Basic, RuntimeValues);

	FString ComposedDesc;
	for (int32 i = 0; i < RuntimeValues.Num(); i++)
	{
		if (ComposedDesc.Len())
		{
			ComposedDesc.AppendChar(TEXT('\n'));
		}

		ComposedDesc += RuntimeValues[i];
	}

	// accessing RuntimeDesc should never be out of bounds (active task MUST be part of active instance)
	const uint16& ExecutionIndex = TaskNode->GetExecutionIndex();
	CurrentStep.InstanceStack[InstanceIdx].RuntimeDesc[ExecutionIndex] = ComposedDesc;
#endif
}

void UBehaviorTreeComponent::StoreDebuggerBlackboard(TArray<FString>& BlackboardValueDesc) const
{
#if USE_BEHAVIORTREE_DEBUGGER
	if (BlackboardComp && BlackboardComp->HasValidAsset())
	{
		const int32 NumKeys = BlackboardComp->GetNumKeys();
		BlackboardValueDesc.Reset();
		BlackboardValueDesc.AddZeroed(NumKeys);

		for (int32 i = 0; i < NumKeys; i++)
		{
			BlackboardValueDesc[i] = BlackboardComp->DescribeKeyValue(i, EBlackboardDescription::OnlyValue);
			
			if (BlackboardValueDesc[i].Len() == 0)
			{
				BlackboardValueDesc[i] = TEXT("n/a");
			}
		}
	}
#endif
}
