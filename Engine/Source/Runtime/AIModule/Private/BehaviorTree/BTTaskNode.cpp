// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "GameplayTasksComponent.h"
#include "BehaviorTree/BTTaskNode.h"

UBTTaskNode::UBTTaskNode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NodeName = "UnknownTask";
	bNotifyTick = false;
	bNotifyTaskFinished = false;
}

EBTNodeResult::Type UBTTaskNode::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::Succeeded;
}

EBTNodeResult::Type UBTTaskNode::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::Aborted;
}

EBTNodeResult::Type UBTTaskNode::WrappedExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBTNode* NodeOb = bCreateNodeInstance ? GetNodeInstance(OwnerComp, NodeMemory) : this;
	return NodeOb ? ((UBTTaskNode*)NodeOb)->ExecuteTask(OwnerComp, NodeMemory) : EBTNodeResult::Failed;
}

EBTNodeResult::Type UBTTaskNode::WrappedAbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	UBTNode* NodeOb = const_cast<UBTNode*>(bCreateNodeInstance ? GetNodeInstance(OwnerComp, NodeMemory) : this);
	UBTTaskNode* TaskNodeOb = static_cast<UBTTaskNode*>(NodeOb);
	EBTNodeResult::Type Result = TaskNodeOb ? TaskNodeOb->AbortTask(OwnerComp, NodeMemory) : EBTNodeResult::Aborted;

	if (TaskNodeOb && TaskNodeOb->bOwnsGameplayTasks && OwnerComp.GetAIOwner())
	{
		UGameplayTasksComponent* GTComp = OwnerComp.GetAIOwner()->GetGameplayTasksComponent();
		if (GTComp)
		{
			GTComp->EndAllResourceConsumingTasksOwnedBy(*TaskNodeOb);
		}
	}

	return Result;
}

void UBTTaskNode::WrappedTickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) const
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

void UBTTaskNode::WrappedOnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) const
{
	UBTNode* NodeOb = const_cast<UBTNode*>(bCreateNodeInstance ? GetNodeInstance(OwnerComp, NodeMemory) : this);

	if (NodeOb)
	{
		UBTTaskNode* TaskNodeOb = static_cast<UBTTaskNode*>(NodeOb);
		if (TaskNodeOb->bNotifyTaskFinished)
		{
			TaskNodeOb->OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
		}

		if (TaskNodeOb->bOwnsGameplayTasks && OwnerComp.GetAIOwner())
		{
			UGameplayTasksComponent* GTComp = OwnerComp.GetAIOwner()->GetGameplayTasksComponent();
			if (GTComp)
			{
				GTComp->EndAllResourceConsumingTasksOwnedBy(*TaskNodeOb);
			}
		}
	}
}

void UBTTaskNode::ReceivedMessage(UBrainComponent* BrainComp, const FAIMessage& Message)
{
	UBehaviorTreeComponent* OwnerComp = static_cast<UBehaviorTreeComponent*>(BrainComp);
	check(OwnerComp);
	
	const uint16 InstanceIdx = OwnerComp->FindInstanceContainingNode(this);
	if (OwnerComp->InstanceStack.IsValidIndex(InstanceIdx))
	{
		uint8* NodeMemory = GetNodeMemory<uint8>(OwnerComp->InstanceStack[InstanceIdx]);
		OnMessage(*OwnerComp, NodeMemory, Message.MessageName, Message.RequestID, Message.Status == FAIMessage::Success);
	}
	else
	{
		UE_VLOG(OwnerComp->GetOwner(), LogBehaviorTree, Warning, TEXT("UBTTaskNode::ReceivedMessage called while %s node no longer in active BT")
			, *GetNodeName());
	}
}

void UBTTaskNode::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	// empty in base class
}

void UBTTaskNode::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	// empty in base class
}

