// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemEditorPrivatePCH.h"
#include "GameplayAbilitiesEditor.h"

#include "GameplayAbilityBlueprint.h"

#define LOCTEXT_NAMESPACE "FGameplayAbilitiesEditor"


/////////////////////////////////////////////////////
// FGameplayAbilitiesEditor

FGameplayAbilitiesEditor::FGameplayAbilitiesEditor()
{
	// todo: Do we need to register a callback for when properties are changed?
}

FGameplayAbilitiesEditor::~FGameplayAbilitiesEditor()
{
	FEditorDelegates::OnAssetPostImport.RemoveAll(this);
	FReimportManager::Instance()->OnPostReimport().RemoveAll(this);
	
	// NOTE: Any tabs that we still have hanging out when destroyed will be cleaned up by FBaseToolkit's destructor
}

void FGameplayAbilitiesEditor::InitGameplayAbilitiesEditor(const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, const TArray<UBlueprint*>& InBlueprints, bool bShouldOpenInDefaultsMode)
{
	InitBlueprintEditor(Mode, InitToolkitHost, InBlueprints, bShouldOpenInDefaultsMode);
}

FName FGameplayAbilitiesEditor::GetToolkitFName() const
{
	return FName("GameplayAbilitiesEditor");
}

FText FGameplayAbilitiesEditor::GetBaseToolkitName() const
{
	return LOCTEXT("GameplayAbilitiesEditorAppLabel", "Gameplay Abilities Editor");
}

FText FGameplayAbilitiesEditor::GetToolkitName() const
{
	const auto EditingObjects = GetEditingObjects();

	check(EditingObjects.Num() > 0);

	FFormatNamedArguments Args;

	const UObject* EditingObject = EditingObjects[0];

	const bool bDirtyState = EditingObject->GetOutermost()->IsDirty();

	Args.Add(TEXT("ObjectName"), FText::FromString(EditingObject->GetName()));
	Args.Add(TEXT("DirtyState"), bDirtyState ? FText::FromString(TEXT("*")) : FText::GetEmpty());
	return FText::Format(LOCTEXT("GameplayAbilitiesEditorAppLabel", "{ObjectName}{DirtyState}"), Args);
}

FString FGameplayAbilitiesEditor::GetWorldCentricTabPrefix() const
{
	return TEXT("GameplayAbilitiesEditor");
}

FLinearColor FGameplayAbilitiesEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor::White;
}

UBlueprint* FGameplayAbilitiesEditor::GetBlueprintObj() const
{
	auto EditingObjects = GetEditingObjects();
	for (int32 i = 0; i < EditingObjects.Num(); ++i)
	{
		if (EditingObjects[i]->IsA<UGameplayAbilityBlueprint>()) { return (UBlueprint*)EditingObjects[i]; }
	}
	return NULL;
}

FString FGameplayAbilitiesEditor::GetDocumentationLink() const
{
	return FBlueprintEditor::GetDocumentationLink(); // todo: point this at the correct documentation
}

#undef LOCTEXT_NAMESPACE

