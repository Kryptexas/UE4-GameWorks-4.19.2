
#include "AbilitySystemPrivatePCH.h"
#include "Abilities/Tasks/AbilityTask_MoveToLocation.h"

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
				ActorOwner->GetWorld()->GetTimerManager().ClearTimer(this, &UAbilityTask_MoveToLocation::InterpolatePosition);
				ActorOwner->SetActorLocation(TargetLocation);
				OnTargetLocationReached.Broadcast();
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
	MyObj->LerpCurve = OptionalInterpolationCurve;
	WorldContextObject->GetWorld()->GetTimerManager().SetTimer(MyObj, &UAbilityTask_MoveToLocation::InterpolatePosition, 0.001f, true);

	return MyObj;
}
