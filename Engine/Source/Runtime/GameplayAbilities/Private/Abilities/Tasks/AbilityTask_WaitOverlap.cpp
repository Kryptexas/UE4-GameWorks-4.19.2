
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

/**
*	Need:
*	-Easy way to specify which primitive components should be used for this overlap test
*	-Easy way to specify which types of actors/collision overlaps that we care about/want to block on
*/

UAbilityTask_WaitOverlap* UAbilityTask_WaitOverlap::WaitForOverlap(class UObject* WorldContextObject)
{
	check(WorldContextObject);

	UAbilityTask_WaitOverlap * MyObj = NewObject<UAbilityTask_WaitOverlap>();
	UGameplayAbility* ThisAbility = CastChecked<UGameplayAbility>(WorldContextObject);
	MyObj->Ability = ThisAbility;

	return MyObj;
}

void UAbilityTask_WaitOverlap::Activate()
{
	if (Ability.IsValid())
	{
		AActor * ActorOwner = Cast<AActor>(Ability->GetOuter());

		// TEMP - we are just using root component's collision. A real system will need more data to specify which component to use
		UPrimitiveComponent * PrimComponent = Cast<UPrimitiveComponent>(ActorOwner->GetRootComponent());
		if (!PrimComponent)
		{
			PrimComponent = ActorOwner->FindComponentByClass<UPrimitiveComponent>();
		}
		check(PrimComponent);

		PrimComponent->OnComponentBeginOverlap.AddDynamic(this, &UAbilityTask_WaitOverlap::OnOverlapCallback);
		PrimComponent->OnComponentHit.AddDynamic(this, &UAbilityTask_WaitOverlap::OnHitCallback);
	}
}