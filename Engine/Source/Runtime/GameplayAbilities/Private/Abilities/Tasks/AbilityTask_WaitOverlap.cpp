
#include "AbilitySystemPrivatePCH.h"
#include "Abilities/Tasks/AbilityTask_WaitOverlap.h"

UAbilityTask_WaitOverlap::UAbilityTask_WaitOverlap(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

}


void UAbilityTask_WaitOverlap::OnOverlapCallback(class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	if (OtherActor && bFromSweep)
	{	
		//OnOverlap.Broadcast(SweepResult);
	}
}

void UAbilityTask_WaitOverlap::OnHitCallback(AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if(OtherActor)
	{
		// Construct TargetData
		FGameplayAbilityTargetData_SingleTargetHit * TargetData = new FGameplayAbilityTargetData_SingleTargetHit(Hit);

		// Give it a handle and return
		FGameplayAbilityTargetDataHandle	Handle;
		Handle.Data = TSharedPtr<FGameplayAbilityTargetData>(TargetData);
		OnOverlap.Broadcast(Handle);


		// We are done. Kill us so we don't keep getting broadcast messages
		MarkPendingKill();
	}
}