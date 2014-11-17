// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBTTaskNode::UBTTaskNode(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = "UnknownTask";
	bNotifyTick = false;
}

EBTNodeResult::Type UBTTaskNode::ExecuteTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::Succeeded;
}

EBTNodeResult::Type UBTTaskNode::AbortTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::Aborted;
}

EBTNodeResult::Type UBTTaskNode::WrappedExecuteTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	const UBTNode* NodeOb = bCreateNodeInstance ? GetNodeInstance(OwnerComp, NodeMemory) : this;
	return NodeOb ? ((UBTTaskNode*)NodeOb)->ExecuteTask(OwnerComp, NodeMemory) : EBTNodeResult::Failed;
}

EBTNodeResult::Type UBTTaskNode::WrappedAbortTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	const UBTNode* NodeOb = bCreateNodeInstance ? GetNodeInstance(OwnerComp, NodeMemory) : this;
	return NodeOb ? ((UBTTaskNode*)NodeOb)->AbortTask(OwnerComp, NodeMemory) : EBTNodeResult::Aborted;
}

void UBTTaskNode::WrappedTickTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) const
{
	if (bNotifyTick)
	{
		const UBTNode* NodeOb = bCreateNodeInstance ? GetNodeInstance(OwnerComp, NodeMemory) : this;
		if (NodeOb)
		{
			((UBTTaskNode*)NodeOb)->TickTask(OwnerComp, NodeMemory, DeltaSeconds);
		}
	}
}

void UBTTaskNode::ReceivedMessage(UBrainComponent* BrainComp, const struct FAIMessage& Message)
{
	UBehaviorTreeComponent* OwnerComp = (UBehaviorTreeComponent*)BrainComp;
	uint16 InstanceIdx = OwnerComp->FindInstanceContainingNode(this);
	uint8* NodeMemory = GetNodeMemory<uint8>(OwnerComp->InstanceStack[InstanceIdx]);

	OnMessage(OwnerComp, NodeMemory, Message.Message, Message.MessageID, Message.Status == FAIMessage::Success);
}

void UBTTaskNode::TickTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	// empty in base class
}

void UBTTaskNode::OnMessage(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, FName Message, int32 RequestID, bool bSuccess)
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

void UBTTaskNode::FinishLatentTask(class UBehaviorTreeComponent* OwnerComp, EBTNodeResult::Type TaskResult) const
{
	// OnTaskFinished must receive valid template node
	UBTTaskNode* TemplateNode = (UBTTaskNode*)OwnerComp->FindTemplateNode(this);
	OwnerComp->OnTaskFinished(TemplateNode, TaskResult);
}

void UBTTaskNode::FinishLatentAbort(class UBehaviorTreeComponent* OwnerComp) const
{
	// OnTaskFinished must receive valid template node
	UBTTaskNode* TemplateNode = (UBTTaskNode*)OwnerComp->FindTemplateNode(this);
	OwnerComp->OnTaskFinished(TemplateNode, EBTNodeResult::Aborted);
}

void UBTTaskNode::WaitForMessage(class UBehaviorTreeComponent* OwnerComp, FName MessageType) const
{
	// messages delegates should be called on node instances (if they exists)
	OwnerComp->RegisterMessageObserver(this, MessageType);
}

void UBTTaskNode::WaitForMessage(class UBehaviorTreeComponent* OwnerComp, FName MessageType, int32 RequestID) const
{
	// messages delegates should be called on node instances (if they exists)
	OwnerComp->RegisterMessageObserver(this, MessageType, RequestID);
}
	
void UBTTaskNode::StopWaitingForMessages(class UBehaviorTreeComponent* OwnerComp) const
{
	// messages delegates should be called on node instances (if they exists)
	OwnerComp->UnregisterMessageObserversFrom(this);
}
