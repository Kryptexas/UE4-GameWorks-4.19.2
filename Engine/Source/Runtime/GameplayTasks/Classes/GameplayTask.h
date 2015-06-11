// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "GameplayTaskOwnerInterface.h"
#include "GameplayTaskTypes.h"
#include "GameplayTask.generated.h"

class UGameplayTask;
class UGameplayTaskResource;
class UGameplayTasksComponent; 

GAMEPLAYTASKS_API DECLARE_LOG_CATEGORY_EXTERN(LogGameplayTasks, Log, All);

UENUM()
enum class EGameplayTaskState : uint8
{
	Uninitialized,
	AwaitingActivation,
	Paused,
	Active,
	Finished
};

UENUM()
enum class ETaskResourceOverlapPolicy : uint8
{
	/** Pause overlapping same-priority tasks. */
	StartOnTop,
	/** Wait for other same-priority tasks to finish. */
	StartAtEnd,
};
	

UCLASS(Abstract)
class GAMEPLAYTASKS_API UGameplayTask : public UObject
{
	GENERATED_BODY()

public:

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGenericGameplayTaskDelegate);

	UGameplayTask(const FObjectInitializer& ObjectInitializer);
	
	/** Called to trigger the actual task once the delegates have been set up */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"), Category = "Gameplay Tasks")
	void ReadyForActivation();

protected:
	/** Called to trigger the actual task once the delegates have been set up
	 *	Note that the default implementation does nothing and you don't have to call it */
	virtual void Activate();

	/** Initailizes the task with the task owner interface instance but does not actviate until Activate() is called */
	void InitTask(IGameplayTaskOwnerInterface& InTaskOwner);

public:
	virtual void InitSimulatedTask(UGameplayTasksComponent& InGameplayTasksComponent);

	/** Tick function for this task, if bTickingTask == true */
	virtual void TickTask(float DeltaTime) {}

	/** Called when the task is asked to confirm from an outside node. What this means depends on the individual task. By default, this does nothing other than ending if bEndTask is true. */
	virtual void ExternalConfirm(bool bEndTask);

	/** Called when the task is asked to cancel from an outside node. What this means depends on the individual task. By default, this does nothing other than ending the task. */
	virtual void ExternalCancel();

	/** Return debug string describing task */
	virtual FString GetDebugString() const;

	/** Helper function for getting UWorld off a task */
	virtual UWorld* GetWorld() const override;

	/** Proper way to get the owning actor of task owner. This can be the owner itself since the owner is given as a interface */
	AActor* GetOwnerActor() const;

	/** Proper way to get the avatar actor associated with the task owner (usually a pawn, tower, etc) */
	AActor* GetAvatarActor() const;

	/** Helper function for instantiating and initializing a new task */
	template <class T>
	FORCEINLINE static T* NewTask(UObject* WorldContextObject, FName InstanceName = FName())
	{
		return WorldContextObject != nullptr ? NewTask<T>(*WorldContextObject, InstanceName) : nullptr;
	}

	template <class T>
	FORCEINLINE static T* NewTask(TScriptInterface<IGameplayTaskOwnerInterface> TaskOwner, FName InstanceName = FName())
	{
		return TaskOwner.GetInterface() != nullptr ? NewTask<T>(*TaskOwner, InstanceName) : nullptr;
	}

	template <class T>
	inline static T* NewTask(UObject& WorldContextObject, FName InstanceName = FName());

	template <class T>
	inline static T* NewTask(IGameplayTaskOwnerInterface& TaskOwner, FName InstanceName = FName());

	/** Called when task owner has "ended" (before the task ends) kills the task. Calls OnDestroy. */
	void TaskOwnerEnded();

	/** Called explicitly to end the task (usually by the task itself). Calls OnDestroy. 
	 *	@NOTE: you need to call EndTask before sending out any "on completed" delegates. 
	 *	If you don't the task will still be in an "active" state while the event receivers may
	 *	assume it's already "finished" */
	UFUNCTION(BlueprintCallable, Category="GameplayTasks")
	void EndTask();
	
	virtual bool IsSupportedForNetworking() const override { return bSimulatedTask; }

	FORCEINLINE FName GetInstanceName() const { return InstanceName; }
	FORCEINLINE bool IsTickingTask() const { return (bTickingTask != 0); }
	FORCEINLINE bool IsSimulatedTask() const { return (bSimulatedTask != 0); }
	FORCEINLINE bool IsSimulating() const { return (bIsSimulating != 0); }
	FORCEINLINE bool IsPausable() const { return (bIsPausable != 0); }
	FORCEINLINE uint8 GetPriority() const { return Priority; }
	FORCEINLINE bool RequiresPriorityOrResourceManagement() const { return bCaresAboutPriority == true || RequiredResources.IsEmpty() == false; }
	FORCEINLINE FGameplayResourceSet GetRequiredResources() { return RequiredResources; }
	
	FORCEINLINE EGameplayTaskState GetState() const { return TaskState; }
	FORCEINLINE bool IsActive() const { return (TaskState == EGameplayTaskState::Active); }
	
	IGameplayTaskOwnerInterface* GetTaskOwner() const { return TaskOwner.IsValid() ? &(*TaskOwner) : nullptr; }
	UGameplayTasksComponent* GetGameplayTasksComponent() { return TasksComponent.Get(); }
	bool IsOwnedByTasksComponent() const { return bOwnedByTasksComponent; }

	template <class T>
	inline void RequireResource()
	{
		RequireResource(T::StaticClass());
	}

	/** Marks this task as requiring specified resource which has a number of consequences,
	*	like task not being able to run if the resource is already taken.
	*
	*	@node: Calling this function makes sense only until the task is being passed over to the GameplayTasksComponent.
	*	Once that's that resources data is consumed and further changes won't get applied */
	void RequireResource(TSubclassOf<UGameplayTaskResource> RequiredResource);

	ETaskResourceOverlapPolicy GetResourceOverlapPolicy() const { return ResourceOverlapPolicy; }

