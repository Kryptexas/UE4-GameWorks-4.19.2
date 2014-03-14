// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BehaviorTreeEditorPrivatePCH.h"
#include "AI/BehaviorTreeDelegates.h"
#include "SBehaviorTreeDebuggerView.h"

FBehaviorTreeDebugger::FBehaviorTreeDebugger()
{
	EditorOwner = NULL;
	TreeAsset = NULL;
	bIsPIEActive = false;
	bPauseOnNodeChange = false;
	bIsCurrentSubtree = false;

	FEditorDelegates::BeginPIE.AddRaw(this, &FBehaviorTreeDebugger::OnBeginPIE);
	FEditorDelegates::EndPIE.AddRaw(this, &FBehaviorTreeDebugger::OnEndPIE);
}

FBehaviorTreeDebugger::~FBehaviorTreeDebugger()
{
	FEditorDelegates::BeginPIE.RemoveAll(this);
	FEditorDelegates::EndPIE.RemoveAll(this);
	USelection::SelectObjectEvent.RemoveAll(this);
	FBehaviorTreeDelegates::OnTreeStarted.RemoveAll(this);
	FBehaviorTreeDelegates::OnDebugLocked.RemoveAll(this);
	FBehaviorTreeDelegates::OnDebugSelected.RemoveAll(this);
}

void FBehaviorTreeDebugger::Setup(class UBehaviorTree* InTreeAsset, const class FBehaviorTreeEditor* InEditorOwner, TSharedPtr<SBehaviorTreeDebuggerView> DebuggerView)
{
	DebuggerDetails = DebuggerView;
	EditorOwner = (class FBehaviorTreeEditor*)InEditorOwner;
	TreeAsset = InTreeAsset;
	DebuggerInstanceIndex = INDEX_NONE;
	ActiveStepIndex = 0;
	ActiveBreakpoints.Reset();
	KnownInstances.Reset();

	if (TreeAsset)
	{
		for (int32 i = 0; i < TreeAsset->BTGraph->Nodes.Num(); i++)
		{
			RootNode = Cast<UBehaviorTreeGraphNode_Root>(TreeAsset->BTGraph->Nodes[i]);
			if (RootNode.IsValid())
			{
				break;
			}
		}
	}

#if USE_BEHAVIORTREE_DEBUGGER
	// check if PIE is already running
	if (IsPIESimulating())
	{
		OnBeginPIE(GEditor->bIsSimulatingInEditor);

		// make sure is grabs data if currently paused
		if (IsPlaySessionPaused() && TreeInstance.IsValid())
		{
			FindLockedDebugActor(GEditor->PlayWorld);
			
			UpdateDebuggerInstance();
			if (DebuggerInstanceIndex != INDEX_NONE)
			{
				UpdateDebuggerViewOnStepChange();
				UpdateDebuggerViewOnTick();

				const FBehaviorTreeDebuggerInstance& ShowInstance = TreeInstance->DebuggerSteps[ActiveStepIndex].InstanceStack[DebuggerInstanceIndex];
				OnActiveNodeChanged(ShowInstance.ActivePath, HasContinuousPrevStep() ?
					TreeInstance->DebuggerSteps[ActiveStepIndex - 1].InstanceStack[DebuggerInstanceIndex].ActivePath :
					TArray<uint16>());

				UpdateAssetFlags(ShowInstance, RootNode.Get(), ActiveStepIndex);
			}
		}
	}
#endif
}

void FBehaviorTreeDebugger::Tick(float DeltaTime)
{
	if (TreeAsset == NULL || IsPlaySessionPaused())
	{
		return;
	}

	if (!TreeInstance.IsValid())
	{
		// clear state when active tree is lost
		if (DebuggerInstanceIndex != INDEX_NONE)
		{
			ClearDebuggerState();
		}

		return;
	}

#if USE_BEHAVIORTREE_DEBUGGER
	TArray<uint16> EmptyPath;

	// find index of previously displayed state and notify about all changes in between to give breakpoints a chance to trigger
	for (int32 i = FMath::Max(0, ActiveStepIndex); i < TreeInstance->DebuggerSteps.Num(); i++)
	{
		const FBehaviorTreeExecutionStep& Step = TreeInstance->DebuggerSteps[i];
		if (Step.StepIndex > DisplayedStepIndex)
		{
			ActiveStepIndex = i;
			UpdateDebuggerInstance();

			if (DebuggerInstanceIndex != INDEX_NONE)
			{
				UpdateDebuggerViewOnStepChange();

				const FBehaviorTreeDebuggerInstance& ShowInstance = TreeInstance->DebuggerSteps[ActiveStepIndex].InstanceStack[DebuggerInstanceIndex];
				OnActiveNodeChanged(ShowInstance.ActivePath, HasContinuousPrevStep() ?
					TreeInstance->DebuggerSteps[ActiveStepIndex - 1].InstanceStack[DebuggerInstanceIndex].ActivePath :
					EmptyPath);
			}
		}

		// skip rest of them if breakpoint hit
		if (IsPlaySessionPaused())
		{
			break;
		}
	}

	UpdateDebuggerInstance();
	if (DebuggerInstanceIndex != INDEX_NONE)
	{
		const FBehaviorTreeDebuggerInstance& ShowInstance = TreeInstance->DebuggerSteps[ActiveStepIndex].InstanceStack[DebuggerInstanceIndex];

		if (DisplayedStepIndex != TreeInstance->DebuggerSteps[ActiveStepIndex].StepIndex)
		{
			UpdateAssetFlags(ShowInstance, RootNode.Get(), ActiveStepIndex);
		}

		// collect current runtime descriptions for every node
		TArray<FString> RuntimeDescriptions;
		TreeInstance->StoreDebuggerRuntimeValues(RuntimeDescriptions, ShowInstance.RootNode, DebuggerInstanceIndex);
	
		UpdateAssetRuntimeDescription(RuntimeDescriptions, RootNode.Get());
	}

	UpdateDebuggerViewOnTick();
#endif
}

