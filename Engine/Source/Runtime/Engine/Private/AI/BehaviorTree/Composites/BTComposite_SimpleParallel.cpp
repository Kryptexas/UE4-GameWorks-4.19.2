// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBTComposite_SimpleParallel::UBTComposite_SimpleParallel(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = "Simple Parallel";
	bUseChildExecutionNotify = true;
	bUseNodeDeactivationNotify = true;
	OnNextChild.BindUObject(this, &UBTComposite_SimpleParallel::GetNextChildHandler);
}

int32 UBTComposite_SimpleParallel::GetNextChildHandler(struct FBehaviorTreeSearchData& SearchData, int32 PrevChild, EBTNodeResult::Type LastResult) const
{
	FBTParallelMemory* MyMemory = GetNodeMemory<FBTParallelMemory>(SearchData);
	int32 NextChildIdx = BTSpecialChild::ReturnToParent;

	if (PrevChild == BTSpecialChild::NotInitialized)
	{
		NextChildIdx = EBTParallelChild::MainTask;
	}
	else if ((MyMemory->bMainTaskIsActive || MyMemory->bForceBackgroundTree) && !SearchData.OwnerComp->IsRestartPending())
	{
		// if main task is running - always pick background tree
		// unless search is from abort from background tree - return to parent (and break from search when node gets deactivated)
		NextChildIdx = EBTParallelChild::BackgroundTree;
		MyMemory->bForceBackgroundTree = false;
	}

	return NextChildIdx;
}

void UBTComposite_SimpleParallel::NotifyChildExecution(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, int32 ChildIdx, EBTNodeResult::Type& NodeResult) const
{
	FBTParallelMemory* MyMemory = (FBTParallelMemory*)NodeMemory;
	if (ChildIdx == EBTParallelChild::MainTask)
	{
		if (NodeResult == EBTNodeResult::InProgress)
		{
			EBTTaskStatus::Type Status = OwnerComp->GetTaskStatus(Children[EBTParallelChild::MainTask].ChildTask);
			if (Status == EBTTaskStatus::Active)
			{
				// can't register if task is currently being aborted (latent abort returns in progress)

				MyMemory->bMainTaskIsActive = true;
				MyMemory->bForceBackgroundTree = false;
				
				OwnerComp->RegisterParallelTask(Children[EBTParallelChild::MainTask].ChildTask);
				RequestDelayedExecution(OwnerComp, EBTNodeResult::Succeeded);
			}
		}
		else if (MyMemory->bMainTaskIsActive)
		{
			MyMemory->MainTaskResult = NodeResult;
			MyMemory->bMainTaskIsActive = false;

			OwnerComp->UnregisterParallelTask(Children[EBTParallelChild::MainTask].ChildTask);
			if (NodeResult != EBTNodeResult::Aborted)
			{
				// check if subtree should be aborted when task finished with success/failed result
				if (FinishMode == EBTParallelMode::AbortBackground)
				{
					OwnerComp->RequestExecution((UBTCompositeNode*)this, OwnerComp->GetActiveInstanceIdx(),
						Children[EBTParallelChild::MainTask].ChildTask, EBTParallelChild::MainTask,
						NodeResult);
				}
			}
		}
		else if (NodeResult == EBTNodeResult::Succeeded && FinishMode == EBTParallelMode::WaitForBackground)
		{
			// special case: if main task finished instantly, but composite is supposed to wait for background tree,
			// request single run of background tree anyway

			MyMemory->bForceBackgroundTree = true;

			RequestDelayedExecution(OwnerComp, EBTNodeResult::Succeeded);
		}
	}
}

void UBTComposite_SimpleParallel::NotifyNodeDeactivation(struct FBehaviorTreeSearchData& SearchData, EBTNodeResult::Type& NodeResult) const
{
	FBTParallelMemory* MyMemory = GetNodeMemory<FBTParallelMemory>(SearchData);
	const uint16 ActiveInstanceIdx = SearchData.OwnerComp->GetActiveInstanceIdx();

	// modify node result if main task has finished
	if (!MyMemory->bMainTaskIsActive)
	{
		NodeResult = MyMemory->MainTaskResult;
	}

	// remove main task
	SearchData.AddUniqueUpdate(FBehaviorTreeSearchUpdate(Children[EBTParallelChild::MainTask].ChildTask, ActiveInstanceIdx, EBTNodeUpdateMode::Remove));

	// remove all active nodes from background tree
	const FBTNodeIndex FirstBackgroundIndex(ActiveInstanceIdx, GetChildExecutionIndex(EBTParallelChild::MainTask) + 1);
	SearchData.OwnerComp->UnregisterAuxNodesUpTo(FirstBackgroundIndex);
}

uint16 UBTComposite_SimpleParallel::GetInstanceMemorySize() const
{
	return sizeof(FBTParallelMemory);
}

FString UBTComposite_SimpleParallel::DescribeFinishMode(EBTParallelMode::Type Mode)
{
	static FString FinishDesc[] = { TEXT("AbortBackground"), TEXT("WaitForBackground") };
	return FinishDesc[Mode];
}

FString UBTComposite_SimpleParallel::GetStaticDescription() const
{
	static FString FinishDesc[] = { TEXT("finish with main task"), TEXT("wait for subtree") };

	return FString::Printf(TEXT("%s: %s"), *Super::GetStaticDescription(),
		FinishMode < 2 ? *FinishDesc[FinishMode] : TEXT(""));
}

void UBTComposite_SimpleParallel::DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	Super::DescribeRuntimeValues(OwnerComp, NodeMemory, Verbosity, Values);

	if (Verbosity == EBTDescriptionVerbosity::Detailed)
	{
		FBTParallelMemory* MyMemory = (FBTParallelMemory*)NodeMemory;
		Values.Add(FString::Printf(TEXT("main task: %s"), MyMemory->bMainTaskIsActive ? TEXT("active") : TEXT("not active")));

		if (MyMemory->bForceBackgroundTree)
		{
			Values.Add(TEXT("force background tree"));
		}
	}
}

#if WITH_EDITOR

bool UBTComposite_SimpleParallel::CanAbortLowerPriority() const
{
	return false;
}

bool UBTComposite_SimpleParallel::CanAbortSelf() const
{
	return false;
}

#endif // WITH_EDITOR
