
#include "AbilitySystemPrivatePCH.h"
#include "Abilities/Tasks/AbilityTask_MoveToLocation.h"

UAbilityTask_MoveToLocation::UAbilityTask_MoveToLocation(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
	bTickingTask = true;
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

void UAbilityTask_MoveToLocation::Activate()
{
}

//TODO: This is still an awful way to do this and we should scrap this task or do it right.
void UAbilityTask_MoveToLocation::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);
	AActor* MyActor = GetActor();
	if (MyActor)
	{
		float CurrentTime = GetWorld()->GetTimeSeconds();

		if (CurrentTime >= TimeMoveWillEnd)
		{
			MyActor->SetActorLocation(TargetLocation);
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
			MyActor->SetActorLocation(FMath::Lerp<FVector, float>(StartLocation, TargetLocation, MoveFraction));
		}
	}
}
