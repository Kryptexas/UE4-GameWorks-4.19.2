// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "AbilitySystemEditorPrivatePCH.h"
#include "K2Node_GameplayCueEvent.h"
#include "CompilerResultsLog.h"
#include "K2ActionMenuBuilder.h"
#include "GameplayTagsModule.h"
#include "GameplayCueInterface.h"

#define LOCTEXT_NAMESPACE "K2Node_PlayMontageAndWait"

UK2Node_GameplayCueEvent::UK2Node_GameplayCueEvent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	EventSignatureName = GAMEPLAYABILITIES_BlueprintCustomHandler;
	EventSignatureClass = UGameplayCueInterface::StaticClass();
}

FString UK2Node_GameplayCueEvent::GetCategoryName()
{
	return TEXT("GameplayCueEvent");
}

FString UK2Node_GameplayCueEvent::GetTooltip() const
{
	return TEXT("Handle GameplayCue Event");
}

FText UK2Node_GameplayCueEvent::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromName(CustomFunctionName);
	//return LOCTEXT("HandleGameplayCueEvent", "HandleGameplaCueEvent");
}

void UK2Node_GameplayCueEvent::GetMenuEntries(FGraphContextMenuBuilder& Context) const
{
	Super::GetMenuEntries(Context);

	const FString FunctionCategory(TEXT("GameplayCue Event"));

	IGameplayTagsModule& GameplayTagsModule = IGameplayTagsModule::Get();
	FGameplayTag RootTag = GameplayTagsModule.GetGameplayTagsManager().RequestGameplayTag(FName(TEXT("GameplayCue")));

	FGameplayTagContainer CueTags = GameplayTagsModule.GetGameplayTagsManager().RequestGameplayTagChildren(RootTag);

	for (auto It = CueTags.CreateConstIterator(); It; ++It)
	{
		FGameplayTag Tag = *It;

		UK2Node_GameplayCueEvent* NodeTemplate = Context.CreateTemplateNode<UK2Node_GameplayCueEvent>();

		NodeTemplate->CustomFunctionName = Tag.GetTagName();

		const FString Category = FunctionCategory;
		const FText MenuDesc = NodeTemplate->GetNodeTitle(ENodeTitleType::ListView);
		const FString Tooltip = NodeTemplate->GetTooltip();
		const FString Keywords = NodeTemplate->GetKeywords();

		TSharedPtr<FEdGraphSchemaAction_K2NewNode> NodeAction = FK2ActionMenuBuilder::AddNewNodeAction(Context, Category, MenuDesc, Tooltip, 0, Keywords);
		NodeAction->NodeTemplate = NodeTemplate;
	}	
}

#undef LOCTEXT_NAMESPACE