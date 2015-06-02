// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "GameplayTasksPrivatePCH.h"
#include "GameplayTasksComponent.h"
#include "GameplayTask.h"
#include "VisualLogger.h"

#define LOCTEXT_NAMESPACE "GameplayTasksComponent"

UGameplayTasksComponent::UGameplayTasksComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.TickGroup = TG_DuringPhysics;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = true;

	bReplicates = true;
}
	
void UGameplayTasksComponent::TaskStarted(UGameplayTask& NewTask)
{
	if (NewTask.IsTickingTask())
	{
		check(TickingTasks.Contains(&NewTask) == false);
		TickingTasks.Add(&NewTask);

		// If this is our first ticking task, set this component as active so it begins ticking
		if (TickingTasks.Num() == 1)
		{
			UpdateShouldTick();
		}

	}
	if (NewTask.IsSimulatedTask())
	{
		check(SimulatedTasks.Contains(&NewTask) == false);
		SimulatedTasks.Add(&NewTask);
	}
}

void UGameplayTasksComponent::TaskEnded(UGameplayTask& Task)
{
	if (Task.IsTickingTask())
	{
		// If we are removing our last ticking task, set this component as inactive so it stops ticking
		TickingTasks.RemoveSingleSwap(&Task);
	}

	if (Task.IsSimulatedTask())
	{
		SimulatedTasks.RemoveSingleSwap(&Task);
	}

	FinishedTasks.Add(&Task);
	RequestTicking();
}

void UGameplayTasksComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	// Intentionally not calling super: We do not want to replicate bActive which controls ticking. We sometimes need to tick on client predictively.
	DOREPLIFETIME_CONDITION(UGameplayTasksComponent, SimulatedTasks, COND_SkipOwner);
}

bool UGameplayTasksComponent::ReplicateSubobjects(UActorChannel* Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	
	if (!RepFlags->bNetOwner)
	{
		for (UGameplayTask* SimulatedTask : SimulatedTasks)
		{
			if (SimulatedTask && !SimulatedTask->HasAnyFlags(RF_PendingKill))
			{
				WroteSomething |= Channel->ReplicateSubobject(SimulatedTask, *Bunch, *RepFlags);
			}
		}
	}

	return WroteSomething;
}

void UGameplayTasksComponent::OnRep_SimulatedTasks()
{
	for (UGameplayTask* SimulatedTask : SimulatedTasks)
	{
		// Temp check 
		if (SimulatedTask && SimulatedTask->IsTickingTask() && TickingTasks.Contains(SimulatedTask) == false)
		{
			SimulatedTask->InitSimulatedTask(*this);
			if (TickingTasks.Num() == 0)
			{
				UpdateShouldTick();
			}

			TickingTasks.Add(SimulatedTask);
		}
	}
}

void UGameplayTasksComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	SCOPE_CYCLE_COUNTER(STAT_TickGameplayTasks);

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// clean up the tasks that have already finished
	for (auto FinishedTask : FinishedTasks)
	{
		ensure(FinishedTask->IsPendingKill() == false);
		FinishedTask->MarkPendingKill();
	}
	FinishedTasks.Reset();

	// Because we have no control over what a task may do when it ticks, we must be careful.
	// Ticking a task may kill the task right here. It could also potentially kill another task
	// which was waiting on the original task to do something. Since when a tasks is killed, it removes
	// itself from the TickingTask list, we will make a copy of the tasks we want to service before ticking any

	int32 NumTickingTasks = TickingTasks.Num();
	int32 NumActuallyTicked = 0;
	switch (NumTickingTasks)
	{
	case 0:
		break;
	case 1:
		if (TickingTasks[0].IsValid())
		{
			TickingTasks[0]->TickTask(DeltaTime);
			NumActuallyTicked++;
		}
		break;
	default:
	{
		TArray<TWeakObjectPtr<UGameplayTask> > LocalTickingTasks = TickingTasks;
		for (TWeakObjectPtr<UGameplayTask>& TickingTask : LocalTickingTasks)
		{
			if (TickingTask.IsValid())
			{
				TickingTask->TickTask(DeltaTime);
				NumActuallyTicked++;
			}
		}
	}
		break;
	};

	// Stop ticking if no more active tasks
	if (NumActuallyTicked == 0)
	{
		TickingTasks.SetNum(0, false);
		UpdateShouldTick();
	}
}

bool UGameplayTasksComponent::GetShouldTick() const
{
	return TickingTasks.Num() > 0 || FinishedTasks.Num() > 0;
}

void UGameplayTasksComponent::RequestTicking()
{
	if (bIsActive == false)
	{
		SetActive(true);
	}
}

void UGameplayTasksComponent::UpdateShouldTick()
{
	const bool bShouldTick = GetShouldTick();	
	if (bIsActive != bShouldTick)
	{
		SetActive(bShouldTick);
	}
}

AActor* UGameplayTasksComponent::GetAvatarActor() const
{
	return GetOwner();
}

#undef LOCTEXT_NAMESPACE