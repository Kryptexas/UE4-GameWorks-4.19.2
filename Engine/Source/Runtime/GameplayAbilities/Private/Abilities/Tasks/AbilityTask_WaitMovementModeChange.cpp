
#include "AbilitySystemPrivatePCH.h"
#include "Abilities/Tasks/AbilityTask_WaitMovementModeChange.h"

UAbilityTask_WaitMovementModeChange::UAbilityTask_WaitMovementModeChange(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	RequiredMode = MOVE_None;
}

UAbilityTask_WaitMovementModeChange* UAbilityTask_WaitMovementModeChange::CreateWaitMovementModeChange(class UObject* WorldContextObject, EMovementMode NewMode)
{
	check(WorldContextObject);

	UAbilityTask_WaitMovementModeChange* MyObj = NewObject<UAbilityTask_WaitMovementModeChange>();
	UGameplayAbility* ThisAbility = CastChecked<UGameplayAbility>(WorldContextObject);
	MyObj->Ability = ThisAbility;
	MyObj->RequiredMode = NewMode;

	return MyObj;
}

void UAbilityTask_WaitMovementModeChange::Activate()
{
	if (Ability.IsValid())
	{
		// Fixme: getting from the activating ability to the owning character is pretty awkward. We need
		// a more standard way of doing this along with with a way for ability tasks to specify their requirements 
		// (e.g., requires character / pawn, etc)

		AActor * ActorOwner = Cast<AActor>(Ability->GetOuter());

		TSharedPtr<FGameplayAbilityActorInfo> ActorInfo = TSharedPtr<FGameplayAbilityActorInfo>(UAbilitySystemGlobals::Get().AllocAbilityActorInfo(ActorOwner));

		ACharacter * Character = Cast<ACharacter>(ActorInfo->Actor.Get());
		if (Character)
		{
			Character->MovementModeChangedDelegate.AddDynamic(this, &UAbilityTask_WaitMovementModeChange::OnMovementModeChange);
		}
	}
}

void UAbilityTask_WaitMovementModeChange::OnMovementModeChange(ACharacter * Character, EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	if (Character)
	{
		if (UCharacterMovementComponent *MoveComp = Cast<UCharacterMovementComponent>(Character->GetMovementComponent()))
		{
			if (RequiredMode == MOVE_None || MoveComp->MovementMode == RequiredMode)
			{
				Character->MovementModeChangedDelegate.RemoveDynamic(this, &UAbilityTask_WaitMovementModeChange::OnMovementModeChange);
				OnChange.Broadcast(MoveComp->MovementMode);
				return;
			}
		}
	}
}