// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AITestSuitePrivatePCH.h"
#include "MockAI_BT.h"

//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//
TArray<int32> UMockAI_BT::ExecutionLog;

UMockAI_BT::UMockAI_BT(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		UseBlackboardComponent();
		UseBrainComponent<UBehaviorTreeComponent>();

		BTComp = Cast<UBehaviorTreeComponent>(BrainComp);
	}
}

bool UMockAI_BT::IsRunning() const
{
	return BTComp && BTComp->IsRunning() && BTComp->GetRootTree();
}

bool UMockAI_BT::RunBT(UBehaviorTree* BTAsset, EBTExecutionMode::Type RunType)
{
	BBComp->InitializeBlackboard(BTAsset->BlackboardAsset);
	BTComp->CacheBlackboardComponent(BBComp);

	UWorld* World = FAITestHelpers::GetWorld();

	BBComp->RegisterComponentWithWorld(World);
	BTComp->RegisterComponentWithWorld(World);

	return BTComp->StartTree(BTAsset, RunType);
}