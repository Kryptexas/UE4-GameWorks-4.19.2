// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "MockAI.h"
#include "MockAI_BT.generated.h"

UCLASS()
class UMockAI_BT : public UMockAI
{
	GENERATED_BODY()
public:
	UMockAI_BT(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY()
	UBehaviorTreeComponent* BTComp;

	static TArray<int32> ExecutionLog;
	TArray<int32> ExpectedResult;

	bool IsRunning() const;
	void RunBT(UBehaviorTree& BTAsset, EBTExecutionMode::Type RunType = EBTExecutionMode::SingleRun);
};
