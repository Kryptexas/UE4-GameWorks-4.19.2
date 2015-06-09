// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTaskOwnerInterface.h"
#include "GameplayTaskTypes.h"
#include "GameplayTask.h"
#include "GameplayTasksComponent.generated.h"

class FOutBunch;
class UActorChannel;

enum class EGameplayTaskEvent
{
	Add,
	Remove,
};

struct FGameplayTaskEventData
{
	EGameplayTaskEvent Event;
	UGameplayTask& RelatedTask;

	FGameplayTaskEventData(EGameplayTaskEvent InEvent, UGameplayTask& InRelatedTask)
		: Event(InEvent), RelatedTask(InRelatedTask)
	{

	}
};

/**
*	The core ActorComponent for interfacing with the GameplayAbilities System
*/
UCLASS(ClassGroup = GameplayTasks, hidecategories = (Object, LOD, Lighting, Transform, Sockets, TextureStreaming), editinlinenew, meta = (BlueprintSpawnableComponent))
class GAMEPLAYTASKS_API UGameplayTasksComponent : public UActorComponent, public IGameplayTaskOwnerInterface
{
	GENERATED_BODY()

protected:
	/** Tasks that run on simulated proxies */
	UPROPERTY(ReplicatedUsing = OnRep_SimulatedTasks)
	TArray<UGameplayTask*> SimulatedTasks;

	UPROPERTY()
	TArray<UGameplayTask*> TaskPriorityQueue;

	/** Transient array of events whose main role is to avoid
	 *	long chain of recurrent calls if an activated/paused/removed task 
	 *	wants to push/pause/kill other tasks.
	 *	Note: this TaskEvents is assumed to be used in a single thread */
	TArray<FGameplayTaskEventData> TaskEvents;

	/** Array of currently active UAbilityTasks that require ticking */
	TArray<TWeakObjectPtr<UGameplayTask> > TickingTasks;

	/** Indicates what's the highest priority among currently running tasks */
	uint8 TopActivePriority;

	/** Resources used by currently active tasks */
	FGameplayResourceSet CurrentlyUsedResources;

public:
	UGameplayTasksComponent(const FObjectInitializer& ObjectInitializer);
	
	UFUNCTION()
	void OnRep_SimulatedTasks();

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;
	virtual bool ReplicateSubobjects(UActorChannel *Channel, FOutBunch* Bunch, FReplicationFlags* RepFlags) override;

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void UpdateShouldTick();

	/** retrieves information whether this component should be ticking taken current
	*	activity into consideration*/
	virtual bool GetShouldTick() const;
	
	virtual UGameplayTasksComponent* GetGameplayTasksComponent() override { return this; }
	/** Adds a task (most often an UAbilityTask) to the list of tasks to be ticked */
	virtual void OnTaskActivated(UGameplayTask& Task) override;
	/** Removes a task (most often an UAbilityTask) task from the list of tasks to be ticked */
	virtual void OnTaskDeactivated(UGameplayTask& Task) override;

	virtual AActor* GetOwnerActor() const override { return GetOwner(); }
	virtual AActor* GetAvatarActor() const override;
		
	/** processes the task and figures out if it should get triggered instantly or wait
	 *	based on task's RequiredResources, Priority and ResourceOverlapPolicy */
	void AddTaskReadyForActivation(UGameplayTask& NewTask);

	void RemoveResourceConsumingTask(UGameplayTask& Task);

	FORCEINLINE FGameplayResourceSet GetCurrentlyUsedResources() const { return CurrentlyUsedResources; }

#if ENABLE_VISUAL_LOG
	static FString GetTaskStateName(EGameplayTaskState Value);
	void DescribeSelfToVisLog(struct FVisualLogEntry* Snapshot) const;
#endif // ENABLE_VISUAL_LOG

protected:
	void RequestTicking();
	void ProcessTaskEvents();
	void UpdateTaskActivationFromIndex(int32 StartingIndex, FGameplayResourceSet ResourcesUsedByActiveUpToIndex, FGameplayResourceSet ResourcesRequiredUpToIndex);

private:
	/** called when a task gets ended with an external call, meaning not coming from UGameplayTasksComponent mechanics */
	void OnTaskEnded(UGameplayTask& Task);

	void AddTaskToPriorityQueue(UGameplayTask& NewTask);
	void RemoveTaskFromPriorityQueue(UGameplayTask& Task);
};