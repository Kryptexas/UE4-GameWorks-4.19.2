// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SBehaviorTreeDebuggerView.h"

class FBehaviorTreeDebugger : public FTickableGameObject
{
public:
	FBehaviorTreeDebugger();
	~FBehaviorTreeDebugger();

	virtual void Tick(float DeltaTime) OVERRIDE;
	virtual bool IsTickable() const OVERRIDE;
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual bool IsTickableInEditor() const OVERRIDE { return true; }
	virtual TStatId GetStatId() const OVERRIDE { RETURN_QUICK_DECLARE_CYCLE_STAT(FBehaviorTreeEditorTickHelper, STATGROUP_Tickables); }

	/** Refresh the debugging information we are displaying (only when paused, as Tick() updates when running) */
	void Refresh();

	void Setup(class UBehaviorTree* InTreeAsset, const class FBehaviorTreeEditor* InEditorOwner, TSharedPtr<SBehaviorTreeDebuggerView> DebuggerView);

	bool IsDebuggerReady() const;
	bool IsDebuggerRunning() const;
	bool IsShowingCurrentState() const;
	int32 GetShownStateIndex() const;

	void OnObjectSelected(UObject* Object);
	void OnAIDebugSelected(const APawn* Pawn);
	void OnTreeStarted(const class UBehaviorTreeComponent* OwnerComp, const class UBehaviorTree* InTreeAsset);
	void OnBeginPIE(const bool bIsSimulating);
	void OnEndPIE(const bool bIsSimulating);

	void OnBreakpointAdded(class UBehaviorTreeGraphNode* Node);
	void OnBreakpointRemoved(class UBehaviorTreeGraphNode* Node);

	void StepBackInto();
	void StepBackOver();
	void StepForwardInto();
	void StepForwardOver();
	void StepOut();
	bool CanStepBackInto() const;
	bool CanStepBackOver() const;
	bool CanStepForwardInto() const;
	bool CanStepForwardOver() const;
	bool CanStepOut() const;

	static void StopPlaySession();
	static void PausePlaySession();
	static void ResumePlaySession();
	static bool IsPlaySessionPaused();
	static bool IsPlaySessionRunning();
	static bool IsPIESimulating();
	static bool IsPIENotSimulating();

	FString GetDebuggedInstanceDesc() const;
	FString DescribeInstance(class UBehaviorTreeComponent* InstanceToDescribe) const;
	void OnInstanceSelectedInDropdown(class UBehaviorTreeComponent* SelectedInstance);
	void GetMatchingInstances(TArray<class UBehaviorTreeComponent*>& MatchingInstances);

	void InitializeFromParent(class FBehaviorTreeDebugger* ParentDebugger);
	bool HasContinuousNextStep() const;
	bool HasContinuousPrevStep() const;

private:

	/** owning editor */
	class FBehaviorTreeEditor* EditorOwner;

	/** asset for debugging */
	class UBehaviorTree* TreeAsset;

	/** root node in asset's graph */
	TWeakObjectPtr<class UBehaviorTreeGraphNode_Root> RootNode;

	/** instance for debugging */
	TWeakObjectPtr<class UBehaviorTreeComponent> TreeInstance;

	/** widget with details to show in editor */
	TSharedPtr<SBehaviorTreeDebuggerView> DebuggerDetails;

	/** matching debugger instance index from component's stack */
	int32 DebuggerInstanceIndex;

	/** index of state from buffer to show */
	int32 ActiveStepIndex;

	/** index of displayed step, used to detect changes */
	int32 DisplayedStepIndex;

	/** indices of display steps for different step actions */
	int32 StepForwardIntoIdx;
	int32 StepForwardOverIdx;
	int32 StepBackIntoIdx;
	int32 StepBackOverIdx;
	int32 StepOutIdx;

	/** execution indices of currently active breakpoints */
	TArray<uint16> ActiveBreakpoints;

	/** all known BT instances, cached for dropdown list */
	TArray<TWeakObjectPtr<class UBehaviorTreeComponent> > KnownInstances;

	/** cached PIE state */
	uint32 bIsPIEActive : 1;

	/** set when debugger instance is currently active one */
	uint32 bIsCurrentSubtree : 1;

	/** execution index of node that caused activated the breakpoint */
	uint16 StoppedOnBreakpointExecutionIndex;

	/** set value of DebuggerInstanceIndex variable */
	void UpdateDebuggerInstance();

	/** clear all runtime variables */
	void ClearDebuggerState();

	/** try using breakpoints on node change */
	void OnActiveNodeChanged(const TArray<uint16>& ActivePath, const TArray<uint16>& PrevStepPath);

	/** scan all actors and try to find matching BT component
	  * used only when user starts PIE before opening editor */
	void FindMatchingTreeInstance();

	/** find index on execution instance stack of matching tree asset */
	int32 FindMatchingDebuggerStack(class UBehaviorTreeComponent* TestInstance) const;

	/** find BT component in given actor */
	class UBehaviorTreeComponent* FindInstanceInActor(AActor* TestActor);

	/** try to find pawn currently locked by ai debug tool */
	void FindLockedDebugActor(class UWorld* World);

	/** recursively collect all breakpoint indices from child nodes */
	void CollectBreakpointsFromAsset(class UBehaviorTreeGraphNode* Node);

	/** recursively update node flags on all child nodes */
	void UpdateAssetFlags(const struct FBehaviorTreeDebuggerInstance& Data, class UBehaviorTreeGraphNode* Node, int32 StepIdx);

	/** set debugger flags on GraphNode */
	void SetNodeFlags(const struct FBehaviorTreeDebuggerInstance& Data, class UBehaviorTreeGraphNode* Node, class UBTNode* NodeInstance);

	/** set debugger flags on GraphNode for composite decorator */
	void SetCompositeDecoratorFlags(const struct FBehaviorTreeDebuggerInstance& Data, class UBehaviorTreeGraphNode_CompositeDecorator* Node);

	/** recursively update node flags on all child nodes */
	void UpdateAssetRuntimeDescription(const TArray<FString>& RuntimeDescriptions, class UBehaviorTreeGraphNode* Node);

	/** set debugger flags on GraphNode */
	void SetNodeRuntimeDescription(const TArray<FString>& RuntimeDescriptions, class UBehaviorTreeGraphNode* Node, class UBTNode* NodeInstance);

	/** set debugger flags on GraphNode for composite decorator */
	void SetCompositeDecoratorRuntimeDescription(const TArray<FString>& RuntimeDescriptions, class UBehaviorTreeGraphNode_CompositeDecorator* Node);

	/** updates variables in debugger details view */
	void UpdateDebuggerViewOnInstanceChange();
	void UpdateDebuggerViewOnStepChange();
	void UpdateDebuggerViewOnTick();

	/** find valid instance for given debugger step */
	int32 FindActiveInstanceIdx(int32 StepIdx) const;

	/** check if currently debugged instance is active subtree */
	void UpdateCurrentSubtree();

	/** updates currently displayed execution step */
	void UpdateCurrentStep(int32 PrevStepIdx, int32 NewStepIdx);

	/** updates button states */
	void UpdateAvailableActions();
};
