// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AITypes.h"
#include "BrainComponent.h"
#include "BehaviorTreeTypes.h"
#include "BehaviorTreeComponent.generated.h"

struct FBTNodeExecutionInfo
{
	/** index of first task allowed to be executed */
	struct FBTNodeIndex SearchStart;

	/** node to be executed */
	class UBTCompositeNode* ExecuteNode;

	/** subtree index */
	uint16 ExecuteInstanceIdx;

	/** result used for resuming execution */
	TEnumAsByte<EBTNodeResult::Type> ContinueWithResult;

	/** if set, tree will try to execute next child of composite instead of forcing branch containing SearchStart */
	uint8 bTryNextChild : 1;

	FBTNodeExecutionInfo() : ExecuteNode(NULL) { }
};

UCLASS()
class AIMODULE_API UBehaviorTreeComponent : public UBrainComponent
{
	GENERATED_UCLASS_BODY()

	// Begin UBrainComponent overrides
	virtual void RestartLogic() override;
protected:
	virtual void StopLogic(const FString& Reason) override;
	virtual void PauseLogic(const FString& Reason) override;
	virtual EAILogicResuming::Type ResumeLogic(const FString& Reason) override;

	/** indicates instance has been initialized to work with specific BT asset */
	bool TreeHasBeenStarted() const;

public:
	virtual bool IsRunning() const override;
	virtual bool IsPaused() const override;
	// End UBrainComponent overrides

	/** starts execution from root */
	bool StartTree(class UBehaviorTree* Asset, EBTExecutionMode::Type ExecuteMode = EBTExecutionMode::Looped);

	/** stops execution */
	void StopTree();

	/** restarts execution from root */
	void RestartTree();

	/** request execution change */
	void RequestExecution(class UBTCompositeNode* RequestedOn, int32 InstanceIdx, 
		const class UBTNode* RequestedBy, int32 RequestedByChildIndex,
		EBTNodeResult::Type ContinueWithResult, bool bStoreForDebugger = true);

	/** request execution change: helpers for decorator nodes */
	void RequestExecution(const class UBTDecorator* RequestedBy);

	/** request execution change: helpers for task nodes */
	void RequestExecution(EBTNodeResult::Type ContinueWithResult);

	/** finish latent execution or abort */
	void OnTaskFinished(const class UBTTaskNode* TaskNode, EBTNodeResult::Type TaskResult);

	/** setup message observer for given task */
	void RegisterMessageObserver(const class UBTTaskNode* TaskNode, FName MessageType);
	void RegisterMessageObserver(const class UBTTaskNode* TaskNode, FName MessageType, FAIRequestID MessageID);
	
	/** remove message observers registered with task */
	void UnregisterMessageObserversFrom(const class UBTTaskNode* TaskNode);
	void UnregisterMessageObserversFrom(const FBTNodeIndex& TaskIdx);

	/** add active parallel task */
	void RegisterParallelTask(const class UBTTaskNode* TaskNode);

	/** remove parallel task */
	void UnregisterParallelTask(const class UBTTaskNode* TaskNode, uint16 InstanceIdx);

	/** unregister all aux nodes less important than given index */
	void UnregisterAuxNodesUpTo(const struct FBTNodeIndex& Index);

	/** BEGIN UActorComponent overrides */
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;
	/** END UActorComponent overrides */

	/** process execution flow */
	void ProcessExecutionRequest();

	/** schedule execution flow update in next tick */
	void ScheduleExecutionUpdate();

	/** tries to find behavior tree instance in context */
	int32 FindInstanceContainingNode(const class UBTNode* Node) const;

	/** tries to find template node for given instanced node */
	class UBTNode* FindTemplateNode(const class UBTNode* Node) const;

	/** @return current tree */
	class UBehaviorTree* GetCurrentTree() const;

	/** @return tree from top of instance stack */
	class UBehaviorTree* GetRootTree() const;

	/** @return active node */
	const class UBTNode* GetActiveNode() const;
	
	/** get index of active instance on stack */
	uint16 GetActiveInstanceIdx() const;

	/** @return node memory */
	uint8* GetNodeMemory(class UBTNode* Node, int32 InstanceIdx) const;

	/** @return true if ExecutionRequest is switching to higher priority node */
	bool IsRestartPending() const;

	/** @return true if active node is one of child nodes of given one */
	bool IsExecutingBranch(const class UBTNode* Node, int32 ChildIndex = -1) const;

	/** @return true if aux node is currently active */
	bool IsAuxNodeActive(const class UBTAuxiliaryNode* AuxNode) const;

	/** @return status of speficied task */
	EBTTaskStatus::Type GetTaskStatus(const class UBTTaskNode* TaskNode) const;

	virtual FString GetDebugInfoString() const override;
	virtual FString DescribeActiveTasks() const;
	virtual FString DescribeActiveTrees() const;

#if ENABLE_VISUAL_LOG
	virtual void DescribeSelfToVisLog(struct FVisLogEntry* Snapshot) const override;
#endif

protected:
	/** stack of behavior tree instances */
	TArray<struct FBehaviorTreeInstance> InstanceStack;

	/** list of known subtree instances */
	TArray<struct FBehaviorTreeInstanceId> KnownInstances;

	/** instanced nodes */
	UPROPERTY(transient)
	TArray<class UBTNode*> NodeInstances;

	/** search data being currently used */
	struct FBehaviorTreeSearchData SearchData;

	/** execution request, will be applied when current task finish execution/aborting */
	FBTNodeExecutionInfo ExecutionRequest;

