// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BTTaskNode.generated.h"

UCLASS(Abstract)
class ENGINE_API UBTTaskNode : public UBTNode
{
	GENERATED_UCLASS_BODY()

	/** starts this task, should return Succeeded, Failed or InProgress
	 *  (use FinishLatentTask() when returning InProgress) */
	virtual EBTNodeResult::Type ExecuteTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const;

	/** aborts this task, should return Aborted or InProgress
	 *  (use FinishLatentAbort() when returning InProgress) */
	virtual EBTNodeResult::Type AbortTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const;

	/** ticks this task, when bTickEnabled is set */
	void ConditionalTickTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) const;

	/** called on search start */
	void ConditionalSearchStart(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const;

	/** message observer's hook */
	void ReceivedMessage(UBrainComponent* BrainComp, const struct FAIMessage& Message) const;

protected:

	/** if set, TickTask will be called */
	uint8 bNotifyTick : 1;

	/** if set, OnSearchStart will be called */
	uint8 bNotifySearchStart : 1;

	/** ticks this task */
	virtual void TickTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) const;

	/** message handler, default implementation will finish latent execution/abortion */
	virtual void OnMessage(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, FName Message, int32 RequestID, bool bSuccess) const;

	/** called on search start with searches memory copy
	  * it's responsible for updating any flow control mechanics of this task (e.g. internal semaphores)
	  *
	  * DO NOT UPDATE WORLD STATE HERE! (search can be discarded)
	  */
	virtual void OnSearchStart(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const;

	/** helper function: finish latent executing */
	void FinishLatentTask(class UBehaviorTreeComponent* OwnerComp, EBTNodeResult::Type TaskResult) const;

	/** helper function: finishes latent aborting */
	void FinishLatentAbort(class UBehaviorTreeComponent* OwnerComp) const;

	/** register message observer */
	void WaitForMessage(class UBehaviorTreeComponent* OwnerComp, FName MessageType) const;
	void WaitForMessage(class UBehaviorTreeComponent* OwnerComp, FName MessageType, int32 RequestID) const;
	
	/** unregister message observers */
	void StopWaitingForMessages(class UBehaviorTreeComponent* OwnerComp) const;
};
