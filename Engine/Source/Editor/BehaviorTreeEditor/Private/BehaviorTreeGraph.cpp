// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "BehaviorTreeEditorPrivatePCH.h"
#include "BehaviorTreeEditorModule.h"

//////////////////////////////////////////////////////////////////////////
// BehaviorTreeGraph

UBehaviorTreeGraph::UBehaviorTreeGraph(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	Schema = UEdGraphSchema_BehaviorTree::StaticClass();
}

void UBehaviorTreeGraph::UpdateBlackboardChange()
{
	UBehaviorTree* BTAsset = Cast<UBehaviorTree>(GetOuter());
	for (int32 Index = 0; Index < Nodes.Num(); ++Index)
	{
		UBehaviorTreeGraphNode* MyNode = Cast<UBehaviorTreeGraphNode>(Nodes[Index]);

		if (MyNode)
		{
			UBTNode* MyNodeInstance = Cast<UBTNode>(MyNode->NodeInstance);
			if (MyNodeInstance)
			{
				MyNodeInstance->InitializeFromAsset(BTAsset);
			}
		}

		for (int32 iDecorator = 0; iDecorator < MyNode->Decorators.Num(); iDecorator++)
		{
			UBTNode* MyNodeInstance = MyNode->Decorators[iDecorator] ? Cast<UBTNode>(MyNode->Decorators[iDecorator]->NodeInstance) : NULL;
			if (MyNodeInstance)
			{
				MyNodeInstance->InitializeFromAsset(BTAsset);
			}

			UBehaviorTreeGraphNode_CompositeDecorator* CompDecoratorNode = Cast<UBehaviorTreeGraphNode_CompositeDecorator>(MyNode->Decorators[iDecorator]);
			if (CompDecoratorNode)
			{
				CompDecoratorNode->OnBlackboardUpdate();
			}
		}

		for (int32 iService = 0; iService < MyNode->Services.Num(); iService++)
		{
			UBTNode* MyNodeInstance = MyNode->Services[iService] ? Cast<UBTNode>(MyNode->Services[iService]->NodeInstance) : NULL;
			if (MyNodeInstance)
			{
				MyNodeInstance->InitializeFromAsset(BTAsset);
			}
		}
	}
}

void UBehaviorTreeGraph::UpdateAsset(EDebuggerFlags DebuggerFlags)
{
	// initial cleanup & root node search
	UBehaviorTreeGraphNode_Root* RootNode = NULL;
	for (int32 Index = 0; Index < Nodes.Num(); ++Index)
	{
		UBehaviorTreeGraphNode* Node = Cast<UBehaviorTreeGraphNode>(Nodes[Index]);
		if (DebuggerFlags == ClearDebuggerFlags)
		{
			Node->ClearDebuggerState();

			for (int32 iAux = 0; iAux < Node->Services.Num(); iAux++)
			{
				Node->Services[iAux]->ClearDebuggerState();
			}

			for (int32 iAux = 0; iAux < Node->Decorators.Num(); iAux++)
			{
				Node->Decorators[iAux]->ClearDebuggerState();
			}
		}

		UBTNode* NodeInstance = Cast<UBTNode>(Node->NodeInstance);
		if (NodeInstance)
		{
			// mark all nodes as disconnected first, path from root will replcae it with valid values later
			NodeInstance->InitializeNode(NULL, MAX_uint16, 0, 0);
		}

		if (RootNode == NULL)
		{
			RootNode = Cast<UBehaviorTreeGraphNode_Root>(Nodes[Index]);
		}

		UBehaviorTreeGraphNode_CompositeDecorator* CompositeDecorator = Cast<UBehaviorTreeGraphNode_CompositeDecorator>(Nodes[Index]);
		if (CompositeDecorator)
		{
			CompositeDecorator->ResetExecutionRange();
		}
	}

	if (RootNode && RootNode->Pins.Num() > 0 && RootNode->Pins[0]->LinkedTo.Num() > 0)
	{
		UBehaviorTreeGraphNode* Node = Cast<UBehaviorTreeGraphNode>(RootNode->Pins[0]->LinkedTo[0]->GetOwningNode());
		if (Node)
		{
			CreateBTFromGraph(Node);
		}
	}
}

void UBehaviorTreeGraph::UpdatePinConnectionTypes()
{
	for (int32 Index = 0; Index < Nodes.Num(); ++Index)
	{
		UEdGraphNode* Node = Nodes[Index];

		const bool bIsCompositeNode = Node && Node->IsA(UBehaviorTreeGraphNode_Composite::StaticClass());

		for (int32 iPin = 0; iPin < Node->Pins.Num(); iPin++)
		{
			FString& PinCategory = Node->Pins[iPin]->PinType.PinCategory;
			if (PinCategory == TEXT("Transition"))
			{
				PinCategory = bIsCompositeNode ? 
					UBehaviorTreeEditorTypes::PinCategory_MultipleNodes :
					UBehaviorTreeEditorTypes::PinCategory_SingleComposite;
			}
		}
	}
}

