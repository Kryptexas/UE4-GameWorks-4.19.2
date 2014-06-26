// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BehaviorTreeEditorPrivatePCH.h"
#include "BehaviorTreeEditorModule.h"
#include "BehaviorTree/BehaviorTree.h"

#define LOCTEXT_NAMESPACE "BehaviorTreeFactory"

UBehaviorTreeFactory::UBehaviorTreeFactory(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	SupportedClass = UBehaviorTree::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

bool UBehaviorTreeFactory::CanCreateNew() const
{
	bool bBehaviorTreeNewAssetsEnabled = false;
	GConfig->GetBool(TEXT("BehaviorTreesEd"), TEXT("BehaviorTreeNewAssetsEnabled"), bBehaviorTreeNewAssetsEnabled, GEngineIni);
	return (bBehaviorTreeNewAssetsEnabled || GetDefault<UEditorExperimentalSettings>()->bBehaviorTreeEditor);
}

UObject* UBehaviorTreeFactory::FactoryCreateNew(UClass* Class,UObject* InParent,FName Name,EObjectFlags Flags,UObject* Context,FFeedbackContext* Warn)
{
	check(Class->IsChildOf(UBehaviorTree::StaticClass()));
	return ConstructObject<UBehaviorTree>(Class, InParent, Name, Flags);;
}

#undef LOCTEXT_NAMESPACE