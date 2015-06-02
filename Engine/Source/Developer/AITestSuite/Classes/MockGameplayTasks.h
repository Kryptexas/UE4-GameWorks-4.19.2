// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "GameplayTask.h"
#include "MockGameplayTasks.generated.h"

namespace ETestTaskMessage
{
	enum Type
	{
		Activate,
		Tick,
		ExternalConfirm,
		ExternalCancel,
		Ended
	};
}

UCLASS()
class UMockTask_Log : public UGameplayTask
{
	GENERATED_BODY()

protected:
	FTestLogger<int32>* Logger;

public:
	UMockTask_Log(const FObjectInitializer& ObjectInitializer);

	static UMockTask_Log* CreateTask(IGameplayTaskOwnerInterface& TaskOwner, FTestLogger<int32>& InLogger);

protected:
	virtual void Activate() override;
	virtual void OnDestroy(bool bOwnerFinished) override;

public:
	virtual void TickTask(float DeltaTime) override;
	virtual void ExternalConfirm(bool bEndTask) override;
	virtual void ExternalCancel() override;

	// testing only hack-functions
	void EnableTick() { bTickingTask = true; }
};