void CreateBTChildren(UBehaviorTree* BTAsset, UBTCompositeNode* RootNode, const UBehaviorTreeGraphNode* RootEdNode, uint16* ExecutionIndex, uint8 TreeDepth)
{
	if (RootEdNode == NULL || RootEdNode == NULL)
	{
		return;
	}

	RootNode->Children.Reset();
	RootNode->Services.Reset();

	// collect services
	if (RootEdNode->Services.Num())
	{
		for (int32 i = 0; i < RootEdNode->Services.Num(); i++)
		{
			UBTService* ServiceInstance = RootEdNode->Services[i] ? Cast<UBTService>(RootEdNode->Services[i]->NodeInstance) : NULL;
			if (ServiceInstance)
			{
				if ( Cast<UBehaviorTree>(ServiceInstance->GetOuter()) == NULL)
				{
					ServiceInstance->Rename(NULL, BTAsset);
				}
				ServiceInstance->InitializeNode(RootNode, *ExecutionIndex, 0, TreeDepth);
				*ExecutionIndex += 1;

				RootNode->Services.Add(ServiceInstance);
			}
		}
	}

	// gather all nodes
	for (int32 PinIdx = 0; PinIdx < RootEdNode->Pins.Num(); PinIdx++)
	{
		UEdGraphPin* Pin = RootEdNode->Pins[PinIdx];
		if (Pin->Direction != EGPD_Output)
		{
			continue;
		}

		// sort connections so that they're organized the same as user can see in the editor
		Pin->LinkedTo.Sort(FCompareNodeXLocation());

		for (int32 Index=0; Index < Pin->LinkedTo.Num(); ++Index)
		{
			UBehaviorTreeGraphNode* GraphNode = Cast<UBehaviorTreeGraphNode>(Pin->LinkedTo[Index]->GetOwningNode());
			if (GraphNode == NULL)
			{
				continue;
			}

			UBTTaskNode* TaskInstance = Cast<UBTTaskNode>(GraphNode->NodeInstance);
			if ( TaskInstance && Cast<UBehaviorTree>(TaskInstance->GetOuter()) == NULL)
			{
				TaskInstance->Rename(NULL, BTAsset);
			}

			UBTCompositeNode* CompositeInstance = Cast<UBTCompositeNode>(GraphNode->NodeInstance);
			if ( CompositeInstance && Cast<UBehaviorTree>(CompositeInstance->GetOuter()) == NULL)
			{
				CompositeInstance->Rename(NULL, BTAsset);
			}

			if (TaskInstance == NULL && CompositeInstance == NULL)
			{
				continue;
			}

			// collect decorators
			TArray<UBTDecorator*> DecoratorInstances;
			TArray<FBTDecoratorLogic> DecoratorOperations;
			{
				struct FIntIntPair
				{
					int32 FirstIdx;
					int32 LastIdx;
				};

				TMap<UBehaviorTreeGraphNode_CompositeDecorator*,FIntIntPair> CompositeMap;
				int32 NumNodes = 0;

				for (int32 i = 0; i < GraphNode->Decorators.Num(); i++)
				{
					UBehaviorTreeGraphNode_Decorator* MyDecoratorNode = Cast<UBehaviorTreeGraphNode_Decorator>(GraphNode->Decorators[i]);
					UBehaviorTreeGraphNode_CompositeDecorator* MyCompositeNode = Cast<UBehaviorTreeGraphNode_CompositeDecorator>(GraphNode->Decorators[i]);

					if (MyDecoratorNode)
					{
						MyDecoratorNode->CollectDecoratorData(DecoratorInstances, DecoratorOperations);
						NumNodes++;
					}
					else if (MyCompositeNode)
					{
						MyCompositeNode->SetDecoratorData(RootNode, RootNode->Children.Num());
						
						FIntIntPair RangeData;
						RangeData.FirstIdx = DecoratorInstances.Num();
						
						MyCompositeNode->CollectDecoratorData(DecoratorInstances, DecoratorOperations);
						NumNodes++;
						
						RangeData.LastIdx = DecoratorInstances.Num() - 1;
						CompositeMap.Add(MyCompositeNode, RangeData);						
					}
				}

				for (int32 i = 0; i < DecoratorInstances.Num(); i++)
				{
					if ( DecoratorInstances[i] != NULL && Cast<UBehaviorTree>(DecoratorInstances[i]->GetOuter()) == NULL)
					{
						DecoratorInstances[i]->Rename(NULL, BTAsset);
					}
					DecoratorInstances[i]->InitializeNode(RootNode, *ExecutionIndex, 0, TreeDepth);
					DecoratorInstances[i]->InitializeDecorator(RootNode->Children.Num());
					*ExecutionIndex += 1;

					// make sure that flow abort mode matches
					DecoratorInstances[i]->ValidateFlowAbortMode();
				}

				for (TMap<UBehaviorTreeGraphNode_CompositeDecorator*,FIntIntPair>::TIterator It(CompositeMap); It; ++It)
				{
					UBehaviorTreeGraphNode_CompositeDecorator* Node = It.Key();
					const FIntIntPair& PairInfo = It.Value();
					
					if (DecoratorInstances.IsValidIndex(PairInfo.FirstIdx) &&
						DecoratorInstances.IsValidIndex(PairInfo.LastIdx))
					{
						Node->FirstExecutionIndex = DecoratorInstances[PairInfo.FirstIdx]->GetExecutionIndex();
						Node->LastExecutionIndex = DecoratorInstances[PairInfo.LastIdx]->GetExecutionIndex();
					}
				}

				// setup logic operations only when composite decorator is linked
				if (CompositeMap.Num())
				{
					if (NumNodes > 1)
					{
						FBTDecoratorLogic LogicOp(EBTDecoratorLogic::And, NumNodes);
						DecoratorOperations.Insert(LogicOp, 0);
					}
				}
				else
				{
					DecoratorOperations.Reset();
				}
			}

			int32 ChildIdx = RootNode->Children.AddZeroed();
			FBTCompositeChild& ChildInfo = RootNode->Children[ChildIdx];
			ChildInfo.ChildComposite = CompositeInstance;
			ChildInfo.ChildTask = TaskInstance;
			ChildInfo.Decorators = DecoratorInstances;
			ChildInfo.DecoratorOps = DecoratorOperations;

			UBTNode* ChildNode = CompositeInstance ? (UBTNode*)CompositeInstance : (UBTNode*)TaskInstance;
			if ( ChildNode && Cast<UBehaviorTree>(ChildNode->GetOuter()) == NULL)
			{
				ChildNode->Rename(NULL, BTAsset);
			}

			ChildNode->InitializeNode(RootNode, *ExecutionIndex, 0, TreeDepth);
			*ExecutionIndex += 1;

			if (CompositeInstance)
			{
				CreateBTChildren(BTAsset, CompositeInstance, GraphNode, ExecutionIndex, TreeDepth + 1);
				
				CompositeInstance->InitializeComposite((*ExecutionIndex) - 1);
			}
		}
	}
}

