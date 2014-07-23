
#include "AbilitySystemPrivatePCH.h"
#include "Abilities/Tasks/AbilityTask_MoveToLocation.h"

UAbilityTask_MoveToLocation::UAbilityTask_MoveToLocation(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{

}



void UAbilityTask_MoveToLocation::OnOverlapCallback(class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult)
{
	/*
	if (OtherActor)
	{
		// Construct TargetData
		FGameplayAbilityTargetData_SingleTargetHit * TargetData = new FGameplayAbilityTargetData_SingleTargetHit(Hit);

		// Give it a handle and return
		FGameplayAbilityTargetDataHandle	Handle;
		Handle.Data = TSharedPtr<FGameplayAbilityTargetData>(TargetData);
		OnOverlap.Broadcast(Handle);
	}
	*/
}


/**
*	Need:
*	-Easy way to specify which primitive components should be used for this overlap test
*	-Easy way to specify which types of actors/collision overlaps that we care about/want to block on
*/

void UAbilityTask_MoveToLocation::InterpolatePosition()
{
	if (Ability.IsValid())
	{
		if (AActor* ActorOwner = Cast<AActor>(Ability->GetOuter()))
		{
			float CurrentTime = ActorOwner->GetWorld()->GetTimeSeconds();

			if (CurrentTime >= TimeMoveWillEnd)
			{
				ActorOwner->GetWorld()->GetTimerManager().ClearTimer(this, &UAbilityTask_MoveToLocation::InterpolatePosition);
				ActorOwner->SetActorLocation(TargetLocation);
				OnTargetLocationReached.Broadcast();
			}
			else
			{
				float MoveFraction = (CurrentTime - TimeMoveStarted) / DurationOfMovement;
				ActorOwner->SetActorLocation(FMath::Lerp<FVector, float>(StartLocation, TargetLocation, MoveFraction));
			}
		}
	}
}

UAbilityTask_MoveToLocation* UAbilityTask_MoveToLocation::MoveToLocation(class UObject* WorldContextObject, FVector Location, float Duration)
{
	check(WorldContextObject);
	UGameplayAbility* ThisAbility = CastChecked<UGameplayAbility>(WorldContextObject);
	check(ThisAbility);
	AActor* ActorOwner = Cast<AActor>(ThisAbility->GetOuter());
	check(ActorOwner);

	UAbilityTask_MoveToLocation* MyObj = NewObject<UAbilityTask_MoveToLocation>();
	MyObj->Ability = ThisAbility;

	MyObj->StartLocation = ActorOwner->GetActorLocation();
	MyObj->TargetLocation = Location;
	MyObj->DurationOfMovement = FMath::Max(Duration, 0.001f);		//Avoid negative or divide-by-zero cases
	MyObj->TimeMoveStarted = WorldContextObject->GetWorld()->GetTimeSeconds();
	MyObj->TimeMoveWillEnd = MyObj->TimeMoveStarted + MyObj->DurationOfMovement;
	WorldContextObject->GetWorld()->GetTimerManager().SetTimer(MyObj, &UAbilityTask_MoveToLocation::InterpolatePosition, 0.001f, true);

	return MyObj;
}

void UAbilityTask_MoveToLocation::Activate()
{
	if (Ability.IsValid())
	{
		AActor* ActorOwner = Cast<AActor>(Ability->GetOuter());

		// TEMP - we are just using root component's collision. A real system will need more data to specify which component to use
		UPrimitiveComponent * PrimComponent = Cast<UPrimitiveComponent>(ActorOwner->GetRootComponent());
		if (!PrimComponent)
		{
			PrimComponent = ActorOwner->FindComponentByClass<UPrimitiveComponent>();
		}
		check(PrimComponent);

		//PrimComponent->OnComponentBeginOverlap.AddDynamic(this, &UAbilityTask_WaitOverlap::OnOverlapCallback);
	}
}
