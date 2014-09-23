// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Tickable.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "TestBTObject.generated.h"

struct FTestTickHelper : FTickableGameObject
{
	TWeakObjectPtr<class UTestBTObject> Owner;

	FTestTickHelper() : Owner(NULL) {}
	virtual void Tick(float DeltaTime);
	virtual bool IsTickable() const { return Owner.IsValid(); }
	virtual bool IsTickableInEditor() const { return true; }
	virtual TStatId GetStatId() const;
};

UCLASS()
class UTestBTObject : public UObject
{
	GENERATED_UCLASS_BODY()

	UBehaviorTreeComponent* BTComp;
	UBlackboardComponent* BBComp;
	FTestTickHelper TickHelper;
	
	static TArray<int32> ExecutionLog;
	TArray<int32> ExpectedResult;

	void TickMe(float DeltaTime);
	bool RunTest(UBehaviorTree* TreeOb);
	bool IsRunning() const;

	void VerifyResults();
};
