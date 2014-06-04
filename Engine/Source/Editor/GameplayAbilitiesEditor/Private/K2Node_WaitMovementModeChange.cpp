// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "SkillSystemEditorModulePrivatePCH.h"
#include "K2Node_WaitMovementModeChange.h"
#include "SkillSystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_WaitMovementModeChange.h"
#include "CompilerResultsLog.h"

#define LOCTEXT_NAMESPACE "K2Node_WaitMovementModeChange"

UK2Node_WaitMovementModeChange::UK2Node_WaitMovementModeChange(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	
	ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(USkillSystemBlueprintLibrary, CreateWaitMovementModeChange);
	ProxyFactoryClass = USkillSystemBlueprintLibrary::StaticClass();
	ProxyClass = UAbilityTask_WaitMovementModeChange::StaticClass();
}

FString UK2Node_WaitMovementModeChange::GetCategoryName()
{
	return TEXT("Ability");
}

FString UK2Node_WaitMovementModeChange::GetTooltip() const
{
	return TEXT("Waits until the movement mode changes");
}

FText UK2Node_WaitMovementModeChange::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("WaitMovementModeChange", "Wait Movement Mode Change");
}

#undef LOCTEXT_NAMESPACE