bool FBehaviorTreeDebugger::IsTickable() const
{
	return IsDebuggerReady();
}

void FBehaviorTreeDebugger::OnBeginPIE(const bool bIsSimulating)
{
	bIsPIEActive = true;
	EditorOwner->RegenerateMenusAndToolbars();

	ActiveBreakpoints.Reset();
	CollectBreakpointsFromAsset(RootNode.Get());

	FindMatchingTreeInstance();

	USelection::SelectObjectEvent.AddRaw(this, &FBehaviorTreeDebugger::OnObjectSelected);
	FBehaviorTreeDelegates::OnTreeStarted.AddRaw(this, &FBehaviorTreeDebugger::OnTreeStarted);
	FBehaviorTreeDelegates::OnDebugSelected.AddRaw(this, &FBehaviorTreeDebugger::OnAIDebugSelected);
}

void FBehaviorTreeDebugger::OnEndPIE(const bool bIsSimulating)
{
	bIsPIEActive = false;
	EditorOwner->RegenerateMenusAndToolbars();

	USelection::SelectObjectEvent.RemoveAll(this);
	FBehaviorTreeDelegates::OnTreeStarted.RemoveAll(this);
	FBehaviorTreeDelegates::OnDebugSelected.RemoveAll(this);

	ClearDebuggerState();
	ActiveBreakpoints.Reset();

	FBehaviorTreeDebuggerInstance EmptyData;
	UpdateAssetFlags(EmptyData, RootNode.Get(), INDEX_NONE);
	UpdateDebuggerViewOnInstanceChange();
}

void FBehaviorTreeDebugger::OnObjectSelected(UObject* Object)
{
	if (Object && Object->IsSelected())
	{
		UBehaviorTreeComponent* InstanceComp = FindInstanceInActor(Cast<AActor>(Object));
		if (InstanceComp)
		{
			ClearDebuggerState();
			TreeInstance = InstanceComp;

			UpdateDebuggerViewOnInstanceChange();
		}
	}	
}

void FBehaviorTreeDebugger::OnAIDebugSelected(const APawn* Pawn)
{
	UBehaviorTreeComponent* TestComp = FindInstanceInActor((APawn*)Pawn);
	if (TestComp)
	{
		ClearDebuggerState();
		TreeInstance = TestComp;

		UpdateDebuggerViewOnInstanceChange();
	}
}

void FBehaviorTreeDebugger::OnTreeStarted(const class UBehaviorTreeComponent* OwnerComp, const class UBehaviorTree* InTreeAsset)
{
	// start debugging if tree asset matches, and no other actor was selected
	if (!TreeInstance.IsValid() && TreeAsset && TreeAsset == InTreeAsset)
	{
		ClearDebuggerState();
		TreeInstance = OwnerComp;

		UpdateDebuggerViewOnInstanceChange();
	}

	// update known instances
	TWeakObjectPtr<UBehaviorTreeComponent> KnownComp = (UBehaviorTreeComponent*)OwnerComp;
	KnownInstances.AddUnique(KnownComp);
}

void FBehaviorTreeDebugger::ClearDebuggerState()
{
	DebuggerInstanceIndex = INDEX_NONE;
	ActiveStepIndex = 0;
	bPauseOnNodeChange = false;
	DisplayedStepIndex = INDEX_NONE;

	if (TreeAsset && RootNode.IsValid())
	{
		FBehaviorTreeDebuggerInstance EmptyData;
		UpdateAssetFlags(EmptyData, RootNode.Get(), INDEX_NONE);
	}
}

void FBehaviorTreeDebugger::UpdateDebuggerInstance()
{
	int32 PrevStackIndex = DebuggerInstanceIndex;
	DebuggerInstanceIndex = INDEX_NONE;

	if (TreeInstance.IsValid())
	{
#if USE_BEHAVIORTREE_DEBUGGER
		if (TreeInstance->DebuggerSteps.IsValidIndex(ActiveStepIndex))
		{
			const FBehaviorTreeExecutionStep& StepInfo = TreeInstance->DebuggerSteps[ActiveStepIndex];
			for (int32 i = 0; i < StepInfo.InstanceStack.Num(); i++)
			{
				if (StepInfo.InstanceStack[i].TreeAsset == TreeAsset)
				{
					DebuggerInstanceIndex = i;
					break;
				}
			}
		}
#endif
		UpdateCurrentSubtree();
	}

	if (DebuggerInstanceIndex != PrevStackIndex)
	{
		UpdateDebuggerViewOnInstanceChange();
	}
}

