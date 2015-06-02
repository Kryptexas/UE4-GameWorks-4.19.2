// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "GameplayTaskOwnerInterface.h"
#include "GameplayTaskTypes.h"
#include "GameplayTask.generated.h"

class UGameplayTask;
class UGameplayTasksComponent; 

GAMEPLAYTASKS_API DECLARE_LOG_CATEGORY_EXTERN(LogGameplayTask, Log, All);

UENUM()
enum class EGameplayTaskState : uint8
{
	Uninitialized,
	AwaitingActivation,
	Active,
	Finished
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
	virtual void Activate() {}

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
	static T* NewTask(UObject& WorldContextObject, FName InstanceName = FName());

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

	FORCEINLINE EGameplayTaskState GetState() const { return TaskState; }

	UGameplayTasksComponent* GetGameplayTasksComponent() { return TasksComponent.Get(); }

protected:
	/** End and CleanUp the task - may be called by the task itself or by the task owner if the owner is ending. Do NOT call directly! Call EndTask() or TaskOwnerEnded() */
	virtual void OnDestroy(bool bOwnerFinished);

	static IGameplayTaskOwnerInterface* ConvertToTaskOwner(UObject& OwnerObject);
	static IGameplayTaskOwnerInterface* ConvertToTaskOwner(AActor& OwnerActor);

	/** This name allows us to find the task later so that we can end it. */
	UPROPERTY()
	FName InstanceName;

	/** If true, this task will receive TickTask calls from TasksComponent */
	uint32 bTickingTask : 1;

	/** Should this task run on simulated clients? This should only be used in rare cases, such as movement tasks. Simulated Tasks do not broadcast their end delegates.  */
	uint32 bSimulatedTask : 1;

	/** Am I actually running this as a simulated task. (This will be true on clients that simulating. This will be false on the server and the owning client) */
	uint32 bIsSimulating : 1;

	uint32 bIsPausable : 1;

	/** this is set to avoid duplicate calls to task's owner and TasksComponent when both are the same object */
	uint32 bOwnedByTasksComponent : 1;

	EGameplayTaskState TaskState;

	/** Task Owner that created us */
	TWeakInterfacePtr<IGameplayTaskOwnerInterface> TaskOwner;

	TWeakObjectPtr<UGameplayTasksComponent>	TasksComponent;
};

template <class T>
T* UGameplayTask::NewTask(UObject& WorldContextObject, FName InstanceName)
{
	IGameplayTaskOwnerInterface* TaskOwner = ConvertToTaskOwner(WorldContextObject);

	if (TaskOwner)
	{
		T* MyObj = NewObject<T>();
		MyObj->InitTask(*TaskOwner);
		MyObj->InstanceName = InstanceName;
		return MyObj;
	}

	return nullptr;
}

template <class T>
T* UGameplayTask::NewTask(IGameplayTaskOwnerInterface& TaskOwner, FName InstanceName)
{
	T* MyObj = NewObject<T>();
	MyObj->InitTask(TaskOwner);
	MyObj->InstanceName = InstanceName;
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

