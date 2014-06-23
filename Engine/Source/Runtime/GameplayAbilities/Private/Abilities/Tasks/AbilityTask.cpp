
#include "AbilitySystemPrivatePCH.h"
#include "Abilities/Tasks/AbilityTask.h"

UAbilityTask::UAbilityTask(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

}

void UAbilityTask::Activate()
{

}

void UAbilityTask::InitTask(UGameplayAbility* InAbility)
{
	Ability = InAbility;
	AbilitySystemComponent = InAbility->GetCurrentActorInfo()->AbilitySystemComponent;
}