void FBehaviorTreeDebugger::SetNodeFlags(const struct FBehaviorTreeDebuggerInstance& Data, class UBehaviorTreeGraphNode* Node, class UBTNode* NodeInstance)
{
	const bool bIsNodeActivePath = Data.ActivePath.Contains(NodeInstance->GetExecutionIndex());
	const bool bIsNodeActiveAdditional = Data.AdditionalActiveNodes.Contains(NodeInstance->GetExecutionIndex());
	const bool bIsNodeActive = bIsNodeActivePath || bIsNodeActiveAdditional;
	const bool bIsShowingCurrentState = IsShowingCurrentState();

	Node->DebuggerUpdateCounter = DisplayedStepIndex;
	Node->bDebuggerMarkCurrentlyActive = bIsNodeActive && bIsShowingCurrentState;
	Node->bDebuggerMarkPreviouslyActive = bIsNodeActive && !bIsShowingCurrentState;
	
	const bool bIsTaskNode = NodeInstance->IsA(UBTTaskNode::StaticClass());
	Node->bDebuggerMarkFlashActive = bIsNodeActivePath && bIsTaskNode && IsPlaySessionRunning();
	Node->bDebuggerMarkSearchTrigger = false;
	Node->bDebuggerMarkSearchFailedTrigger = false;
	Node->bDebuggerMarkSearchOptional = false;

	Node->bDebuggerMarkBreakpointTrigger = NodeInstance->GetExecutionIndex() == StoppedOnBreakpointExecutionIndex;
	if (Node->bDebuggerMarkBreakpointTrigger)
	{
		EditorOwner->JumpToNode(Node);
	}

	int32 SearchPathIdx = INDEX_NONE;
	int32 NumTriggers = 0;
	bool bTriggerOnly = false;

	for (int32 i = 0; i < Data.PathFromPrevious.Num(); i++)
	{
		const FBehaviorTreeDebuggerInstance::FNodeFlowData& SearchStep = Data.PathFromPrevious[i];
		const bool bMatchesNodeIndex = (SearchStep.ExecutionIndex == NodeInstance->GetExecutionIndex());
		if (SearchStep.bTrigger || SearchStep.bDiscardedTrigger)
		{
			NumTriggers++;
			if (bMatchesNodeIndex)
			{
				Node->bDebuggerMarkSearchTrigger = SearchStep.bTrigger;
				Node->bDebuggerMarkSearchFailedTrigger = SearchStep.bDiscardedTrigger;
				bTriggerOnly = true;
			}
		}
		else if (bMatchesNodeIndex)
		{
			SearchPathIdx = i;
			bTriggerOnly = false;
		}
	}

	Node->bDebuggerMarkSearchSucceeded = (SearchPathIdx != INDEX_NONE) && Data.PathFromPrevious[SearchPathIdx].bPassed;
	Node->bDebuggerMarkSearchOptional = (SearchPathIdx != INDEX_NONE) && Data.PathFromPrevious[SearchPathIdx].bOptional;
	Node->bDebuggerMarkSearchFailed = (SearchPathIdx != INDEX_NONE) && !Data.PathFromPrevious[SearchPathIdx].bPassed && !Data.PathFromPrevious[SearchPathIdx].bOptional;
	Node->DebuggerSearchPathIndex = bTriggerOnly ? 0 : FMath::Max(-1, SearchPathIdx - NumTriggers);
	Node->DebuggerSearchPathSize = Data.PathFromPrevious.Num() - NumTriggers;
}

void FBehaviorTreeDebugger::SetCompositeDecoratorFlags(const struct FBehaviorTreeDebuggerInstance& Data, class UBehaviorTreeGraphNode_CompositeDecorator* Node)
{
	const bool bIsShowingCurrentState = IsShowingCurrentState();
	bool bIsNodeActive = false;
	for (int32 i = 0; i < Data.AdditionalActiveNodes.Num(); i++)
	{
		if (Node->FirstExecutionIndex <= Data.AdditionalActiveNodes[i] && Node->LastExecutionIndex >= Data.AdditionalActiveNodes[i])
		{
			bIsNodeActive = true;
			break;
		}
	}	
	
	Node->DebuggerUpdateCounter = DisplayedStepIndex;
	Node->bDebuggerMarkCurrentlyActive = bIsNodeActive && bIsShowingCurrentState;
	Node->bDebuggerMarkPreviouslyActive = bIsNodeActive && !bIsShowingCurrentState;

	Node->bDebuggerMarkFlashActive = false;
	Node->bDebuggerMarkSearchTrigger = false;
	Node->bDebuggerMarkSearchFailedTrigger = false;

	int32 SearchPathIdx = INDEX_NONE;
	int32 NumTriggers = 0;
	bool bTriggerOnly = false;
	for (int32 i = 0; i < Data.PathFromPrevious.Num(); i++)
	{
		const FBehaviorTreeDebuggerInstance::FNodeFlowData& SearchStep = Data.PathFromPrevious[i];
		const bool bMatchesNodeIndex = (Node->FirstExecutionIndex <= SearchStep.ExecutionIndex && Node->LastExecutionIndex >= SearchStep.ExecutionIndex);
		if (SearchStep.bTrigger || SearchStep.bDiscardedTrigger)
		{
			NumTriggers++;
			if (bMatchesNodeIndex)
			{
				Node->bDebuggerMarkSearchTrigger = SearchStep.bTrigger;
				Node->bDebuggerMarkSearchFailedTrigger = SearchStep.bDiscardedTrigger;
				bTriggerOnly = true;
			}
			else if (bMatchesNodeIndex)
			{
				SearchPathIdx = i;
				bTriggerOnly = false;
			}
		}
	}

	Node->bDebuggerMarkSearchSucceeded = (SearchPathIdx != INDEX_NONE) && Data.PathFromPrevious[SearchPathIdx].bPassed;
	Node->bDebuggerMarkSearchOptional = (SearchPathIdx != INDEX_NONE) && Data.PathFromPrevious[SearchPathIdx].bOptional;
	Node->bDebuggerMarkSearchFailed = (SearchPathIdx != INDEX_NONE) && !Data.PathFromPrevious[SearchPathIdx].bPassed && !Data.PathFromPrevious[SearchPathIdx].bOptional;
	Node->DebuggerSearchPathIndex = bTriggerOnly ? 0 : FMath::Max(-1, SearchPathIdx - NumTriggers);
	Node->DebuggerSearchPathSize = Data.PathFromPrevious.Num() - NumTriggers;
}

