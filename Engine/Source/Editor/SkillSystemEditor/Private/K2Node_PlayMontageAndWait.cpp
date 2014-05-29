// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "SkillSystemEditorModulePrivatePCH.h"
#include "SkillSystemBlueprintLibrary.h"
#include "Abilities/Tasks/BlueprintPlayMontageAndWaitTaskProxy.h"
#include "CompilerResultsLog.h"

#define LOCTEXT_NAMESPACE "K2Node_PlayMontageAndWait"

UK2Node_PlayMontageAndWait::UK2Node_PlayMontageAndWait(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	
	ProxyFactoryFunctionName = GET_FUNCTION_NAME_CHECKED(USkillSystemBlueprintLibrary, CreatePlayMontageAndWaitProxy);
	ProxyFactoryClass = USkillSystemBlueprintLibrary::StaticClass();
	ProxyClass = UBlueprintPlayMontageAndWaitTaskProxy::StaticClass();
}

FString UK2Node_PlayMontageAndWait::GetCategoryName()
{
	return TEXT("Ability");
}

FString UK2Node_PlayMontageAndWait::GetTooltip() const
{
	return TEXT("Play Anim Montage until it completes or is interrupted");
}

FText UK2Node_PlayMontageAndWait::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("PlayMontageAndWait", "Play Montage and Wait");
}

#undef LOCTEXT_NAMESPACE


