// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTaskOwnerInterface.h"
#include "GameplayTasksComponent.generated.h"

class FOutBunch;
class UActorChannel;
class UGameplayTask;

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

	/** tasks that have finished but have not been destroyed yet. We destroy tasks in a latent 
	 *	manner mostly to be able to safely send out broadcast notifications after the task has 
	 *	been marked as "finished" */
	UPROPERTY()
	TArray<UGameplayTask*> FinishedTasks;

	/** Array of currently active UAbilityTasks that require ticking */
	TArray<TWeakObjectPtr<UGameplayTask> >	TickingTasks;

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
	virtual void TaskStarted(UGameplayTask& NewTask) override;
	/** Removes a task (most often an UAbilityTask) task from the list of tasks to be ticked */
	virtual void TaskEnded(UGameplayTask& Task) override;
	virtual AActor* GetOwnerActor() const override { return GetOwner(); }
	virtual AActor* GetAvatarActor() const override;

protected:
	void RequestTicking();
};