
#include "AbilitySystemPrivatePCH.h"
#include "Abilities/Tasks/AbilityTask_MoveToLocation.h"

#pragma optimize( "", off )

UAbilityTask_MoveToLocation::UAbilityTask_MoveToLocation(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{

}

void UAbilityTask_MoveToLocation::InterpolatePosition()
{
	if (Ability.IsValid())
	{
		if (AActor* ActorOwner = Cast<AActor>(Ability->GetOuter()))
		{
			float CurrentTime = ActorOwner->GetWorld()->GetTimeSeconds();

			if (CurrentTime >= TimeMoveWillEnd)
			{
				ActorOwner->SetActorLocation(TargetLocation);
				OnTargetLocationReached.Broadcast();
				EndTask();
			}
			else
			{
				float MoveFraction = (CurrentTime - TimeMoveStarted) / DurationOfMovement;
				if (LerpCurve.IsValid())
				{
					MoveFraction = LerpCurve.Get()->GetFloatValue(MoveFraction);
				}
				ActorOwner->SetActorLocation(FMath::Lerp<FVector, float>(StartLocation, TargetLocation, MoveFraction));				
			}
		}
	}
}

UAbilityTask_MoveToLocation* UAbilityTask_MoveToLocation::MoveToLocation(class UObject* WorldContextObject, FVector Location, float Duration, UCurveFloat* OptionalInterpolationCurve)
{
	auto MyObj = NewTask<UAbilityTask_MoveToLocation>(WorldContextObject);

	MyObj->StartLocation = MyObj->GetActor()->GetActorLocation();
	MyObj->TargetLocation = Location;
	MyObj->DurationOfMovement = FMath::Max(Duration, 0.001f);		//Avoid negative or divide-by-zero cases
	MyObj->TimeMoveStarted = MyObj->GetWorld()->GetTimeSeconds();
	MyObj->TimeMoveWillEnd = MyObj->TimeMoveStarted + MyObj->DurationOfMovement;
	MyObj->LerpCurve = OptionalInterpolationCurve;

	return MyObj;
}

void UAbilityTask_MoveToLocation::OnDestroy(bool AbilityEnded)
{
	GetWorld()->GetTimerManager().ClearTimer(this, &UAbilityTask_MoveToLocation::InterpolatePosition);

	Super::OnDestroy(AbilityEnded);
}

void UAbilityTask_MoveToLocation::Activate()
{
	// FIXME: This is bad - its calling InterpolatePOsition @ 1000hz!!!!
	GetWorld()->GetTimerManager().SetTimer(this, &UAbilityTask_MoveToLocation::InterpolatePosition, 0.001f, true);
}