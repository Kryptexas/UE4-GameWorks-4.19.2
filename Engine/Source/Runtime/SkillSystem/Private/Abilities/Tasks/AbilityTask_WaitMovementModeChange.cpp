
#include "SkillSystemModulePrivatePCH.h"

UAbilityTask_WaitMovementModeChange::UAbilityTask_WaitMovementModeChange(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	RequiredMode = MOVE_None;
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