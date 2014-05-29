
#include "SkillSystemModulePrivatePCH.h"

UBlueprintWaitMovementModeChangeTaskProxy::UBlueprintWaitMovementModeChangeTaskProxy(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	RequiredMode = MOVE_None;
}

void UBlueprintWaitMovementModeChangeTaskProxy::OnMovementModeChange(ACharacter * Character, EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	if (Character)
	{
		if (UCharacterMovementComponent *MoveComp = Cast<UCharacterMovementComponent>(Character->GetMovementComponent()))
		{
			if (RequiredMode == MOVE_None || MoveComp->MovementMode == RequiredMode)
			{
				Character->MovementModeChangedDelegate.RemoveDynamic(this, &UBlueprintWaitMovementModeChangeTaskProxy::OnMovementModeChange);
				OnChange.Broadcast(MoveComp->MovementMode);
				return;
			}
		}
	}
}