void FBehaviorTreeDebugger::UpdateAssetFlags(const struct FBehaviorTreeDebuggerInstance& Data, class UBehaviorTreeGraphNode* Node, int32 StepIdx)
{
	if (Node == NULL)
	{
		return;
	}

	// special case for marking root when out of nodes
	if (Node == RootNode.Get())
	{
		const bool bIsNodeActive = (Data.ActivePath.Num() == 0) && (StepIdx >= 0);
		const bool bIsShowingCurrentState = IsShowingCurrentState();

		Node->bDebuggerMarkCurrentlyActive = bIsNodeActive && bIsShowingCurrentState;
		Node->bDebuggerMarkPreviouslyActive = bIsNodeActive && !bIsShowingCurrentState;
		DisplayedStepIndex = StepIdx;
	}

	for (int32 PinIdx = 0; PinIdx < Node->Pins.Num(); PinIdx++)
	{
		UEdGraphPin* Pin = Node->Pins[PinIdx];
		if (Pin->Direction != EGPD_Output)
		{
			continue;
		}

		for (int32 i = 0; i < Pin->LinkedTo.Num(); i++)
		{
			UBehaviorTreeGraphNode* LinkedNode = Cast<UBehaviorTreeGraphNode>(Pin->LinkedTo[i]->GetOwningNode());
			if (LinkedNode)
			{
				UBTNode* BTNode = Cast<UBTNode>(LinkedNode->NodeInstance);
				if (BTNode)
				{
					SetNodeFlags(Data, LinkedNode, BTNode);
					SetNodeRuntimeDescription(Data.RuntimeDesc, LinkedNode, BTNode);
				}

				for (int32 iAux = 0; iAux < LinkedNode->Decorators.Num(); iAux++)
				{
					UBehaviorTreeGraphNode_Decorator* DecoratorNode = Cast<UBehaviorTreeGraphNode_Decorator>(LinkedNode->Decorators[iAux]);
					UBTAuxiliaryNode* AuxNode = DecoratorNode ? Cast<UBTAuxiliaryNode>(DecoratorNode->NodeInstance) : NULL;
					if (AuxNode)
					{
						SetNodeFlags(Data, DecoratorNode, AuxNode);
						SetNodeRuntimeDescription(Data.RuntimeDesc, DecoratorNode, AuxNode);

						// pass restart trigger to parent graph node for drawing
						LinkedNode->bDebuggerMarkSearchTrigger |= DecoratorNode->bDebuggerMarkSearchTrigger;
						LinkedNode->bDebuggerMarkSearchFailedTrigger |= DecoratorNode->bDebuggerMarkSearchFailedTrigger;
					}

					UBehaviorTreeGraphNode_CompositeDecorator* CompDecoratorNode = Cast<UBehaviorTreeGraphNode_CompositeDecorator>(LinkedNode->Decorators[iAux]);
					if (CompDecoratorNode)
					{
						SetCompositeDecoratorFlags(Data, CompDecoratorNode);
						SetCompositeDecoratorRuntimeDescription(Data.RuntimeDesc, CompDecoratorNode);

						// pass restart trigger to parent graph node for drawing
						LinkedNode->bDebuggerMarkSearchTrigger |= CompDecoratorNode->bDebuggerMarkSearchTrigger;
						LinkedNode->bDebuggerMarkSearchFailedTrigger |= CompDecoratorNode->bDebuggerMarkSearchFailedTrigger;
					}
				}

				for (int32 iAux = 0; iAux < LinkedNode->Services.Num(); iAux++)
				{
					UBehaviorTreeGraphNode_Service* ServiceNode = Cast<UBehaviorTreeGraphNode_Service>(LinkedNode->Services[iAux]);
					UBTAuxiliaryNode* AuxNode = ServiceNode ? Cast<UBTAuxiliaryNode>(ServiceNode->NodeInstance) : NULL;
					if (AuxNode)
					{
						SetNodeFlags(Data, ServiceNode, AuxNode);
						SetNodeRuntimeDescription(Data.RuntimeDesc, ServiceNode, AuxNode);
					}
				}

				UpdateAssetFlags(Data, LinkedNode, StepIdx);
			}
		}
	}
}

void FBehaviorTreeDebugger::UpdateAssetRuntimeDescription(const TArray<FString>& RuntimeDescriptions, class UBehaviorTreeGraphNode* Node)
{
	if (Node == NULL)
	{
		return;
	}

	for (int32 PinIdx = 0; PinIdx < Node->Pins.Num(); PinIdx++)
	{
		UEdGraphPin* Pin = Node->Pins[PinIdx];
		if (Pin->Direction != EGPD_Output)
		{
			continue;
		}

		for (int32 i = 0; i < Pin->LinkedTo.Num(); i++)
		{
			UBehaviorTreeGraphNode* LinkedNode = Cast<UBehaviorTreeGraphNode>(Pin->LinkedTo[i]->GetOwningNode());
			if (LinkedNode)
			{
				UBTNode* BTNode = Cast<UBTNode>(LinkedNode->NodeInstance);
				if (BTNode)
				{
					SetNodeRuntimeDescription(RuntimeDescriptions, LinkedNode, BTNode);
				}

				for (int32 iAux = 0; iAux < LinkedNode->Decorators.Num(); iAux++)
				{
					UBehaviorTreeGraphNode_Decorator* DecoratorNode = Cast<UBehaviorTreeGraphNode_Decorator>(LinkedNode->Decorators[iAux]);
					UBTAuxiliaryNode* AuxNode = DecoratorNode ? Cast<UBTAuxiliaryNode>(DecoratorNode->NodeInstance) : NULL;
					if (AuxNode)
					{
						SetNodeRuntimeDescription(RuntimeDescriptions, DecoratorNode, AuxNode);
					}

					UBehaviorTreeGraphNode_CompositeDecorator* CompDecoratorNode = Cast<UBehaviorTreeGraphNode_CompositeDecorator>(LinkedNode->Decorators[iAux]);
					if (CompDecoratorNode)
					{
						SetCompositeDecoratorRuntimeDescription(RuntimeDescriptions, CompDecoratorNode);
					}
				}

				for (int32 iAux = 0; iAux < LinkedNode->Services.Num(); iAux++)
				{
					UBehaviorTreeGraphNode_Service* ServiceNode = Cast<UBehaviorTreeGraphNode_Service>(LinkedNode->Services[iAux]);
					UBTAuxiliaryNode* AuxNode = ServiceNode ? Cast<UBTAuxiliaryNode>(ServiceNode->NodeInstance) : NULL;
					if (AuxNode)
					{
						SetNodeRuntimeDescription(RuntimeDescriptions, ServiceNode, AuxNode);
					}
				}

				UpdateAssetRuntimeDescription(RuntimeDescriptions, LinkedNode);
			}
		}
	}
}

