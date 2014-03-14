// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "AI/BehaviorTreeDelegates.h"

//----------------------------------------------------------------------//
// UBehaviorTreeComponent
//----------------------------------------------------------------------//

UBehaviorTreeComponent::UBehaviorTreeComponent(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	ActiveInstanceIdx = 0;
	bLoopExecution = false;
	bWaitingForParallelTasks = false;
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

void UBehaviorTreeComponent::ResumeLogic(const FString& Reason)
{
	UE_VLOG(GetOwner(), LogBehaviorTree, Log, TEXT("Execution updates: RESUMED (%s)"), *Reason);
	bIsPaused = false;

	if (BlackboardComp)
	{
		BlackboardComp->ResumeUpdates();
	}

	if (ExecutionRequest.ExecuteNode)
	{
		ScheduleExecutionUpdate();
	}
}

bool UBehaviorTreeComponent::IsRunning() const
{ 
	return bIsRunning && InstanceStack.Num();
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
	
	if (CurrentRoot == TreeAsset && IsRunning())
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
	if (bPushed)
	{
		FBehaviorTreeDelegates::OnTreeStarted.Broadcast(this, TreeAsset);
	}

	return bPushed;
}

void UBehaviorTreeComponent::StopTree()
{
	// clear current state, don't touch debugger data
	InstanceStack.Reset();
	TaskMessageObservers.Reset();
	ExecutionRequest = FBTNodeExecutionInfo();
	ActiveInstanceIdx = 0;
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

	const bool bWasWaitingForParallel = bWaitingForParallelTasks;
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

		// make sure that we continue execution after all pending latent aborts finished
		if (!bWaitingForParallelTasks && bWasWaitingForParallel)
		{
			ScheduleExecutionUpdate();
		}
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

	const FBehaviorTreeInstance& TestInstance = InstanceStack[TestInstanceIdx];
	if (Node == TestInstance.RootNode || Node == TestInstance.ActiveNode)
	{
		return true;
	}

	const uint16 ActiveExecutionIndex = TestInstance.ActiveNode->GetExecutionIndex();
	const uint16 NextChildExecutionIndex = Node->GetParentNode()->GetChildExecutionIndex(ChildIndex + 1);
	return (ActiveExecutionIndex >= Node->GetExecutionIndex()) && (ActiveExecutionIndex < NextChildExecutionIndex);
}

EBTTaskStatus::Type UBehaviorTreeComponent::GetTaskStatus(const class UBTTaskNode* TaskNode) const
{
	EBTTaskStatus::Type Status = EBTTaskStatus::Inactive;
	const int32 InstanceIdx = FindInstanceContainingNode(TaskNode);
	
	if (InstanceIdx != INDEX_NONE)
	{
		const FBehaviorTreeInstance& InstanceInfo = InstanceStack[InstanceIdx];

		// always check parallel execution first, it takes priority over ActiveNodeType
		for (int32 i = 0; i < InstanceInfo.ParallelTasks.Num(); i++)
		{
			const FBehaviorTreeParallelTask& ParallelInfo = InstanceInfo.ParallelTasks[i];
			if (ParallelInfo.TaskNode == TaskNode)
			{
				Status = ParallelInfo.Status;
				break;
			}
		}

		if (InstanceInfo.ActiveNode == TaskNode && Status == EBTTaskStatus::Inactive)
		{
			Status =
				(InstanceInfo.ActiveNodeType == EBTActiveNode::ActiveTask) ? EBTTaskStatus::Active :
				(InstanceInfo.ActiveNodeType == EBTActiveNode::AbortingTask) ? EBTTaskStatus::Aborting :
				EBTTaskStatus::Inactive;
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
	//         UNLESS condition evaluates as pass, in which case it will restart itself (nodes under decorator)
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

	if (AbortMode == EBTFlowAbortMode::Both)
	{
		const bool bIsExecutingChildNodes = IsExecutingBranch(RequestedBy, RequestedBy->GetChildIndex());
		AbortMode = bIsExecutingChildNodes ? EBTFlowAbortMode::Self : EBTFlowAbortMode::LowerPriority;
	}

	EBTNodeResult::Type ContinueResult = EBTNodeResult::Failed;
	if (AbortMode == EBTFlowAbortMode::Self)
	{
		uint8* NodeMemory = RequestedBy->GetNodeMemory<uint8>(InstanceStack[InstanceIdx]);
		const bool bCanRestartMyBranch = RequestedBy->IsRestartChildOnlyAllowed() && RequestedBy->CanExecute(this, NodeMemory);
		if (bCanRestartMyBranch)
		{
			ContinueResult = EBTNodeResult::Aborted;
		}
	}
	else
	{
		ContinueResult = EBTNodeResult::Aborted;
	}

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

static void FindCommonParent(const TArray<FBehaviorTreeInstance>& Instances,
							 class UBTCompositeNode* InNodeA, uint16 InstanceIdxA,
							 class UBTCompositeNode* InNodeB, uint16 InstanceIdxB,
							 class UBTCompositeNode*& CommonParentNode, uint16& CommonInstanceIdx)
{
	// find two nodes in the same instance (choose lower index = closer to root)
	CommonInstanceIdx = (InstanceIdxA <= InstanceIdxB) ? InstanceIdxA : InstanceIdxB;

	class UBTCompositeNode* NodeA = (CommonInstanceIdx == InstanceIdxA) ? InNodeA : Instances[CommonInstanceIdx].ActiveNode->GetParentNode();
	class UBTCompositeNode* NodeB = (CommonInstanceIdx == InstanceIdxB) ? InNodeB : Instances[CommonInstanceIdx].ActiveNode->GetParentNode();

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
	const bool bShouldCheckDecorators = bSwitchToHigherPriority && (RequestedByChildIndex >= 0);
	const UBTNode* DebuggerNode = bStoreForDebugger ? RequestedBy : NULL;

	FBTNodeIndex ExecutionIdx;
	ExecutionIdx.InstanceIndex = InstanceIdx;
	ExecutionIdx.ExecutionIndex = RequestedBy->GetExecutionIndex();

	if (bSwitchToHigherPriority)
	{
		RequestedByChildIndex = FMath::Max(0, RequestedByChildIndex);
		ExecutionIdx.ExecutionIndex = RequestedOn->GetChildExecutionIndex(RequestedByChildIndex);
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

			FindCommonParent(InstanceStack, RequestedOn, InstanceIdx, CurrentNode, CurrentInstanceIdx, CommonParent, CommonInstanceIdx);

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

void UBehaviorTreeComponent::ApplySearchUpdates(const TArray<struct FBehaviorTreeSearchUpdate>& UpdateList, int32 UpToIdx, bool bPostUpdate)
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
			if (UpdateInfo.Mode == EBTNodeUpdateMode::Add)
			{
				UpdateInstance.ActiveAuxNodes.Add(UpdateInfo.AuxNode);
				UpdateInfo.AuxNode->ConditionalOnBecomeRelevant(this, NodeMemory);
			}
			else
			{
				UpdateInstance.ActiveAuxNodes.RemoveSingleSwap(UpdateInfo.AuxNode);
				UpdateInfo.AuxNode->ConditionalOnCeaseRelevant(this, NodeMemory);
			}
		}
		else if (UpdateInfo.TaskNode)
		{
			if (UpdateInfo.Mode == EBTNodeUpdateMode::Add)
			{
				UE_VLOG(GetOwner(), LogBehaviorTree, Verbose, TEXT("Parallel task: %s added to active list"),
					*UBehaviorTreeTypes::DescribeNodeHelper(UpdateInfo.TaskNode));

				UpdateInstance.ParallelTasks.Add(FBehaviorTreeParallelTask(UpdateInfo.TaskNode, EBTTaskStatus::Active));
			}
			else
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
					bWaitingForParallelTasks = true;
				}

				OnTaskFinished(UpdateInfo.TaskNode, NodeResult);
			}
		}
	}
}

void UBehaviorTreeComponent::ApplySearchData(int32 UpToIdx)
{
	const bool bFullUpdate = (UpToIdx < 0);
	const int32 MaxIdx = bFullUpdate ? SearchData.PendingUpdates.Num() : (UpToIdx + 1);
	ApplySearchUpdates(SearchData.PendingUpdates, MaxIdx);

	// go though post updates (services) and clear both arrays if function was called without limit
	if (bFullUpdate)
	{
		ApplySearchUpdates(SearchData.PendingUpdates, SearchData.PendingUpdates.Num(), true);
		SearchData.PendingUpdates.Reset();
	}
	else
	{
		SearchData.PendingUpdates.RemoveAt(0, UpToIdx + 1, false);
	}
}

void UBehaviorTreeComponent::InitializeComponent()
{
	Super::InitializeComponent();

	// cache blackboard component if owner has one
	BlackboardComp = GetOwner()->FindComponentByClass<UBlackboardComponent>();
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
			AuxNode->ConditionalTickNode(this, NodeMemory, DeltaTime);
		}

		for (int32 iTask = 0; iTask < InstanceInfo.ParallelTasks.Num(); iTask++)
		{
			const UBTTaskNode* ParallelTask = InstanceInfo.ParallelTasks[iTask].TaskNode;
			uint8* NodeMemory = ParallelTask->GetNodeMemory<uint8>(InstanceInfo);
			ParallelTask->ConditionalTickTask(this, NodeMemory, DeltaTime);
		}
	}

	// tick active task
	FBehaviorTreeInstance& ActiveInstance = InstanceStack[ActiveInstanceIdx];
	if (ActiveInstance.ActiveNodeType == EBTActiveNode::ActiveTask ||
		ActiveInstance.ActiveNodeType == EBTActiveNode::AbortingTask)
	{
		UBTTaskNode* ActiveTask = (UBTTaskNode*)ActiveInstance.ActiveNode;
		uint8* NodeMemory = ActiveTask->GetNodeMemory<uint8>(ActiveInstance);
		ActiveTask->ConditionalTickTask(this, NodeMemory, DeltaTime);
	}
}

