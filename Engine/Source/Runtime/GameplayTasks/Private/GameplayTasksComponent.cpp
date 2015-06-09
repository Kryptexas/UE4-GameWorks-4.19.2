// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "GameplayTasksPrivatePCH.h"
#include "GameplayTasksComponent.h"
#include "GameplayTask.h"

#define LOCTEXT_NAMESPACE "GameplayTasksComponent"

UGameplayTasksComponent::UGameplayTasksComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.TickGroup = TG_DuringPhysics;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = true;

	bReplicates = true;
	TopActivePriority = 0;
}
	
void UGameplayTasksComponent::OnTaskActivated(UGameplayTask& Task)
{
	if (Task.IsTickingTask())
	{
		check(TickingTasks.Contains(&Task) == false);
		TickingTasks.Add(&Task);

		// If this is our first ticking task, set this component as active so it begins ticking
		if (TickingTasks.Num() == 1)
		{
			UpdateShouldTick();
		}
	}
	if (Task.IsSimulatedTask())
	{
		check(SimulatedTasks.Contains(&Task) == false);
		SimulatedTasks.Add(&Task);
	}
}

void UGameplayTasksComponent::OnTaskDeactivated(UGameplayTask& Task)
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

	UpdateShouldTick();

	// Resource-using task
	if (Task.RequiresPriorityOrResourceManagement() == true && Task.GetState() == EGameplayTaskState::Finished)
	{
		OnTaskEnded(Task);
	}
}

