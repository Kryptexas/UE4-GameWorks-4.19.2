// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "SkillSystemEditorModulePrivatePCH.h"
#include "K2Node_WaitOverlap.h"
#include "SkillSystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_WaitOverlap.h"
#include "CompilerResultsLog.h"

#define LOCTEXT_NAMESPACE "K2Node_WaitOverlap"

UK2Node_WaitOverlap::UK2Node_WaitOverlap(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	
	ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(USkillSystemBlueprintLibrary, CreateWaitOverlap);
	ProxyFactoryClass = USkillSystemBlueprintLibrary::StaticClass();
	ProxyClass = UAbilityTask_WaitOverlap::StaticClass();
}

FString UK2Node_WaitOverlap::GetCategoryName()
{
	return TEXT("Ability");
}

FString UK2Node_WaitOverlap::GetTooltip() const
{
	return TEXT("Wait until an overlap occurs");
}

FText UK2Node_WaitOverlap::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("WaitForOverlap", "Wait For Overlap");
}

#undef LOCTEXT_NAMESPACE


