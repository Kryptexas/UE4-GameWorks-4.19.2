// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBTTaskNode::UBTTaskNode(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = "UnknownTask";
	bNotifyTick = false;
	bNotifySearchStart = false;
}

EBTNodeResult::Type UBTTaskNode::ExecuteTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	return EBTNodeResult::Succeeded;
}

EBTNodeResult::Type UBTTaskNode::AbortTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	return EBTNodeResult::Aborted;
}

void UBTTaskNode::ConditionalTickTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) const
{
	if (bNotifyTick)
	{
		TickTask(OwnerComp, NodeMemory, DeltaSeconds);
	}
}

void UBTTaskNode::ConditionalSearchStart(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	if (bNotifySearchStart)
	{
		OnSearchStart(OwnerComp, NodeMemory);
	}
}

void UBTTaskNode::ReceivedMessage(UBrainComponent* BrainComp, const struct FAIMessage& Message) const
{
	UBehaviorTreeComponent* OwnerComp = (UBehaviorTreeComponent*)BrainComp;
	uint16 InstanceIdx = OwnerComp->FindInstanceContainingNode(this);
	uint8* NodeMemory = GetNodeMemory<uint8>(OwnerComp->InstanceStack[InstanceIdx]);

	OnMessage(OwnerComp, NodeMemory, Message.Message, Message.MessageID, Message.Status == FAIMessage::Success);
}

void UBTTaskNode::TickTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) const
{
	// empty in base class
}

void UBTTaskNode::OnMessage(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, FName Message, int32 RequestID, bool bSuccess) const
{
	const EBTTaskStatus::Type Status = OwnerComp->GetTaskStatus(this);
	if (Status == EBTTaskStatus::Active)
	{
		FinishLatentTask(OwnerComp, bSuccess ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
	}
	else if (Status == EBTTaskStatus::Aborting)
	{
		FinishLatentAbort(OwnerComp);
	}
}

void UBTTaskNode::OnSearchStart(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	// empty in base class
}

void UBTTaskNode::FinishLatentTask(class UBehaviorTreeComponent* OwnerComp, EBTNodeResult::Type TaskResult) const
{
	OwnerComp->OnTaskFinished(this, TaskResult);
}

void UBTTaskNode::FinishLatentAbort(class UBehaviorTreeComponent* OwnerComp) const
{
	OwnerComp->OnTaskFinished(this, EBTNodeResult::Aborted);
}

void UBTTaskNode::WaitForMessage(class UBehaviorTreeComponent* OwnerComp, FName MessageType) const
{
	OwnerComp->RegisterMessageObserver(this, MessageType);
}

void UBTTaskNode::WaitForMessage(class UBehaviorTreeComponent* OwnerComp, FName MessageType, int32 RequestID) const
{
	OwnerComp->RegisterMessageObserver(this, MessageType, RequestID);
}
	
void UBTTaskNode::StopWaitingForMessages(class UBehaviorTreeComponent* OwnerComp) const
{
	OwnerComp->UnregisterMessageObserversFrom(this);
}
