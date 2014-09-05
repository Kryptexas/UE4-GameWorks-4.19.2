
#include "AbilitySystemPrivatePCH.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilitySystemComponent.h"

UAbilityTask::UAbilityTask(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bTickingTask = false;
}

void UAbilityTask::Activate()
{

}

void UAbilityTask::InitTask(UGameplayAbility* InAbility)
{
	Ability = InAbility;
	AbilitySystemComponent = InAbility->GetCurrentActorInfo()->AbilitySystemComponent;
	InAbility->TaskStarted(this);

	// If this task requires ticking, register it with AbilitySystemComponent
	if (bTickingTask && AbilitySystemComponent.IsValid())
	{
		AbilitySystemComponent->TickingTaskStarted(this);
	}
}

UWorld* UAbilityTask::GetWorld() const
{
	if (AbilitySystemComponent.IsValid())
	{
		return AbilitySystemComponent.Get()->GetWorld();
	}

	return nullptr;
}

AActor* UAbilityTask::GetActor() const
{
	if (Ability.IsValid())
	{
		const FGameplayAbilityActorInfo* Info = Ability->GetCurrentActorInfo();
		if (Info)
		{
			return Info->Actor.Get();
		}
	}

	return nullptr;	
}

void UAbilityTask::AbilityEnded()
{
	if (!IsPendingKill())
	{
		OnDestroy(true);
	}
}

void UAbilityTask::EndTask()
{
	if (!IsPendingKill())
	{
		OnDestroy(false);
	}
}

void UAbilityTask::ExternalConfirm(bool bEndTask)
{
	if (bEndTask)
	{
		EndTask();
	}
}

void UAbilityTask::ExternalCancel()
{
	EndTask();
}

void UAbilityTask::OnDestroy(bool AbilityIsEnding)
{
	ensure(!IsPendingKill());

	// If this task required ticking, unregister it with AbilitySystemComponent
	if (bTickingTask && AbilitySystemComponent.IsValid())
	{
		AbilitySystemComponent->TickingTaskEnded(this);
	}

	// Remove ourselves from the owning Ability's task list, if the ability isn't ending
	if (!AbilityIsEnding && Ability.IsValid())
	{
		Ability->TaskEnded(this);
	}

	MarkPendingKill();
}