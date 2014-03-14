// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AI/BrainComponent.h"
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

UCLASS(HeaderGroup=Component)
class ENGINE_API UBehaviorTreeComponent : public UBrainComponent
{
	GENERATED_UCLASS_BODY()

	// Begin UBrainComponent overrides
	virtual void RestartLogic() OVERRIDE;
protected:
	virtual void StopLogic(const FString& Reason) OVERRIDE;
	virtual void PauseLogic(const FString& Reason) OVERRIDE;
	virtual void ResumeLogic(const FString& Reason) OVERRIDE;
public:
	virtual bool IsRunning() const OVERRIDE;
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
	void RegisterMessageObserver(const class UBTTaskNode* TaskNode, FName MessageType, int32 MessageID);
	
	/** remove message observers registered with task */
	void UnregisterMessageObserversFrom(const class UBTTaskNode* TaskNode);
	void UnregisterMessageObserversFrom(const FBTNodeIndex& TaskIdx);

	/** add active parallel task */
	void RegisterParallelTask(const class UBTTaskNode* TaskNode);

	/** remove parallel task */
	void UnregisterParallelTask(const class UBTTaskNode* TaskNode);

	/** unregister all aux nodes less important than given index */
	void UnregisterAuxNodesUpTo(const struct FBTNodeIndex& Index);

	/** BEGIN UActorComponent overrides */
	virtual void InitializeComponent() OVERRIDE;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) OVERRIDE;
	/** END UActorComponent overrides */

	/** process execution flow */
	void ProcessExecutionRequest();

	/** schedule execution flow udpate in next tick */
	void ScheduleExecutionUpdate();

	/** tries to find behavior tree instance in context */
	int32 FindInstanceContainingNode(const class UBTNode* Node) const;

	/** @return current tree */
	class UBehaviorTree* GetCurrentTree() const;

	/** @return tree from top of instance stack */
	class UBehaviorTree* GetRootTree() const;

	/** @return active node */
	const class UBTNode* GetActiveNode() const;

	/** @return blackboard used with this component */
	class UBlackboardComponent* GetBlackboardComponent();

	/** @return blackboard used with this component */
	const class UBlackboardComponent* GetBlackboardComponent() const;

	/** get index of active instance on stack */
	uint16 GetActiveInstanceIdx() const;

	/** @return node memory */
	uint8* GetNodeMemory(class UBTNode* Node, int32 InstanceIdx) const;

	/** @return true if ExecutionRequest is switching to higher priority node */
	bool IsRestartPending() const;

	/** @return true if active node is one of child nodes of given one */
	bool IsExecutingBranch(const class UBTNode* Node, int32 ChildIndex = -1) const;

	/** @return status of speficied task */
	EBTTaskStatus::Type GetTaskStatus(const class UBTTaskNode* TaskNode) const;

	virtual FString GetDebugInfoString() const OVERRIDE;

#if ENABLE_VISUAL_LOG
	virtual void DescribeSelfToVisLog(struct FVisLogEntry* Snapshot) const OVERRIDE;
#endif

protected:

	/** blackboard component */
	UPROPERTY()
	class UBlackboardComponent* BlackboardComp;

	/** stack of behavior tree instances */
	TArray<struct FBehaviorTreeInstance> InstanceStack;

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
	mutable TArray<FString> SearchStartBlackboard;
	mutable TArray<FBehaviorTreeDebuggerInstance> RemovedInstances;

	/** debugger's recorded data */
	mutable TArray<FBehaviorTreeExecutionStep> DebuggerSteps;
#endif

	/** index of last active instance on stack */
	uint16 ActiveInstanceIdx;

	/** loops tree execution */
	uint8 bLoopExecution : 1;

	/** set when execution is waiting for parallel main tasks to abort */
	uint8 bWaitingForParallelTasks : 1;

	/** set when execution update is scheduled for next tick */
	uint8 bRequestedFlowUpdate : 1;

	/** if set, tree execution is allowed */
	uint8 bIsRunning : 1;

	/** if set, execution requests will be postponed */
	uint8 bIsPaused : 1;

	/** push behavior tree instance on execution stack */
	bool PushInstance(class UBehaviorTree* TreeAsset);

	/** find next task to execute */
	UBTTaskNode* FindNextTask(class UBTCompositeNode* ParentNode, uint16 ParentInstanceIdx, EBTNodeResult::Type LastResult);

	/** called when tree runs out of nodes to execute */
	void OnTreeFinished();

	/** apply pending node updates from SearchData */
	void ApplySearchData(int32 UpToIdx = -1);

	/** apply updates from specific list */
	void ApplySearchUpdates(const TArray<struct FBehaviorTreeSearchUpdate>& UpdateList, int32 UpToIdx, bool bPostUpdate = false);

	/** abort currently executed task */
	void AbortCurrentTask();

	/** execute new task */
	void ExecuteTask(class UBTTaskNode* TaskNode);

	/** deactivate all nodes up to requested one */
	bool DeactivateUpTo(class UBTCompositeNode* Node, EBTNodeResult::Type& NodeResult);

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
	void StoreDebuggerBlackboard(TArray<FString>& BlackboardValueDesc) const;

	/** gather nodes runtime descriptions */
	void StoreDebuggerRuntimeValues(TArray<FString>& RuntimeDescriptions, class UBTNode* RootNode, uint16 InstanceIdx) const;

	/** update runtime description of given task node in latest debugger's snapshot */
	void UpdateDebuggerAfterExecution(const class UBTTaskNode* TaskNode, uint16 InstanceIdx) const;

	friend class UBTNode;
	friend class UBTCompositeNode;
	friend class UBTTaskNode;
	friend class UBTTask_RunBehavior;
	friend class FBehaviorTreeDebugger;
};

//////////////////////////////////////////////////////////////////////////
// Inlines

FORCEINLINE class UBehaviorTree* UBehaviorTreeComponent::GetCurrentTree() const
{
	return InstanceStack.Num() ? InstanceStack[ActiveInstanceIdx].TreeAsset : NULL;
}

FORCEINLINE class UBehaviorTree* UBehaviorTreeComponent::GetRootTree() const
{
	return InstanceStack.Num() ? InstanceStack[0].TreeAsset : NULL;
}

FORCEINLINE class UBlackboardComponent* UBehaviorTreeComponent::GetBlackboardComponent()
{
	return BlackboardComp;
}

FORCEINLINE const class UBlackboardComponent* UBehaviorTreeComponent::GetBlackboardComponent() const
{
	return BlackboardComp;
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