void UBehaviorTreeComponent::ProcessExecutionRequest()
{
	bRequestedFlowUpdate = false;
	if (bIsPaused)
	{
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

	// deactivate up to ExecuteNode
	if (InstanceStack[ActiveInstanceIdx].ActiveNode != ExecutionRequest.ExecuteNode)
	{
		const bool bDeactivated = DeactivateUpTo(ExecutionRequest.ExecuteNode, NodeResult);
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
				ApplySearchData(i);
				break;
			}
		}
	}

	// can't continue if parallel tasks are still aborting
	if (bWaitingForParallelTasks)
	{
		return;
	}

	// make sure that we don't have any additional instances on stack
	// (ApplySearchData doesn't remove them, so parallels can abort latently)
	if (InstanceStack.Num() > (ActiveInstanceIdx + 1))
	{
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

					// apply pending aux changes
					ApplySearchData();

					// and leave subtree
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

	ApplySearchData();
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

bool UBehaviorTreeComponent::DeactivateUpTo(class UBTCompositeNode* Node, EBTNodeResult::Type& NodeResult)
{
	UBTNode* DeactivatedChild = InstanceStack[ActiveInstanceIdx].ActiveNode;
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
			InstanceStack[ActiveInstanceIdx].RootNode->OnNodeDeactivation(SearchData, NodeResult);
			BT_SEARCHLOG(SearchData, Verbose, TEXT("Deactivate node: %s [leave subtree]"),
				*UBehaviorTreeTypes::DescribeNodeHelper(InstanceStack[ActiveInstanceIdx].RootNode));

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
	EBTNodeResult::Type TaskResult = TaskNode->ExecuteTask(this, NodeMemory);

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
	EBTNodeResult::Type TaskResult = ActiveTask->AbortTask(this, NodeMemory);

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

void UBehaviorTreeComponent::RegisterMessageObserver(const class UBTTaskNode* TaskNode, FName MessageType, int32 RequestID)
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

void UBehaviorTreeComponent::UnregisterParallelTask(const class UBTTaskNode* TaskNode)
{
	// remove and check pending aborts
	bWaitingForParallelTasks = false;
	for (int32 i = 0; i < InstanceStack.Num(); i++)
	{
		FBehaviorTreeInstance& InstanceInfo = InstanceStack[i];
		for (int32 iTask = InstanceInfo.ParallelTasks.Num() - 1; iTask >= 0; iTask--)
		{
			FBehaviorTreeParallelTask& ParallelInfo = InstanceInfo.ParallelTasks[iTask];
			if (ParallelInfo.TaskNode == TaskNode)
			{
				UE_VLOG(GetOwner(), LogBehaviorTree, Verbose, TEXT("Parallel task: %s removed from active list"),
					*UBehaviorTreeTypes::DescribeNodeHelper(TaskNode));

				InstanceInfo.ParallelTasks.RemoveAt(iTask);
			}
			else if (ParallelInfo.Status == EBTTaskStatus::Aborting)
			{
				bWaitingForParallelTasks = true;
			}
		}
	}
}

#undef AUX_NODE_WRAPPER

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

	UBTCompositeNode* RootNode = NULL;
	uint16 InstanceMemorySize = 0;

	const bool bLoaded = BTManager->LoadTree(TreeAsset, RootNode, InstanceMemorySize);
	if (bLoaded)
	{
		FBehaviorTreeInstance NewInstance(InstanceMemorySize);
		NewInstance.TreeAsset = TreeAsset;
		NewInstance.RootNode = RootNode;
		NewInstance.ActiveNode = NULL;
		NewInstance.ActiveNodeType = EBTActiveNode::Composite;

		NewInstance.InitializeMemory(this, RootNode);

		INC_DWORD_STAT(STAT_AI_BehaviorTree_NumInstances);
		InstanceStack.Push(NewInstance);
		ActiveInstanceIdx = InstanceStack.Num() - 1;

		// start root level services now (they won't be removed on looping tree anyway)
		for (int32 i = 0; i < RootNode->Services.Num(); i++)
		{
			UBTService* ServiceNode = RootNode->Services[i];
			uint8* NodeMemory = (uint8*)ServiceNode->GetNodeMemory<uint8>(InstanceStack[ActiveInstanceIdx]);

			InstanceStack[ActiveInstanceIdx].ActiveAuxNodes.Add(ServiceNode);
			ServiceNode->ConditionalOnBecomeRelevant(this, NodeMemory);
		}

		// start new task
		RequestExecution(RootNode, ActiveInstanceIdx, RootNode, 0, EBTNodeResult::InProgress);
		return true;
	}

	return false;
}

int32 UBehaviorTreeComponent::FindInstanceContainingNode(const UBTNode* Node) const
{
	int32 InstanceIdx = INDEX_NONE;
	if (Node && InstanceStack.Num())
	{
		if (InstanceStack[ActiveInstanceIdx].ActiveNode != Node)
		{
			const UBTNode* RootNode = Node;
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

uint8* UBehaviorTreeComponent::GetNodeMemory(UBTNode* Node, int32 InstanceIdx) const
{
	return InstanceStack.IsValidIndex(InstanceIdx) ? (uint8*)Node->GetNodeMemory<uint8>(InstanceStack[InstanceIdx]) : NULL;
}

FString UBehaviorTreeComponent::GetDebugInfoString() const 
{ 
	FString DebugInfo = TEXT("Behavior tree:\n");

	for (int32 i = 0; i < InstanceStack.Num(); i++)
	{
		const FBehaviorTreeInstance& InstanceInfo = InstanceStack[i];
		DebugInfo += FString::Printf(TEXT("  Instance %d: %s\n"), i, *GetNameSafe(InstanceInfo.TreeAsset));

		UBTNode* Node = InstanceInfo.ActiveNode;
		FString NodeTrace;

		while (Node)
		{
			uint8* NodeMemory = (uint8*)(Node->GetNodeMemory<uint8>(InstanceInfo));
			NodeTrace = FString::Printf(TEXT("      %s\n"), *Node->GetRuntimeDescription(this, NodeMemory, EBTDescriptionVerbosity::Detailed)) + NodeTrace;
			Node = Node->GetParentNode();
		}

		DebugInfo += NodeTrace;
	}

	return DebugInfo;
}

#if ENABLE_VISUAL_LOG
void UBehaviorTreeComponent::DescribeSelfToVisLog(struct FVisLogEntry* Snapshot) const
{
	if (IsPendingKill())
	{
		return;
	}

	if (BlackboardComp)
	{
		BlackboardComp->DescribeSelfToVisLog(Snapshot);
	}

	Snapshot->StatusString += GetDebugInfoString() + TEXT("\n");
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
	InstanceInfo.TreeAsset = ActiveInstance.TreeAsset;
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

		RuntimeDescriptions.Add(ComposedDesc);
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

	CurrentStep.InstanceStack[InstanceIdx].RuntimeDesc[TaskNode->GetExecutionIndex()] = ComposedDesc;
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