protected:
	/** End and CleanUp the task - may be called by the task itself or by the task owner if the owner is ending. 
	 *	IMPORTANT! Do NOT call directly! Call EndTask() or TaskOwnerEnded() 
	 *	IMPORTANT! When overriding this function make sure to call Super::OnDestroy(bOwnerFinished) as the last thing,
	 *		since the function internally marks the task as "Pending Kill", and this may interfere with internal BP mechanics
	 */
	virtual void OnDestroy(bool bOwnerFinished);

	static IGameplayTaskOwnerInterface* ConvertToTaskOwner(UObject& OwnerObject);
	static IGameplayTaskOwnerInterface* ConvertToTaskOwner(AActor& OwnerActor);

	// protected by design. Not meant to be called outside from GameplayTaskComponent mechanics
	virtual void Pause();
	virtual void Resume();

private:
	friend UGameplayTasksComponent;

	void ActivateInTaskQueue();
	void PauseInTaskQueue();
	
	void PerformActivation();
	
protected:

	/** This name allows us to find the task later so that we can end it. */
	UPROPERTY()
	FName InstanceName;

	/** This controls how this task will be treaded in relation to other, already running tasks, 
	 *	provided GameplayTasksComponent is configured to care about priorities (the default behavior)*/
	uint8 Priority;

	EGameplayTaskState TaskState;

	ETaskResourceOverlapPolicy ResourceOverlapPolicy;

	/** If true, this task will receive TickTask calls from TasksComponent */
	uint32 bTickingTask : 1;

	/** Should this task run on simulated clients? This should only be used in rare cases, such as movement tasks. Simulated Tasks do not broadcast their end delegates.  */
	uint32 bSimulatedTask : 1;

	/** Am I actually running this as a simulated task. (This will be true on clients that simulating. This will be false on the server and the owning client) */
	uint32 bIsSimulating : 1;

	uint32 bIsPausable : 1;

	uint32 bCaresAboutPriority : 1;

	/** this is set to avoid duplicate calls to task's owner and TasksComponent when both are the same object */
	uint32 bOwnedByTasksComponent : 1;
	
	/** Abstract "resource" IDs this task claims it needs. Note that we do not support dynamic changes to this
	 *	variable. Once a task gets submitted to GameplayTasksComponent it's settled 
	 *	@NOTE: if dynamic changes were needed though look for GetRequiredResources occurrences in GameplayTasksComponent
	 *	and replace all uses of cached values with actual GetRequiredResources calls */
	FGameplayResourceSet RequiredResources;

	/** Task Owner that created us */
	TWeakInterfacePtr<IGameplayTaskOwnerInterface> TaskOwner;

	TWeakObjectPtr<UGameplayTasksComponent>	TasksComponent;

#if ENABLE_VISUAL_LOG
	mutable FString DebugDescription;
public:
	const FString& GetDebugDescription() const
	{
		if (DebugDescription.IsEmpty())
		{
			DebugDescription = GenerateDebugDescription();
		}
		return DebugDescription;
	}
	virtual FString GenerateDebugDescription() const;
	FString GetTaskStateName() const;
#endif // ENABLE_VISUAL_LOG
};

template <class T>
T* UGameplayTask::NewTask(UObject& WorldContextObject, FName InstanceName)
{
	IGameplayTaskOwnerInterface* TaskOwner = ConvertToTaskOwner(WorldContextObject);
	return (TaskOwner) ? NewTask<T>(*TaskOwner, InstanceName) : nullptr;
}

template <class T>
T* UGameplayTask::NewTask(IGameplayTaskOwnerInterface& TaskOwner, FName InstanceName)
{
	T* MyObj = NewObject<T>();
	MyObj->InitTask(TaskOwner);
	MyObj->InstanceName = InstanceName;
	MyObj->Priority = TaskOwner.GetDefaultPriority();
	return MyObj;
}

//For searching through lists of task instances
struct FGameplayTaskInstanceNamePredicate
{
	FGameplayTaskInstanceNamePredicate(FName DesiredInstanceName)
	{
		InstanceName = DesiredInstanceName;
	}

	bool operator()(const TWeakObjectPtr<UGameplayTask> A) const
	{
		return (A.IsValid() && !A.Get()->GetInstanceName().IsNone() && A.Get()->GetInstanceName().IsValid() && (A.Get()->GetInstanceName() == InstanceName));
	}

	FName InstanceName;
};


struct FGameplayTaskInstanceClassPredicate
{
	FGameplayTaskInstanceClassPredicate(TSubclassOf<UGameplayTask> Class)
	{
		TaskClass = Class;
	}

	bool operator()(const TWeakObjectPtr<UGameplayTask> A) const
	{
		return (A.IsValid() && (A.Get()->GetClass() == TaskClass));
	}

	TSubclassOf<UGameplayTask> TaskClass;
};