void FBehaviorTreeDebugger::SetNodeRuntimeDescription(const TArray<FString>& RuntimeDescriptions, class UBehaviorTreeGraphNode* Node, class UBTNode* NodeInstance)
{
	Node->DebuggerRuntimeDescription = RuntimeDescriptions.IsValidIndex(NodeInstance->GetExecutionIndex()) ?
		RuntimeDescriptions[NodeInstance->GetExecutionIndex()] : FString();
}

void FBehaviorTreeDebugger::SetCompositeDecoratorRuntimeDescription(const TArray<FString>& RuntimeDescriptions, class UBehaviorTreeGraphNode_CompositeDecorator* Node)
{
	Node->DebuggerRuntimeDescription.Empty();
	for (int32 i = Node->FirstExecutionIndex; i <= Node->LastExecutionIndex; i++)
	{
		if (RuntimeDescriptions.IsValidIndex(i) && RuntimeDescriptions[i].Len())
		{
			if (Node->DebuggerRuntimeDescription.Len())
			{
				Node->DebuggerRuntimeDescription.AppendChar(TEXT('\n'));
			}

			Node->DebuggerRuntimeDescription += FString::Printf(TEXT("[%d] %s"), i, *RuntimeDescriptions[i].Replace(TEXT("\n"), TEXT(", ")));
		}
	}
}

void FBehaviorTreeDebugger::CollectBreakpointsFromAsset(class UBehaviorTreeGraphNode* Node)
{
	if (Node == NULL)
	{
		return;
	}

	for (int32 PinIdx = 0; PinIdx < Node->Pins.Num(); PinIdx++)
	{
		UEdGraphPin* Pin = Node->Pins[PinIdx];
		if (Pin->Direction != EGPD_Output)
		{
			continue;
		}

		for (int32 i = 0; i < Pin->LinkedTo.Num(); i++)
		{
			UBehaviorTreeGraphNode* LinkedNode = Cast<UBehaviorTreeGraphNode>(Pin->LinkedTo[i]->GetOwningNode());
			if (LinkedNode)
			{
				UBTNode* BTNode = Cast<UBTNode>(LinkedNode->NodeInstance);
				if (BTNode && LinkedNode->bHasBreakpoint && LinkedNode->bIsBreakpointEnabled)
				{
					ActiveBreakpoints.Add(BTNode->GetExecutionIndex());
				}

				CollectBreakpointsFromAsset(LinkedNode);
			}
		}
	}
}

int32 FBehaviorTreeDebugger::FindMatchingDebuggerStack(class UBehaviorTreeComponent* TestInstance) const
{
#if USE_BEHAVIORTREE_DEBUGGER
	if (TestInstance && TestInstance->DebuggerSteps.Num())
	{
		const FBehaviorTreeExecutionStep& StepInfo = TestInstance->DebuggerSteps.Last();
		for (int32 i = 0; i < StepInfo.InstanceStack.Num(); i++)
		{
			if (StepInfo.InstanceStack[i].TreeAsset == TreeAsset)
			{
				return i;
			}
		}
	}
#endif

	return INDEX_NONE;
}

class UBehaviorTreeComponent* FBehaviorTreeDebugger::FindInstanceInActor(AActor* TestActor)
{
	class UBehaviorTreeComponent* FoundInstance = NULL;
	if (TestActor)
	{
		APawn* TestPawn = Cast<APawn>(TestActor);
		if (TestPawn && TestPawn->GetController())
		{
			FoundInstance = TestPawn->GetController()->FindComponentByClass<UBehaviorTreeComponent>();
		}

		if (FoundInstance == NULL)
		{
			FoundInstance = TestActor->FindComponentByClass<UBehaviorTreeComponent>();
		}
	}

	return FoundInstance;
}

void FBehaviorTreeDebugger::FindLockedDebugActor(UWorld* World)
{
	APlayerController* LocalPC = GEngine->GetFirstLocalPlayerController(World);
	if (LocalPC && LocalPC->GetHUD() && LocalPC->GetPawnOrSpectator())
	{
		UGameplayDebuggingController* DebuggingController = LocalPC->FindComponentByClass<UGameplayDebuggingController>();
		const APawn* LockedPawn = DebuggingController != NULL ? DebuggingController->GetCurrentDebugTarget() : NULL;
		UBehaviorTreeComponent* TestInstance = FindInstanceInActor((APawn*)LockedPawn);
		if (TestInstance)
		{
			TreeInstance = TestInstance;

#if USE_BEHAVIORTREE_DEBUGGER
			ActiveStepIndex = TestInstance->DebuggerSteps.Num() - 1;
#endif
		}
	}
}

void FBehaviorTreeDebugger::FindMatchingTreeInstance()
{
	KnownInstances.Reset();
	if (GEditor->PlayWorld == NULL)
	{
		return;
	}

	UBehaviorTreeComponent* MatchingComp = NULL;
	for (FActorIterator It(GEditor->PlayWorld); It; ++It)
	{
		AActor* TestActor = *It;
		UBehaviorTreeComponent* TestComp = TestActor ? TestActor->FindComponentByClass<UBehaviorTreeComponent>() : NULL;

		KnownInstances.Add(TestComp);

		const int32 MatchingIdx = FindMatchingDebuggerStack(TestComp);
		if (MatchingIdx != INDEX_NONE)
		{
			MatchingComp = TestComp;

			if (TestActor->IsSelected())
			{
				TreeInstance = TestComp;
				return;
			}
		}
	}

	if (MatchingComp)
	{
		TreeInstance = MatchingComp;
		UpdateDebuggerViewOnInstanceChange();
	}
}

bool FBehaviorTreeDebugger::IsDebuggerReady() const
{
	return bIsPIEActive;
}

bool FBehaviorTreeDebugger::IsDebuggerRunning() const
{
	return TreeInstance.IsValid() && (ActiveStepIndex != INDEX_NONE);
}

bool FBehaviorTreeDebugger::IsShowingCurrentState() const
{
#if USE_BEHAVIORTREE_DEBUGGER
	if (TreeInstance.IsValid() && TreeInstance->DebuggerSteps.Num())
	{
		return (TreeInstance->DebuggerSteps.Num() - 1) == ActiveStepIndex;
	}
#endif

	return false;
}

