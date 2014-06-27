// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BehaviorTreeEditorPrivatePCH.h"
#include "BehaviorTree/BlackboardData.h"
#include "BlackboardDataFactory.h"

#define LOCTEXT_NAMESPACE "BehaviorTreeFactory"

UBlackboardDataFactory::UBlackboardDataFactory(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	SupportedClass = UBlackboardData::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

bool UBlackboardDataFactory::CanCreateNew() const
{
	bool bBehaviorTreeNewAssetsEnabled = false;
	GConfig->GetBool(TEXT("BehaviorTreesEd"), TEXT("BehaviorTreeNewAssetsEnabled"), bBehaviorTreeNewAssetsEnabled, GEngineIni);
	return (bBehaviorTreeNewAssetsEnabled || GetDefault<UEditorExperimentalSettings>()->bBehaviorTreeEditor);
}

UObject* UBlackboardDataFactory::FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn)
{
	check(Class->IsChildOf(UBlackboardData::StaticClass()));
	return ConstructObject<UBlackboardData>(Class, InParent, Name, Flags);
}

#undef LOCTEXT_NAMESPACE