void UBehaviorTreeGraph::CreateBTFromGraph(UBehaviorTreeGraphNode* RootEdNode)
{
	UE_LOG(LogBehaviorTreeEditor, Log, TEXT("Updating BT changes..."));
	const double StartTime = FPlatformTime::Seconds();

	UBehaviorTree* BTAsset = Cast<UBehaviorTree>(GetOuter());
	BTAsset->RootNode = NULL; //discard old tree

	// let's create new tree from graph
	uint16 ExecutionIndex = 0;
	uint8 TreeDepth = 0;

	BTAsset->RootNode = Cast<UBTCompositeNode>(RootEdNode->NodeInstance);
	if (BTAsset->RootNode)
	{
		BTAsset->RootNode->InitializeNode(NULL, ExecutionIndex, 0, TreeDepth);
		ExecutionIndex++;
	}
	
	CreateBTChildren(BTAsset, BTAsset->RootNode, RootEdNode, &ExecutionIndex, TreeDepth + 1);

	if (BTAsset->RootNode)
	{
		BTAsset->RootNode->InitializeComposite(ExecutionIndex - 1);
	}
}

void UBehaviorTreeGraph::UpdateAbortHighlight(struct FAbortDrawHelper& Mode0, struct FAbortDrawHelper& Mode1)
{
	for (int32 Index = 0; Index < Nodes.Num(); ++Index)
	{
		UBehaviorTreeGraphNode* Node = Cast<UBehaviorTreeGraphNode>(Nodes[Index]);
		UBTNode* NodeInstance = Node ? Cast<UBTNode>(Node->NodeInstance) : NULL;
		if (NodeInstance)
		{
			const uint16 ExecIndex = NodeInstance->GetExecutionIndex();

			Node->bHighlightInAbortRange0 = (ExecIndex != MAX_uint16) && (ExecIndex >= Mode0.AbortStart) && (ExecIndex <= Mode0.AbortEnd);
			Node->bHighlightInAbortRange1 = (ExecIndex != MAX_uint16) && (ExecIndex >= Mode1.AbortStart) && (ExecIndex <= Mode1.AbortEnd);
			Node->bHighlightInSearchRange0 = (ExecIndex != MAX_uint16) && (ExecIndex >= Mode0.SearchStart) && (ExecIndex <= Mode0.SearchEnd);
			Node->bHighlightInSearchRange1 = (ExecIndex != MAX_uint16) && (ExecIndex >= Mode1.SearchStart) && (ExecIndex <= Mode1.SearchEnd);
			Node->bHighlightInSearchTree = false;
		}
	}
}