int32 FBehaviorTreeDebugger::GetShownStateIndex() const
{
#if USE_BEHAVIORTREE_DEBUGGER
	if (TreeInstance.IsValid())
	{
		return (TreeInstance->DebuggerSteps.Num() - 1) - ActiveStepIndex;
	}
#endif

	return 0;
}

void FBehaviorTreeDebugger::ShowNextStep()
{
#if USE_BEHAVIORTREE_DEBUGGER
	const int32 CurStep = ActiveStepIndex;
	const int32 NewStep = ActiveStepIndex + 1;

	if (TreeInstance.IsValid() && TreeInstance->DebuggerSteps.IsValidIndex(NewStep))
	{
		const int32 CurShowIdx = FindActiveInstanceIdx(CurStep);
		const int32 NewShowIdx = FindActiveInstanceIdx(NewStep);

		const FBehaviorTreeExecutionStep& CurStepInfo = TreeInstance->DebuggerSteps[CurStep];
		const FBehaviorTreeExecutionStep& NewStepInfo = TreeInstance->DebuggerSteps[NewStep];

		ActiveStepIndex = NewStep;

		if (NewShowIdx != INDEX_NONE && NewStepInfo.InstanceStack[NewShowIdx].TreeAsset != TreeAsset)
		{
			if (CurShowIdx != NewShowIdx || 
				CurStepInfo.InstanceStack[CurShowIdx].TreeAsset != NewStepInfo.InstanceStack[NewShowIdx].TreeAsset)
			{
				EditorOwner->DebuggerSwitchAsset(NewStepInfo.InstanceStack[NewShowIdx].TreeAsset);
				UpdateCurrentSubtree();
			}
		}

		if (NewStepInfo.InstanceStack.IsValidIndex(DebuggerInstanceIndex))
		{
			const FBehaviorTreeDebuggerInstance& ShowInstance = NewStepInfo.InstanceStack[DebuggerInstanceIndex];
			UpdateAssetFlags(ShowInstance, RootNode.Get(), ActiveStepIndex);
		}
		else
		{
			ActiveStepIndex = INDEX_NONE;

			FBehaviorTreeDebuggerInstance EmptyData;
			UpdateAssetFlags(EmptyData, RootNode.Get(), INDEX_NONE);
		}

		UpdateDebuggerViewOnStepChange();
	}
#endif
}

bool FBehaviorTreeDebugger::CanShowNextStep() const
{
#if USE_BEHAVIORTREE_DEBUGGER
	if (TreeInstance.IsValid())
	{
		if (!bIsCurrentSubtree || ActiveStepIndex == INDEX_NONE || !TreeInstance->DebuggerSteps.IsValidIndex(ActiveStepIndex + 1))
		{
			return false;
		}
	}
#else
	return false;
#endif

	return GUnrealEd->PlayWorld && GUnrealEd->PlayWorld->bDebugPauseExecution;
}

void FBehaviorTreeDebugger::ShowPrevStep()
{
#if USE_BEHAVIORTREE_DEBUGGER
	const int32 CurStep = ActiveStepIndex;
	const int32 NewStep = ActiveStepIndex - 1;

	if (TreeInstance.IsValid() && TreeInstance->DebuggerSteps.IsValidIndex(NewStep))
	{
		const int32 CurShowIdx = FindActiveInstanceIdx(CurStep);
		const int32 NewShowIdx = FindActiveInstanceIdx(NewStep);

		const FBehaviorTreeExecutionStep& CurStepInfo = TreeInstance->DebuggerSteps[CurStep];
		const FBehaviorTreeExecutionStep& NewStepInfo = TreeInstance->DebuggerSteps[NewStep];

		ActiveStepIndex = NewStep;

		if (NewShowIdx != INDEX_NONE && NewStepInfo.InstanceStack[NewShowIdx].TreeAsset != TreeAsset)
		{
			if (CurShowIdx != NewShowIdx || 
				CurStepInfo.InstanceStack[CurShowIdx].TreeAsset != NewStepInfo.InstanceStack[NewShowIdx].TreeAsset)
			{
				EditorOwner->DebuggerSwitchAsset(NewStepInfo.InstanceStack[NewShowIdx].TreeAsset);
				UpdateCurrentSubtree();
			}
		}

		if (NewStepInfo.InstanceStack.IsValidIndex(DebuggerInstanceIndex))
		{
			const FBehaviorTreeDebuggerInstance& ShowInstance = NewStepInfo.InstanceStack[DebuggerInstanceIndex];
			UpdateAssetFlags(ShowInstance, RootNode.Get(), ActiveStepIndex);
		}
		else
		{
			ActiveStepIndex = INDEX_NONE;

			FBehaviorTreeDebuggerInstance EmptyData;
			UpdateAssetFlags(EmptyData, RootNode.Get(), INDEX_NONE);
		}

		UpdateDebuggerViewOnStepChange();
	}
#endif
}

bool FBehaviorTreeDebugger::CanShowPrevStep() const
{
#if USE_BEHAVIORTREE_DEBUGGER
	if (TreeInstance.IsValid())
	{
		if (!bIsCurrentSubtree || ActiveStepIndex == INDEX_NONE || !TreeInstance->DebuggerSteps.IsValidIndex(ActiveStepIndex - 1))
		{
			return false;
		}
	}
#else
	return false;
#endif

	return GUnrealEd->PlayWorld && GUnrealEd->PlayWorld->bDebugPauseExecution;
}

