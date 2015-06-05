// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "GameplayTasksPrivatePCH.h"
#include "GameplayTasksComponent.h"
#include "GameplayTask.h"

UGameplayTask::UGameplayTask(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bTickingTask = false;
	bSimulatedTask = false;
	bIsSimulating = false;
	bOwnedByTasksComponent = false;
	TaskState = EGameplayTaskState::Uninitialized;
	SetFlags(RF_StrongRefOnFrame);
}

IGameplayTaskOwnerInterface* UGameplayTask::ConvertToTaskOwner(UObject& OwnerObject)
{
	IGameplayTaskOwnerInterface* OwnerInterface = Cast<IGameplayTaskOwnerInterface>(&OwnerObject);

	if (OwnerInterface == nullptr)
	{
		AActor* AsActor = Cast<AActor>(&OwnerObject);
		if (AsActor)
		{
			OwnerInterface = AsActor->FindComponentByClass<UGameplayTasksComponent>();
		}
	}
	return OwnerInterface;
}

IGameplayTaskOwnerInterface* UGameplayTask::ConvertToTaskOwner(AActor& OwnerActor)
{
	IGameplayTaskOwnerInterface* OwnerInterface = Cast<IGameplayTaskOwnerInterface>(&OwnerActor);

	if (OwnerInterface == nullptr)
	{
		OwnerInterface = OwnerActor.FindComponentByClass<UGameplayTasksComponent>();
	}
	return OwnerInterface;
}

void UGameplayTask::ReadyForActivation()
{
	// default case, we instantly activate the task
	TaskState = EGameplayTaskState::Active;

	if (TaskOwner.IsValid())
	{
		TaskOwner->TaskStarted(*this);

		// If this task requires ticking, register it with TasksComponent
		if (bOwnedByTasksComponent == false && TasksComponent.IsValid())
		{
			TasksComponent->TaskStarted(*this);
		}
		// else: it's not supposed to work this way!

		Activate();
	}
	else
	{
		EndTask();
	}
}

void UGameplayTask::InitTask(IGameplayTaskOwnerInterface& InTaskOwner)
{
	TaskOwner = InTaskOwner;
	UGameplayTasksComponent* GTComponent = InTaskOwner.GetGameplayTasksComponent();
	TasksComponent = GTComponent;

	bOwnedByTasksComponent = (TaskOwner == GTComponent);

	TaskState = EGameplayTaskState::AwaitingActivation;

	InTaskOwner.OnTaskInitialized(*this);
	if (bOwnedByTasksComponent == false && GTComponent != nullptr)
	{
		GTComponent->OnTaskInitialized(*this);
	}
}

void UGameplayTask::InitSimulatedTask(UGameplayTasksComponent& InGameplayTasksComponent)
{
	TasksComponent = &InGameplayTasksComponent;
	bIsSimulating = true;
}

UWorld* UGameplayTask::GetWorld() const
{
	if (TasksComponent.IsValid())
	{
		return TasksComponent.Get()->GetWorld();
	}

	return nullptr;
}

AActor* UGameplayTask::GetOwnerActor() const
{
	if (TaskOwner.IsValid())
	{
		return TaskOwner->GetOwnerActor();		
	}
	else if (TasksComponent.IsValid())
	{
		return TasksComponent->GetOwner();
	}

	return nullptr;
}

AActor* UGameplayTask::GetAvatarActor() const
{
	if (TaskOwner.IsValid())
	{
		return TaskOwner->GetAvatarActor();
	}
	else if (TasksComponent.IsValid())
	{
		return TasksComponent->GetAvatarActor();
	}

	return nullptr;
}

void UGameplayTask::TaskOwnerEnded()
{
	if (TaskState != EGameplayTaskState::Finished && !IsPendingKill())
	{
		OnDestroy(true);
	}
}

void UGameplayTask::EndTask()
{
	if (TaskState != EGameplayTaskState::Finished && !IsPendingKill())
	{
		OnDestroy(false);
	}
}

void UGameplayTask::ExternalConfirm(bool bEndTask)
{
	if (bEndTask)
	{
		EndTask();
	}
}

void UGameplayTask::ExternalCancel()
{
	EndTask();
}

void UGameplayTask::OnDestroy(bool bOwnerFinished)
{
	ensure(TaskState != EGameplayTaskState::Finished && !IsPendingKill());

	TaskState = EGameplayTaskState::Finished;

	bool bMarkAsPendingKill = true;
	// If this task required ticking, unregister it with TasksComponent
	if (TasksComponent.IsValid())
	{
		TasksComponent->TaskEnded(*this);
		// we need to mark task as pending kill only if there's no component that can 
		// do that for us, latently
		bMarkAsPendingKill = false;
	}

	// Remove ourselves from the owner's task list, if the ability isn't ending
	if (bOwnedByTasksComponent == false && bOwnerFinished == false && TaskOwner.IsValid() == true)
	{
		TaskOwner->TaskEnded(*this);
	}

	if (bMarkAsPendingKill)
	{
		// this usually means the driven actor and/or the component are already gone
		MarkPendingKill();
	}
}

FString UGameplayTask::GetDebugString() const
{
	return FString::Printf(TEXT("Generic %s"), *GetName());
}