void UGameplayTasksComponent::OnTaskEnded(UGameplayTask& Task)
{
	ensure(Task.RequiresPriorityOrResourceManagement() == true);

	RemoveTaskFromPriorityQueue(Task);
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
	return TickingTasks.Num() > 0;
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

//----------------------------------------------------------------------//
// Priority and resources handling
//----------------------------------------------------------------------//
void UGameplayTasksComponent::AddTaskReadyForActivation(UGameplayTask& NewTask)
{
	UE_VLOG(this, LogGameplayTasks, Log, TEXT("RemoveResourceConsumingTask %s"), *NewTask.GetName());

	ensure(NewTask.RequiresPriorityOrResourceManagement() == true);
	
	TaskEvents.Add(FGameplayTaskEventData(EGameplayTaskEvent::Add, NewTask));
	// trigger the actual processing only if it was the first event added to the list
	if (TaskEvents.Num() == 1)
	{
		ProcessTaskEvents();
	}
}

void UGameplayTasksComponent::RemoveResourceConsumingTask(UGameplayTask& Task)
{
	UE_VLOG(this, LogGameplayTasks, Log, TEXT("RemoveResourceConsumingTask %s"), *Task.GetName());

	TaskEvents.Add(FGameplayTaskEventData(EGameplayTaskEvent::Remove, Task));
	// trigger the actual processing only if it was the first event added to the list
	if (TaskEvents.Num() == 1)
	{
		ProcessTaskEvents();
	}
}

void UGameplayTasksComponent::ProcessTaskEvents()
{
	static const int32 ErronousIterationsLimit = 100;

	// note that this function allows called functions to add new elements to 
	// TaskEvents array that the main loop is iterating over. It's a feature
	for (int32 EventIndex = 0; EventIndex < TaskEvents.Num(); ++EventIndex)
	{
		if (TaskEvents[EventIndex].RelatedTask.IsPendingKill())
		{
			// we should ignore it, but just in case run the removal code.
			RemoveTaskFromPriorityQueue(TaskEvents[EventIndex].RelatedTask);
			continue;
		}

		switch (TaskEvents[EventIndex].Event)
		{
		case EGameplayTaskEvent::Add:
			AddTaskToPriorityQueue(TaskEvents[EventIndex].RelatedTask);
			break;
		case EGameplayTaskEvent::Remove:
			RemoveTaskFromPriorityQueue(TaskEvents[EventIndex].RelatedTask);
			break;
		default:
			checkNoEntry();
			break;
		}

		if (EventIndex >= ErronousIterationsLimit)
		{
			UE_VLOG(this, LogGameplayTasks, Error, TEXT("UGameplayTasksComponent::ProcessTaskEvents has exceeded warning-level of iterations. Check your GameplayTasks for logic loops!"));
		}
	}

	TaskEvents.Reset();
}

void UGameplayTasksComponent::AddTaskToPriorityQueue(UGameplayTask& NewTask)
{
	// The generic idea is as follows:
	// 1. Find insertion/removal point X
	//		While looking for it add up all ResourcesUsedByActiveUpToX and ResourcesRequiredUpToX
	//		ResourcesUsedByActiveUpToX - resources required by ACTIVE tasks
	//		ResourcesRequiredUpToX - resources required by both active and inactive tasks on the way to X
	// 2. Insert Task at X
	// 3. Starting from X proceed down the queue
	//	a. If ConsideredTask.Resources overlaps with ResourcesRequiredUpToX then PAUSE it
	//	b. Else, ACTIVATE ConsideredTask and add ConsideredTask.Resources to ResourcesUsedByActiveUpToX
	//	c. Add ConsideredTask.Resources to ResourcesRequiredUpToX
	// 4. Set this->CurrentlyUsedResources to ResourcesUsedByActiveUpToX
	//
	// Points 3-4 are implemented as a separate function, UpdateTaskActivationFromIndex,
	// since it's a common code between adding and removing tasks
	
	const bool bStartOnTopOfSamePriority = NewTask.GetResourceOverlapPolicy() == ETaskResourceOverlapPolicy::StartOnTop;

	FGameplayResourceSet ResourcesUsedByActiveUpToX;
	FGameplayResourceSet ResourcesRequiredUpToX;
	int32 InsertionPoint = INDEX_NONE;
	
	if (TaskPriorityQueue.Num() > 0)
	{
		for (int32 TaskIndex = 0; TaskIndex < TaskPriorityQueue.Num(); ++TaskIndex)
		{
			ensure(TaskPriorityQueue[TaskIndex]);
			if (TaskPriorityQueue[TaskIndex] == nullptr)
			{
				continue;
			}

			if ((bStartOnTopOfSamePriority && TaskPriorityQueue[TaskIndex]->GetPriority() <= NewTask.GetPriority())
				|| (!bStartOnTopOfSamePriority && TaskPriorityQueue[TaskIndex]->GetPriority() < NewTask.GetPriority()))
			{
				TaskPriorityQueue.Insert(&NewTask, TaskIndex);
				InsertionPoint = TaskIndex;
				break;
			}

			const FGameplayResourceSet TasksResources = TaskPriorityQueue[TaskIndex]->GetRequiredResources();
			if (TaskPriorityQueue[TaskIndex]->IsActive())
			{
				ResourcesUsedByActiveUpToX.AddSet(TasksResources);
			}
			ResourcesRequiredUpToX.AddSet(TasksResources);
		}
	}
	
	if (InsertionPoint == INDEX_NONE)
	{
		TaskPriorityQueue.Add(&NewTask);
		InsertionPoint = TaskPriorityQueue.Num() - 1;
	}

	UpdateTaskActivationFromIndex(InsertionPoint, ResourcesUsedByActiveUpToX, ResourcesRequiredUpToX);
}

void UGameplayTasksComponent::RemoveTaskFromPriorityQueue(UGameplayTask& Task)
{	
	if (TaskPriorityQueue.Num() > 1)
	{
		const int32 RemovedTaskIndex = TaskPriorityQueue.Find(&Task);
		ensure(RemovedTaskIndex != INDEX_NONE);

		// sum up resources up to TaskIndex
		FGameplayResourceSet ResourcesUsedByActiveUpToX;
		FGameplayResourceSet ResourcesRequiredUpToX;
		for (int32 Index = 0; Index < RemovedTaskIndex; ++Index)
		{
			ensure(TaskPriorityQueue[Index]);
			if (TaskPriorityQueue[Index] == nullptr)
			{
				continue;
			}
			
			const FGameplayResourceSet TasksResources = TaskPriorityQueue[Index]->GetRequiredResources();
			if (TaskPriorityQueue[Index]->IsActive())
			{
				ResourcesUsedByActiveUpToX.AddSet(TasksResources);
			}
			ResourcesRequiredUpToX.AddSet(TasksResources);
		}

		// don't forget to actually remove the task from the queue
		TaskPriorityQueue.RemoveAt(RemovedTaskIndex, 1, /*bAllowShrinking=*/false);

		// if it wasn't the last item then proceed as usual
		if (RemovedTaskIndex < TaskPriorityQueue.Num())
		{
			UpdateTaskActivationFromIndex(RemovedTaskIndex, ResourcesUsedByActiveUpToX, ResourcesRequiredUpToX);
		}
		else
		{
			// no need to do extra processing. This was the last task, so 
			// ResourcesUsedByActiveUpToX is the CurrentlyUsedResources
			CurrentlyUsedResources = ResourcesUsedByActiveUpToX;
		}
	}
	else
	{
		TaskPriorityQueue.Pop(/*bAllowShrinking=*/false);
		CurrentlyUsedResources.Clear();
	}
}

void UGameplayTasksComponent::UpdateTaskActivationFromIndex(int32 StartingIndex, FGameplayResourceSet ResourcesUsedByActiveUpToIndex, FGameplayResourceSet ResourcesRequiredUpToIndex)
{
	check(TaskPriorityQueue.IsValidIndex(StartingIndex));

	TArray<UGameplayTask*> TaskPriorityQueueCopy = TaskPriorityQueue;
	for (int32 TaskIndex = StartingIndex; TaskIndex < TaskPriorityQueueCopy.Num(); ++TaskIndex)
	{
		ensure(TaskPriorityQueueCopy[TaskIndex]);
		if (TaskPriorityQueueCopy[TaskIndex] == nullptr)
		{
			continue;
		}

		const FGameplayResourceSet TasksResources = TaskPriorityQueueCopy[TaskIndex]->GetRequiredResources();
		if (TasksResources.GetOverlap(ResourcesUsedByActiveUpToIndex).IsEmpty())
		{
			TaskPriorityQueueCopy[TaskIndex]->ActivateInTaskQueue();
			ResourcesUsedByActiveUpToIndex.AddSet(TasksResources);
		}
		else
		{
			TaskPriorityQueueCopy[TaskIndex]->PauseInTaskQueue();
		}
		ResourcesRequiredUpToIndex.AddSet(TasksResources);
	}

	CurrentlyUsedResources = ResourcesUsedByActiveUpToIndex;
}

//----------------------------------------------------------------------//
// debugging
//----------------------------------------------------------------------//
#if ENABLE_VISUAL_LOG
void UGameplayTasksComponent::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	static const FString CategoryName = TEXT("GameplayTasks");
	static const FString TickingTasksName = TEXT("Ticking tasks");
	static const FString PriorityQueueName = TEXT("Priority Queue");

	if (IsPendingKill())
	{
		return;
	}

	FVisualLogStatusCategory StatusCategory(CategoryName);

	FString TasksDescription;
	for (auto& Task : TickingTasks)
	{
		if (Task.IsValid())
		{
			TasksDescription += FString::Printf(TEXT("%s %s\n"), *GetTaskStateName(Task->GetState()), *Task->GetDebugDescription());
		}
		else
		{
			TasksDescription += TEXT("NULL\n");
		}
	}
	StatusCategory.Add(TickingTasksName, TasksDescription);

	TasksDescription.Reset();
	for (auto Task : TaskPriorityQueue)
	{
		if (Task != nullptr)
		{
			TasksDescription += FString::Printf(TEXT("%s %s\n"), *GetTaskStateName(Task->GetState()), *Task->GetDebugDescription());
		}
		else
		{
			TasksDescription += TEXT("NULL\n");
		}
	}
	StatusCategory.Add(PriorityQueueName, TasksDescription);

	Snapshot->Status.Add(StatusCategory);
}

FString UGameplayTasksComponent::GetTaskStateName(EGameplayTaskState Value)
{
	static const UEnum* Enum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EGameplayTaskState"));
	check(Enum);
	return Enum->GetEnumName(int32(Value));
}
#endif // ENABLE_VISUAL_LOG

//----------------------------------------------------------------------//
// FGameplayResourceSet
//----------------------------------------------------------------------//
FString FGameplayResourceSet::GetDebugDescription() const
{
	static const int32 FlagsCount = sizeof(FFlagContainer)* 8;
	TCHAR Description[FlagsCount + 1];
	FFlagContainer FlagsCopy = Flags;
	int32 FlagIndex = 0;
	for (; FlagIndex < FlagsCount && FlagsCopy != 0; ++FlagIndex)
	{
		Description[FlagIndex] = (FlagsCopy & (1 << FlagIndex)) ? TCHAR('1') : TCHAR('0');
		FlagsCopy &= ~(1 << FlagIndex);
	}
	Description[FlagIndex] = TCHAR('\0');
	return FString(Description);
}

#undef LOCTEXT_NAMESPACE