void UBTTaskNode::OnMessage(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, FName Message, int32 RequestID, bool bSuccess)
{
	const EBTTaskStatus::Type Status = OwnerComp.GetTaskStatus(this);
	if (Status == EBTTaskStatus::Active)
	{
		FinishLatentTask(OwnerComp, bSuccess ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
	}
	else if (Status == EBTTaskStatus::Aborting)
	{
		FinishLatentAbort(OwnerComp);
	}
}

void UBTTaskNode::FinishLatentTask(UBehaviorTreeComponent& OwnerComp, EBTNodeResult::Type TaskResult) const
{
	// OnTaskFinished must receive valid template node
	UBTTaskNode* TemplateNode = (UBTTaskNode*)OwnerComp.FindTemplateNode(this);
	OwnerComp.OnTaskFinished(TemplateNode, TaskResult);
}

void UBTTaskNode::FinishLatentAbort(UBehaviorTreeComponent& OwnerComp) const
{
	// OnTaskFinished must receive valid template node
	UBTTaskNode* TemplateNode = (UBTTaskNode*)OwnerComp.FindTemplateNode(this);
	OwnerComp.OnTaskFinished(TemplateNode, EBTNodeResult::Aborted);
}

void UBTTaskNode::WaitForMessage(UBehaviorTreeComponent& OwnerComp, FName MessageType) const
{
	// messages delegates should be called on node instances (if they exists)
	OwnerComp.RegisterMessageObserver(this, MessageType);
}

void UBTTaskNode::WaitForMessage(UBehaviorTreeComponent& OwnerComp, FName MessageType, int32 RequestID) const
{
	// messages delegates should be called on node instances (if they exists)
	OwnerComp.RegisterMessageObserver(this, MessageType, RequestID);
}
	
void UBTTaskNode::StopWaitingForMessages(UBehaviorTreeComponent& OwnerComp) const
{
	// messages delegates should be called on node instances (if they exists)
	OwnerComp.UnregisterMessageObserversFrom(this);
}

#if WITH_EDITOR

FName UBTTaskNode::GetNodeIconName() const
{
	return FName("BTEditor.Graph.BTNode.Task.Icon");
}

#endif	// WITH_EDITOR

//----------------------------------------------------------------------//
// UBTTaskNode IGameplayTaskOwnerInterface
//----------------------------------------------------------------------//
UGameplayTasksComponent* UBTTaskNode::GetGameplayTasksComponent(const UGameplayTask& Task) const 
{
	const UAITask* AsAITask = Cast<const UAITask>(&Task);
	if (AsAITask)
	{
		return AsAITask->GetAIController() ? AsAITask->GetAIController()->GetGameplayTasksComponent(Task) : nullptr;
	}

	return Task.GetGameplayTasksComponent();
}

void UBTTaskNode::OnTaskInitialized(UGameplayTask& Task)
{
	// validate the task
	UAITask* AsAITask = Cast<UAITask>(&Task);
	if (AsAITask != nullptr && AsAITask->GetAIController() == nullptr)
	{
		// this means that the task has either been created without specifying 
		// UAITAsk::OwnerController's value (like via BP's Construct Object node)
		// or it has been created in C++ with inappropriate function
		UE_LOG(LogBehaviorTree, Error, TEXT("Missing AIController in AITask %s"), *AsAITask->GetName());
	}
}

UBehaviorTreeComponent* UBTTaskNode::GetBTComponentForTask(UGameplayTask& Task) const
{
	UAITask* AsAITask = Cast<UAITask>(&Task);
	return AsAITask && AsAITask->GetAIController() ? Cast<UBehaviorTreeComponent>(AsAITask->GetAIController()->BrainComponent) : nullptr;
}

void UBTTaskNode::OnTaskActivated(UGameplayTask& Task) 
{
	ensure(Task.GetTaskOwner() == this);
}

void UBTTaskNode::OnTaskDeactivated(UGameplayTask& Task)
{
	ensure(Task.GetTaskOwner() == this);
	UBehaviorTreeComponent* BTComp = GetBTComponentForTask(Task);
	if (BTComp)
	{
		// this is a super-default behavior. Specific task will surely like to 
		// handle this themselves, finishing with specific result
		FinishLatentTask(*BTComp, EBTNodeResult::Succeeded);
	}
}

AActor* UBTTaskNode::GetOwnerActor(const UGameplayTask* Task) const
{
	if (Task == nullptr)
	{
		if (IsInstanced())
		{
			const UBehaviorTreeComponent* BTComponent = Cast<const UBehaviorTreeComponent>(GetOuter());
			//not having BT component for an instanced BT node is invalid!
			check(BTComponent);
			return BTComponent->GetAIOwner();
		}
		else
		{
			UE_LOG(LogBehaviorTree, Warning, TEXT("%s: Unable to determine Owner Actor for a null GameplayTask"), *GetName());
			return nullptr;
		}
	}

	const UAITask* AsAITask = Cast<const UAITask>(Task);
	if (AsAITask)
	{
		return AsAITask->GetAIController();
	}

	const UGameplayTasksComponent* GTComponent = Task->GetGameplayTasksComponent();

	return GTComponent ? GTComponent->GetOwnerActor(Task) : nullptr; 
}

AActor* UBTTaskNode::GetAvatarActor(const UGameplayTask* Task) const
{
	if (Task == nullptr)
	{
		if (IsInstanced())
		{
			const UBehaviorTreeComponent* BTComponent = Cast<const UBehaviorTreeComponent>(GetOuter());
			//not having BT component for an instanced BT node is invalid!
			check(BTComponent); 
			return BTComponent->GetAIOwner() ? BTComponent->GetAIOwner()->GetPawn() : nullptr;
		}
		else
		{
			UE_LOG(LogBehaviorTree, Warning, TEXT("%s: Unable to determine Avatar Actor for a null GameplayTask"), *GetName());
			return nullptr;
		}
	}

	const UAITask* AsAITask = Cast<const UAITask>(Task);
	if (AsAITask)
	{
		return AsAITask->GetAIController() ? AsAITask->GetAIController()->GetPawn() : nullptr;
	}

	const UGameplayTasksComponent* GTComponent = Task->GetGameplayTasksComponent();

	return GTComponent ? GTComponent->GetOwnerActor(Task) : nullptr;
}

uint8 UBTTaskNode::GetDefaultPriority() const 
{ 
	return uint8(EAITaskPriority::AutonomousAI); 
}

//----------------------------------------------------------------------//
// DEPRECATED
//----------------------------------------------------------------------//
EBTNodeResult::Type UBTTaskNode::WrappedExecuteTask(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	return OwnerComp ? WrappedExecuteTask(*OwnerComp, NodeMemory) : EBTNodeResult::Failed;
}
EBTNodeResult::Type UBTTaskNode::WrappedAbortTask(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	return OwnerComp ? WrappedAbortTask(*OwnerComp, NodeMemory) : EBTNodeResult::Failed;
}
void UBTTaskNode::WrappedTickTask(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) const
{
	if (OwnerComp)
	{
		WrappedTickTask(*OwnerComp, NodeMemory, DeltaSeconds);
	}
}
void UBTTaskNode::WrappedOnTaskFinished(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) const
{
	if (OwnerComp)
	{
		WrappedOnTaskFinished(*OwnerComp, NodeMemory, TaskResult);
	}
}
void UBTTaskNode::FinishLatentTask(UBehaviorTreeComponent* OwnerComp, EBTNodeResult::Type TaskResult) const
{
	if (OwnerComp)
	{
		FinishLatentTask(*OwnerComp, TaskResult);
	}
}
void UBTTaskNode::FinishLatentAbort(UBehaviorTreeComponent* OwnerComp) const
{
	if (OwnerComp)
	{
		FinishLatentAbort(*OwnerComp);
	}
}
EBTNodeResult::Type UBTTaskNode::ExecuteTask(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory)
{
	if (OwnerComp)
	{
		return ExecuteTask(*OwnerComp, NodeMemory);
	}
	return EBTNodeResult::Failed;
}
EBTNodeResult::Type UBTTaskNode::AbortTask(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory)
{
	if (OwnerComp)
	{
		return AbortTask(*OwnerComp, NodeMemory);
	}
	return EBTNodeResult::Failed;
}
void UBTTaskNode::TickTask(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	if (OwnerComp)
	{
		TickTask(*OwnerComp, NodeMemory, DeltaSeconds);
	}
}
void UBTTaskNode::OnMessage(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, FName Message, int32 RequestID, bool bSuccess)
{
	if (OwnerComp)
	{
		OnMessage(*OwnerComp, NodeMemory, Message, RequestID, bSuccess);
	}
}
void UBTTaskNode::OnTaskFinished(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	if (OwnerComp)
	{
		OnTaskFinished(*OwnerComp, NodeMemory, TaskResult);
	}
}
void UBTTaskNode::WaitForMessage(UBehaviorTreeComponent* OwnerComp, FName MessageType) const
{
	if (OwnerComp)
	{
		WaitForMessage(*OwnerComp, MessageType);
	}
}
void UBTTaskNode::WaitForMessage(UBehaviorTreeComponent* OwnerComp, FName MessageType, int32 RequestID) const
{
	if (OwnerComp)
	{
		WaitForMessage(*OwnerComp, MessageType, RequestID);
	}
}