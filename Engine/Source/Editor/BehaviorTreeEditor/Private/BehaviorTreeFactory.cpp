// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "BehaviorTreeEditorPrivatePCH.h"
#include "BehaviorTreeEditorModule.h"

#define LOCTEXT_NAMESPACE "BehaviorTreeFactory"

UBehaviorTreeFactory::UBehaviorTreeFactory(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	SupportedClass = UBehaviorTree::StaticClass();
	bEditAfterNew = true;

	//enable asset creation by Epic Labs
	bool bCreateNewByEpicLabs = GetDefault<UEditorExperimentalSettings>()->bBehaviorTreeEditor;

	// Check ini to see if we should enable creation
	bool bBehaviorTreeNewAssetsEnabled = false;
	GConfig->GetBool(TEXT("BehaviorTreesEd"), TEXT("BehaviorTreeNewAssetsEnabled"), bBehaviorTreeNewAssetsEnabled, GEngineIni);

	bCreateNew = (bCreateNewByEpicLabs || bBehaviorTreeNewAssetsEnabled);
}

bool UBehaviorTreeFactory::ShouldShowInNewMenu() const
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