	/** message observers mapped by instance & execution index */
	TMultiMap<FBTNodeIndex,FAIMessageObserverHandle> TaskMessageObservers;

#if USE_BEHAVIORTREE_DEBUGGER
	/** search flow for debugger */
	mutable TArray<TArray<FBehaviorTreeDebuggerInstance::FNodeFlowData> > CurrentSearchFlow;
	mutable TArray<TArray<FBehaviorTreeDebuggerInstance::FNodeFlowData> > CurrentRestarts;
	mutable TMap<FName, FString> SearchStartBlackboard;
	mutable TArray<FBehaviorTreeDebuggerInstance> RemovedInstances;

	/** debugger's recorded data */
	mutable TArray<FBehaviorTreeExecutionStep> DebuggerSteps;
#endif

	/** index of last active instance on stack */
	uint16 ActiveInstanceIdx;

	/** loops tree execution */
	uint8 bLoopExecution : 1;

	/** set when execution is waiting for tasks to abort (current or parallel's main) */
	uint8 bWaitingForAbortingTasks : 1;

	/** set when execution update is scheduled for next tick */
	uint8 bRequestedFlowUpdate : 1;

	/** if set, tree execution is allowed */
	uint8 bIsRunning : 1;

	/** if set, execution requests will be postponed */
	uint8 bIsPaused : 1;

	/** push behavior tree instance on execution stack
	 *	@NOTE: should never be called out-side of BT execution, meaning only BT tasks can push another BT instance! */
	bool PushInstance(class UBehaviorTree* TreeAsset);

	/** add unique Id of newly created subtree to KnownInstances list and return its index */
	uint8 UpdateInstanceId(class UBehaviorTree* TreeAsset, const class UBTNode* OriginNode, int32 OriginInstanceIdx);

	/** find next task to execute */
	UBTTaskNode* FindNextTask(class UBTCompositeNode* ParentNode, uint16 ParentInstanceIdx, EBTNodeResult::Type LastResult);

	/** called when tree runs out of nodes to execute */
	void OnTreeFinished();

	/** apply pending node updates from SearchData */
	void ApplySearchData(class UBTNode* NewActiveNode, int32 UpToIdx = -1);

	/** apply updates from specific list */
	void ApplySearchUpdates(const TArray<struct FBehaviorTreeSearchUpdate>& UpdateList, int32 UpToIdx, int32 NewNodeExecutionIndex, bool bPostUpdate = false);

	/** abort currently executed task */
	void AbortCurrentTask();

	/** execute new task */
	void ExecuteTask(class UBTTaskNode* TaskNode);

	/** deactivate all nodes up to requested one */
	bool DeactivateUpTo(class UBTCompositeNode* Node, uint16 NodeInstanceIdx, EBTNodeResult::Type& NodeResult);

	/** update state of aborting tasks */
	void UpdateAbortingTasks();

	/** make a snapshot for debugger */
	void StoreDebuggerExecutionStep(EBTExecutionSnap::Type SnapType);

	/** make a snapshot for debugger from given subtree instance */
	void StoreDebuggerInstance(FBehaviorTreeDebuggerInstance& InstanceInfo, uint16 InstanceIdx, EBTExecutionSnap::Type SnapType) const;
	void StoreDebuggerRemovedInstance(uint16 InstanceIdx) const;

	/** store search step for debugger */
	void StoreDebuggerSearchStep(const class UBTNode* Node, uint16 InstanceIdx, EBTNodeResult::Type NodeResult) const;
	void StoreDebuggerSearchStep(const class UBTNode* Node, uint16 InstanceIdx, bool bPassed) const;

	/** store restarting node for debugger */
	void StoreDebuggerRestart(const class UBTNode* Node, uint16 InstanceIdx, bool bAllowed);

	/** describe blackboard's key values */
	void StoreDebuggerBlackboard(TMap<FName, FString>& BlackboardValueDesc) const;

	/** gather nodes runtime descriptions */
	void StoreDebuggerRuntimeValues(TArray<FString>& RuntimeDescriptions, class UBTNode* RootNode, uint16 InstanceIdx) const;

	/** update runtime description of given task node in latest debugger's snapshot */
	void UpdateDebuggerAfterExecution(const class UBTTaskNode* TaskNode, uint16 InstanceIdx) const;

	friend class UBTNode;
	friend class UBTCompositeNode;
	friend class UBTTaskNode;
	friend class UBTTask_RunBehavior;
	friend class FBehaviorTreeDebugger;
	friend struct FBehaviorTreeInstance;
};

//////////////////////////////////////////////////////////////////////////
// Inlines

FORCEINLINE class UBehaviorTree* UBehaviorTreeComponent::GetCurrentTree() const
{
	return InstanceStack.Num() ? KnownInstances[InstanceStack[ActiveInstanceIdx].InstanceIdIndex].TreeAsset : NULL;
}

FORCEINLINE class UBehaviorTree* UBehaviorTreeComponent::GetRootTree() const
{
	return InstanceStack.Num() ? KnownInstances[InstanceStack[0].InstanceIdIndex].TreeAsset : NULL;
}

FORCEINLINE const class UBTNode* UBehaviorTreeComponent::GetActiveNode() const
{
	return InstanceStack.Num() ? InstanceStack[ActiveInstanceIdx].ActiveNode : NULL;
}

FORCEINLINE uint16 UBehaviorTreeComponent::GetActiveInstanceIdx() const
{
	return ActiveInstanceIdx;
}

FORCEINLINE bool UBehaviorTreeComponent::IsRestartPending() const
{
	return ExecutionRequest.ExecuteNode && !ExecutionRequest.bTryNextChild;
}
