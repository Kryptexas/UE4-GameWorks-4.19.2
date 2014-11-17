
#include "AbilitySystemPrivatePCH.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UAbilityTask_PlayMontageAndWait::UAbilityTask_PlayMontageAndWait(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

}

void UAbilityTask_PlayMontageAndWait::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (bInterrupted)
	{
		OnInterrupted.Broadcast();
	}
	else
	{
		OnComplete.Broadcast();
	}
}