bool FBehaviorTreeDebugger::HasContinuousNextStep() const
{
#if USE_BEHAVIORTREE_DEBUGGER
	if (TreeInstance.IsValid() && TreeInstance->DebuggerSteps.IsValidIndex(ActiveStepIndex + 1))
	{
		const FBehaviorTreeExecutionStep& NextStepInfo = TreeInstance->DebuggerSteps[ActiveStepIndex + 1];
		const FBehaviorTreeExecutionStep& CurStepInfo = TreeInstance->DebuggerSteps[ActiveStepIndex];

		if (CurStepInfo.InstanceStack.IsValidIndex(DebuggerInstanceIndex) &&
			CurStepInfo.InstanceStack.Num() == NextStepInfo.InstanceStack.Num() &&
			CurStepInfo.InstanceStack[DebuggerInstanceIndex].TreeAsset == NextStepInfo.InstanceStack[DebuggerInstanceIndex].TreeAsset)
		{
			return true;
		}
	}
#endif
	return false;
}

bool FBehaviorTreeDebugger::HasContinuousPrevStep() const
{
#if USE_BEHAVIORTREE_DEBUGGER
	if (TreeInstance.IsValid() && TreeInstance->DebuggerSteps.IsValidIndex(ActiveStepIndex - 1))
	{
		const FBehaviorTreeExecutionStep& PrevStepInfo = TreeInstance->DebuggerSteps[ActiveStepIndex - 1];
		const FBehaviorTreeExecutionStep& CurStepInfo = TreeInstance->DebuggerSteps[ActiveStepIndex];

		if (CurStepInfo.InstanceStack.IsValidIndex(DebuggerInstanceIndex) &&
			CurStepInfo.InstanceStack.Num() == PrevStepInfo.InstanceStack.Num() &&
			CurStepInfo.InstanceStack[DebuggerInstanceIndex].TreeAsset == PrevStepInfo.InstanceStack[DebuggerInstanceIndex].TreeAsset)
		{
			return true;
		}
	}
#endif
	return false;
}

void FBehaviorTreeDebugger::StepForward()
{
	bPauseOnNodeChange = true;
	ResumePlaySession();
	UpdateDebuggerViewOnStepChange();
}

bool FBehaviorTreeDebugger::CanStepFoward() const
{
	return bIsCurrentSubtree && IsPlaySessionPaused() && TreeInstance.IsValid();
}

void FBehaviorTreeDebugger::OnActiveNodeChanged(const TArray<uint16>& ActivePath, const TArray<uint16>& PrevStepPath)
{
	bool bShouldPause = false;
	StoppedOnBreakpointExecutionIndex = MAX_uint16;
	if (bPauseOnNodeChange)
	{
		// step by step
		bPauseOnNodeChange = false;
		bShouldPause = true;
	}
	else
	{
		// breakpoints: check only nodes, that have changed from previous state
		// (e.g. breakpoint on sequence, it would break multiple times for every child
		// but we want only once: when it becomes active)
		
		for (int32 i = 0; i < ActivePath.Num(); i++)
		{
			const uint16 TestExecutionIndex = ActivePath[i];
			if (!PrevStepPath.Contains(TestExecutionIndex))
			{
				if (ActiveBreakpoints.Contains(TestExecutionIndex))
				{
					bShouldPause = true;
					StoppedOnBreakpointExecutionIndex = TestExecutionIndex;
					break;
				}
			}
		}
	}

	if (bShouldPause)
	{
		PausePlaySession();
	}
}

void FBehaviorTreeDebugger::StopPlaySession()
{
	if (GUnrealEd->PlayWorld)
	{
		GEditor->RequestEndPlayMap();
	}
}

void FBehaviorTreeDebugger::PausePlaySession()
{
	if (GUnrealEd->PlayWorld)
	{
		GUnrealEd->PlayWorld->bDebugPauseExecution = !GUnrealEd->PlayWorld->bDebugPauseExecution;
	}
}

void FBehaviorTreeDebugger::ResumePlaySession()
{
	if (GUnrealEd->PlayWorld)
	{
		GUnrealEd->PlayWorld->bDebugPauseExecution = !GUnrealEd->PlayWorld->bDebugPauseExecution;
	}
}

bool FBehaviorTreeDebugger::IsPlaySessionPaused()
{
	return GUnrealEd->PlayWorld && GUnrealEd->PlayWorld->bDebugPauseExecution;
}

bool FBehaviorTreeDebugger::IsPlaySessionRunning()
{
	return GUnrealEd->PlayWorld && !GUnrealEd->PlayWorld->bDebugPauseExecution;
}

bool FBehaviorTreeDebugger::IsPIESimulating()
{
	return GEditor->bIsSimulatingInEditor || GEditor->PlayWorld;
}

bool FBehaviorTreeDebugger::IsPIENotSimulating()
{
	return !GEditor->bIsSimulatingInEditor && (GEditor->PlayWorld == NULL);
}

void FBehaviorTreeDebugger::OnBreakpointAdded(class UBehaviorTreeGraphNode* Node)
{
	if (IsDebuggerReady())
	{
		UBTNode* BTNode = Cast<UBTNode>(Node->NodeInstance);
		if (BTNode)
		{
			ActiveBreakpoints.AddUnique(BTNode->GetExecutionIndex());
		}
	}
}

void FBehaviorTreeDebugger::OnBreakpointRemoved(class UBehaviorTreeGraphNode* Node)
{
	if (IsDebuggerReady())
	{
		UBTNode* BTNode = Cast<UBTNode>(Node->NodeInstance);
		if (BTNode)
		{
			ActiveBreakpoints.RemoveSingleSwap(BTNode->GetExecutionIndex());
		}
	}
}

void FBehaviorTreeDebugger::UpdateDebuggerViewOnInstanceChange()
{
#if USE_BEHAVIORTREE_DEBUGGER
	if (DebuggerDetails.IsValid())
	{
		UBlackboardData* BBAsset = NULL;
		if (TreeInstance.IsValid() &&
			TreeInstance->DebuggerSteps.IsValidIndex(ActiveStepIndex) &&
			TreeInstance->DebuggerSteps[ActiveStepIndex].InstanceStack.IsValidIndex(DebuggerInstanceIndex))
		{
			const FBehaviorTreeDebuggerInstance& ShowInstance = TreeInstance->DebuggerSteps[ActiveStepIndex].InstanceStack[DebuggerInstanceIndex];
			if (ShowInstance.TreeAsset)
			{
				BBAsset = ShowInstance.TreeAsset->BlackboardAsset;
			}
		}

		DebuggerDetails->UpdateBlackboard(BBAsset);
	}
#endif
}

