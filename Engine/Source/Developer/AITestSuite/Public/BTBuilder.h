// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"
#include "BehaviorTree/Composites/BTComposite_SimpleParallel.h"
#include "BehaviorTree/Decorators//BTDecorator_ForceSuccess.h"

#include "BehaviorTree/TestBTTask_Log.h"

struct FBTBuilder
{
	static UBehaviorTree& CreateBehaviorTree()
	{
		UBlackboardData* BB = NewObject<UBlackboardData>();
		FBlackboardEntry KeyData;

		KeyData.EntryName = TEXT("Bool1");
		KeyData.KeyType = NewObject<UBlackboardKeyType_Bool>();
		BB->Keys.Add(KeyData);

		KeyData.EntryName = TEXT("Bool2");
		KeyData.KeyType = NewObject<UBlackboardKeyType_Bool>();
		BB->Keys.Add(KeyData);

		UBehaviorTree* TreeOb = NewObject<UBehaviorTree>();
		TreeOb->BlackboardAsset = BB;

		return *TreeOb;
	}

	static UBTComposite_Selector& AddSelector(UBehaviorTree& TreeOb)
	{
		UBTComposite_Selector* NodeOb = NewObject<UBTComposite_Selector>(&TreeOb);
		NodeOb->InitializeFromAsset(&TreeOb);
		TreeOb.RootNode = NodeOb;
		return *NodeOb;
	}

	static UBTComposite_Selector& AddSelector(UBTCompositeNode& ParentNode)
	{
		UBTComposite_Selector* NodeOb = NewObject<UBTComposite_Selector>(ParentNode.GetTreeAsset());
		NodeOb->InitializeFromAsset(ParentNode.GetTreeAsset());
		return *NodeOb;
	}

	static UBTComposite_Sequence& AddSequence(UBehaviorTree& TreeOb)
	{
		UBTComposite_Sequence* NodeOb = NewObject<UBTComposite_Sequence>(&TreeOb);
		NodeOb->InitializeFromAsset(&TreeOb);
		TreeOb.RootNode = NodeOb;
		return *NodeOb;
	}

	static UBTComposite_Sequence& AddSequence(UBTCompositeNode& ParentNode)
	{
		UBTComposite_Sequence* NodeOb = NewObject<UBTComposite_Sequence>(ParentNode.GetTreeAsset());
		NodeOb->InitializeFromAsset(ParentNode.GetTreeAsset());
		return *NodeOb;
	}

	static UBTComposite_SimpleParallel& AddParallel(UBehaviorTree& TreeOb)
	{
		UBTComposite_SimpleParallel* NodeOb = NewObject<UBTComposite_SimpleParallel>(&TreeOb);
		NodeOb->InitializeFromAsset(&TreeOb);
		TreeOb.RootNode = NodeOb;
		return *NodeOb;
	}

	static UBTComposite_SimpleParallel& AddParallel(UBTCompositeNode& ParentNode)
	{
		UBTComposite_SimpleParallel* NodeOb = NewObject<UBTComposite_SimpleParallel>(ParentNode.GetTreeAsset());
		NodeOb->InitializeFromAsset(ParentNode.GetTreeAsset());
		return *NodeOb;
	}

	static void AddTask(UBTCompositeNode& ParentNode, int32 LogIndex, EBTNodeResult::Type NodeResult, float ExecutionTime = 0.0f)
	{
		UTestBTTask_Log* TaskNode = NewObject<UTestBTTask_Log>(ParentNode.GetTreeAsset());
		TaskNode->LogIndex = LogIndex;
		TaskNode->LogResult = NodeResult;
		TaskNode->ExecutionTime = ExecutionTime;

		const int32 ChildIdx = ParentNode.Children.AddZeroed(1);
		ParentNode.Children[ChildIdx].ChildTask = TaskNode;
	}

	template<class T>
	static T& WithDecorator(UBTCompositeNode& ParentNode, UClass* DecoratorClass = T::StaticClass())
	{
		T* DecoratorOb = NewObject<T>(ParentNode.GetTreeAsset());
		ParentNode.Children.Last().Decorators.Add(DecoratorOb);

		return *DecoratorOb;
	}
};
