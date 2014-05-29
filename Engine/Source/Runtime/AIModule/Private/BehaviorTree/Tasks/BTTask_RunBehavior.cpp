// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "BehaviorTree/Tasks/BTTask_RunBehavior.h"
#include "BehaviorTree/BehaviorTreeManager.h"

UBTTask_RunBehavior::UBTTask_RunBehavior(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = "Run Behavior";
}

EBTNodeResult::Type UBTTask_RunBehavior::ExecuteTask(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory)
{
	const bool bPushed = OwnerComp->PushInstance(BehaviorAsset);
	return bPushed ? EBTNodeResult::InProgress : EBTNodeResult::Failed;
}

FString UBTTask_RunBehavior::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: %s"), *Super::GetStaticDescription(), *GetNameSafe(BehaviorAsset));
}

uint16 UBTTask_RunBehavior::GetInstanceMemorySize() const
{
	// memory required by root level decorators is provided by this task
	// it's not possible to have dynamic subtrees supporting root level decorators
	// (unnecessary complication of accessing node's memory)

	int32 MemorySize = 0;
	if (BehaviorAsset)
	{
		TArray<uint16> MemoryOffsets;
		UBehaviorTreeManager::InitializeMemoryHelper(BehaviorAsset->RootDecorators, MemoryOffsets, MemorySize);
	}

	return MemorySize;
}