void FBehaviorTreeDebugger::UpdateDebuggerViewOnStepChange()
{
#if USE_BEHAVIORTREE_DEBUGGER
	if (IsDebuggerRunning() &&
		DebuggerDetails.IsValid() && TreeInstance.IsValid() &&
		TreeInstance->DebuggerSteps.IsValidIndex(ActiveStepIndex))
	{
		const FBehaviorTreeExecutionStep& ShowStep = TreeInstance->DebuggerSteps[ActiveStepIndex];

		DebuggerDetails->SavedTimestamp = ShowStep.TimeStamp;
		DebuggerDetails->SavedValues = ShowStep.BlackboardValues;
		DebuggerDetails->bCanShowCurrentState = IsShowingCurrentState();
	}
#endif
}

void FBehaviorTreeDebugger::UpdateDebuggerViewOnTick()
{
#if USE_BEHAVIORTREE_DEBUGGER
	if (IsDebuggerRunning() && 
		DebuggerDetails.IsValid() && TreeInstance.IsValid() &&
		DebuggerDetails->IsShowingCurrentState())
	{
		const float GameTime = GEditor && GEditor->PlayWorld ? GEditor->PlayWorld->GetTimeSeconds() : 0.0f;
		DebuggerDetails->CurrentTimestamp = GameTime;

		TreeInstance->StoreDebuggerBlackboard(DebuggerDetails->CurrentValues);
	}
#endif
}

FString FBehaviorTreeDebugger::GetDebuggedInstanceDesc() const
{
	return TreeInstance.IsValid() ? 
		DescribeInstance(TreeInstance.Get()) :
		NSLOCTEXT("BlueprintEditor", "DebugActorNothingSelected", "No debug object selected").ToString();
}

FString FBehaviorTreeDebugger::DescribeInstance(class UBehaviorTreeComponent* InstanceToDescribe) const
{
	FString ActorDesc;
	if (InstanceToDescribe && InstanceToDescribe->GetOwner())
	{
		AController* TestController = Cast<AController>(InstanceToDescribe->GetOwner());
		ActorDesc = (TestController && TestController->GetPawn()) ?
			TestController->GetPawn()->GetName() : 
			InstanceToDescribe->GetOwner()->GetName();
	}

	return ActorDesc;
}

void FBehaviorTreeDebugger::OnInstanceSelectedInDropdown(class UBehaviorTreeComponent* SelectedInstance)
{
	if (SelectedInstance)
	{
		ClearDebuggerState();
		TreeInstance = SelectedInstance;
	}
}

void FBehaviorTreeDebugger::GetMatchingInstances(TArray<class UBehaviorTreeComponent*>& MatchingInstances)
{
	for (int32 i = KnownInstances.Num() - 1; i >= 0; i--)
	{
		UBehaviorTreeComponent* TestInstance = KnownInstances[i].Get();
		if (TestInstance == NULL)
		{
			KnownInstances.RemoveAt(i);
			continue;
		}

		const int32 StackIdx = FindMatchingDebuggerStack(TestInstance);
		if (StackIdx != INDEX_NONE)
		{
			MatchingInstances.Add(TestInstance);
		}
	}
}

void FBehaviorTreeDebugger::InitializeFromParent(class FBehaviorTreeDebugger* ParentDebugger)
{
	ClearDebuggerState();

#if USE_BEHAVIORTREE_DEBUGGER
	TreeInstance = ParentDebugger->TreeInstance;
	ActiveStepIndex = ParentDebugger->ActiveStepIndex;

	UpdateDebuggerInstance();

	if (TreeInstance.IsValid() &&
		TreeInstance->DebuggerSteps.IsValidIndex(ActiveStepIndex) &&
		TreeInstance->DebuggerSteps[ActiveStepIndex].InstanceStack.IsValidIndex(DebuggerInstanceIndex))
	{
		const FBehaviorTreeDebuggerInstance& ShowInstance = TreeInstance->DebuggerSteps[ActiveStepIndex].InstanceStack[DebuggerInstanceIndex];
		UpdateAssetFlags(ShowInstance, RootNode.Get(), ActiveStepIndex);
	}
#endif
}

int32 FBehaviorTreeDebugger::FindActiveInstanceIdx(int32 StepIdx) const
{
#if USE_BEHAVIORTREE_DEBUGGER
	const FBehaviorTreeExecutionStep& StepInfo = TreeInstance->DebuggerSteps[StepIdx];
	for (int32 i = StepInfo.InstanceStack.Num() - 1; i >= 0; i--)
	{
		if (StepInfo.InstanceStack[i].IsValid())
		{
			return i;
		}
	}
#endif

	return INDEX_NONE;
}

void FBehaviorTreeDebugger::UpdateCurrentSubtree()
{
	bIsCurrentSubtree = false;

#if USE_BEHAVIORTREE_DEBUGGER
	if (TreeInstance->DebuggerSteps.IsValidIndex(ActiveStepIndex))
	{
		const FBehaviorTreeExecutionStep& StepInfo = TreeInstance->DebuggerSteps[ActiveStepIndex];

		// assume that top instance is always valid, so it won't take away step buttons when tree is finished as out of nodes
		// current subtree = no child instances, or child instances are not valid
		bIsCurrentSubtree = ((DebuggerInstanceIndex == 0) || (StepInfo.InstanceStack.IsValidIndex(DebuggerInstanceIndex) && StepInfo.InstanceStack[DebuggerInstanceIndex].IsValid())) &&
			(!StepInfo.InstanceStack.IsValidIndex(DebuggerInstanceIndex + 1) || !StepInfo.InstanceStack[DebuggerInstanceIndex + 1].IsValid());
	}
#endif
}
