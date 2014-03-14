// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PlayerInput.cpp: Unreal input system.
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineUserInterfaceClasses.h"

UInputSettings::UInputSettings(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
}

#if WITH_EDITOR
void UInputSettings::PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent)
{
	Super::PostEditChangeChainProperty(PropertyChangedEvent);

	const FName MemberPropertyName = PropertyChangedEvent.PropertyChain.GetActiveMemberNode()->GetValue()->GetFName();

	for (TObjectIterator<UPlayerInput> It; It; ++It)
	{
		It->ForceRebuildingKeyMaps(true);
	}
}

#endif

void UInputSettings::SaveKeyMappings()
{
	ActionMappings.Sort();
	AxisMappings.Sort();
	SaveConfig();
}

void UInputSettings::AddActionMapping(const FInputActionKeyMapping& KeyMapping)
{
	ActionMappings.AddUnique(KeyMapping);
}

void UInputSettings::RemoveActionMapping(const FInputActionKeyMapping& KeyMapping)
{
	for (int32 ActionIndex = ActionMappings.Num() - 1; ActionIndex >= 0; --ActionIndex)
	{
		if (ActionMappings[ActionIndex] == KeyMapping)
		{
			ActionMappings.RemoveAtSwap(ActionIndex);
			// we don't break because the mapping may have been in the array twice
		}
	}
}

void UInputSettings::AddAxisMapping(const FInputAxisKeyMapping& KeyMapping)
{
	AxisMappings.AddUnique(KeyMapping);
}

void UInputSettings::RemoveAxisMapping(const FInputAxisKeyMapping& InKeyMapping)
{
	for (int32 AxisIndex = AxisMappings.Num() - 1; AxisIndex >= 0; --AxisIndex)
	{
		const FInputAxisKeyMapping& KeyMapping = AxisMappings[AxisIndex];
		if (KeyMapping.AxisName == InKeyMapping.AxisName
			&& KeyMapping.Key == InKeyMapping.Key)
		{
			AxisMappings.RemoveAtSwap(AxisIndex);
			// we don't break because the mapping may have been in the array twice
		}
	}
}

void UInputSettings::GetActionNames(TArray<FName>& ActionNames) const
{
	ActionNames.Empty();

	for (const FInputActionKeyMapping& ActionMapping : ActionMappings)
	{
		ActionNames.AddUnique(ActionMapping.ActionName);
	}
}

void UInputSettings::GetAxisNames(TArray<FName>& AxisNames) const
{
	AxisNames.Empty();

	for (const FInputAxisKeyMapping& AxisMapping : AxisMappings)
	{
		AxisNames.AddUnique(AxisMapping.AxisName);
	}
}