void UBTTask_RunBehavior::InjectNodes(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, int32& InstancedIndex) const
{
	if (BehaviorAsset == NULL || BehaviorAsset->RootDecorators.Num() == 0)
	{
		return;
	}

	const int32 NumInjectedDecorators = BehaviorAsset->RootDecorators.Num();
	const int32 FirstNodeIdx = InstancedIndex;

	// initialize on first access
	if (!OwnerComp->NodeInstances.IsValidIndex(InstancedIndex))
	{
		TArray<uint16> MemoryOffsets;
		int32 MemorySize = 0;

		UBehaviorTreeManager::InitializeMemoryHelper(BehaviorAsset->RootDecorators, MemoryOffsets, MemorySize);
		const int32 AlignedInstanceMemorySize = UBehaviorTreeManager::GetAlignedDataSize(sizeof(FBTInstancedNodeMemory));

		// prepare dummy memory block for nodes that won't require instancing and offset it by special data size
		// InitializeForInstance will read it through GetSpecialNodeMemory function, which moves pointer back 
		FBTInstancedNodeMemory DummyMemory;
		uint8* RawDummyMemory = ((uint8*)&DummyMemory) + AlignedInstanceMemorySize;

		// create nodes
		for (int32 Idx = 0; Idx < NumInjectedDecorators; Idx++)
		{
			UBTDecorator* DecoratorOb = BehaviorAsset->RootDecorators[Idx];

			const bool bUsesInstancing = DecoratorOb->HasInstance();
			if (bUsesInstancing)
			{
				DecoratorOb->InitializeForInstance(OwnerComp, NodeMemory + MemoryOffsets[Idx], InstancedIndex);
			}
			else
			{
				DecoratorOb->ForceInstancing(true);
				DecoratorOb->InitializeForInstance(OwnerComp, RawDummyMemory, InstancedIndex);
				DecoratorOb->ForceInstancing(false);

				UBTDecorator* InstancedOb = Cast<UBTDecorator>(OwnerComp->NodeInstances.Last());
				InstancedOb->InitializeMemory(OwnerComp, NodeMemory + MemoryOffsets[Idx]);
			}
		}

		// setup their properties
		uint16 NewExecutionIdx = GetExecutionIndex() - GetInjectedNodesCount();
		for (int32 Idx = 0; Idx < NumInjectedDecorators; Idx++)
		{
			UBTDecorator* InstancedOb = Cast<UBTDecorator>(OwnerComp->NodeInstances[FirstNodeIdx + Idx]);
			InstancedOb->InitializeNode(GetParentNode(), NewExecutionIdx, GetMemoryOffset() + MemoryOffsets[Idx], GetTreeDepth() - 1);
			InstancedOb->MarkInjectedNode();

			NewExecutionIdx++;
		}
	}
	else
	{
		// initialize memory for injected nodes
		for (int32 Idx = 0; Idx < NumInjectedDecorators; Idx++)
		{
			UBTDecorator* DecoratorOb = BehaviorAsset->RootDecorators[Idx];
			UBTDecorator* InstancedOb = Cast<UBTDecorator>(OwnerComp->NodeInstances[FirstNodeIdx + Idx]);

			const bool bUsesInstancing = DecoratorOb->HasInstance();
			if (bUsesInstancing)
			{
				InstancedOb->OnInstanceCreated(OwnerComp);
			}
			else
			{
				uint8* InjectedNodeMemory = NodeMemory + (InstancedOb->GetMemoryOffset() - GetMemoryOffset());
				InstancedOb->InitializeMemory(OwnerComp, InjectedNodeMemory);
			}
		}

		InstancedIndex += NumInjectedDecorators;
	}

	// inject nodes
	if (GetParentNode())
	{
		const int32 ChildIdx = GetParentNode()->GetChildIndex(this);
		if (ChildIdx != INDEX_NONE)
		{
			FBTCompositeChild& LinkData = GetParentNode()->Children[ChildIdx];

			// check if link already has injected decorators
			bool bAlreadyInjected = false;
			for (int32 Idx = 0; Idx < LinkData.Decorators.Num(); Idx++)
			{
				if (LinkData.Decorators[Idx] && LinkData.Decorators[Idx]->IsInjected())
				{
					bAlreadyInjected = true;
					break;
				}
			}

			// add decorators to link
			const int32 NumOriginalDecorators = LinkData.Decorators.Num();
			for (int32 Idx = 0; Idx < NumInjectedDecorators; Idx++)
			{
				UBTDecorator* InstancedOb = Cast<UBTDecorator>(OwnerComp->NodeInstances[FirstNodeIdx + Idx]);
				InstancedOb->InitializeFromAsset(BehaviorAsset);
				InstancedOb->InitializeDecorator(ChildIdx);

				if (!bAlreadyInjected)
				{
					LinkData.Decorators.Add(InstancedOb);
				}
			}

			// update composite logic operators
			if (!bAlreadyInjected && (LinkData.DecoratorOps.Num() || BehaviorAsset->RootDecoratorOps.Num()))
			{
				const int32 NumOriginalOps = LinkData.DecoratorOps.Num();
				if (NumOriginalDecorators > 0)
				{
					// and operator for two groups of composites: original and new one
					FBTDecoratorLogic MasterAndOp(EBTDecoratorLogic::And, LinkData.DecoratorOps.Num() ? 2 : (NumOriginalDecorators + 1));
					LinkData.DecoratorOps.Insert(MasterAndOp, 0);

					if (NumOriginalOps == 0)
					{
						// add Test operations, original link didn't have composite operators
						for (int32 Idx = 0; Idx < NumOriginalDecorators; Idx++)
						{
							FBTDecoratorLogic TestOp(EBTDecoratorLogic::Test, Idx);
							LinkData.DecoratorOps.Add(TestOp);
						}
					}
				}

				// add injected operators
				if (BehaviorAsset->RootDecoratorOps.Num() == 0)
				{
					FBTDecoratorLogic InjectedAndOp(EBTDecoratorLogic::And, NumInjectedDecorators);
					LinkData.DecoratorOps.Add(InjectedAndOp);

					for (int32 Idx = 0; Idx < NumInjectedDecorators; Idx++)
					{
						FBTDecoratorLogic TestOp(EBTDecoratorLogic::Test, NumOriginalDecorators + Idx);
						LinkData.DecoratorOps.Add(TestOp);
					}
				}
				else
				{
					for (int32 Idx = 0; Idx < BehaviorAsset->RootDecoratorOps.Num(); Idx++)
					{
						FBTDecoratorLogic InjectedOp = BehaviorAsset->RootDecoratorOps[Idx];
						if (InjectedOp.Operation == EBTDecoratorLogic::Test)
						{
							InjectedOp.Number += NumOriginalDecorators;
						}

						LinkData.DecoratorOps.Add(InjectedOp);
					}
				}
			}

#if USE_BEHAVIORTREE_DEBUGGER
			if (!bAlreadyInjected)
			{
				// insert to NextExecutionNode list for debugger
				UBTNode* NodeIt = GetParentNode();
				while (NodeIt && NodeIt->GetNextNode() != this)
				{
					NodeIt = NodeIt->GetNextNode();
				}

				if (NodeIt)
				{
					NodeIt->InitializeExecutionOrder(OwnerComp->NodeInstances[FirstNodeIdx]);
					NodeIt = NodeIt->GetNextNode();

					for (int32 Idx = 1; Idx < NumInjectedDecorators; Idx++)
					{
						NodeIt->InitializeExecutionOrder(OwnerComp->NodeInstances[FirstNodeIdx + Idx]);
						NodeIt = NodeIt->GetNextNode();
					}

					NodeIt->InitializeExecutionOrder((UBTNode*)this);
				}
			}
#endif
		}
	}
}
