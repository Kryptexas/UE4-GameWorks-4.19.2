// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "AbilitySystemEditorPrivatePCH.h"
#include "K2Node_GameplayCueEvent.h"
#include "CompilerResultsLog.h"
#include "K2ActionMenuBuilder.h"
#include "GameplayTagsModule.h"
#include "GameplayCueInterface.h"
#include "BlueprintEventNodeSpawner.h"
#include "BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "K2Node_GameplayCueEvent"

UK2Node_GameplayCueEvent::UK2Node_GameplayCueEvent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	EventSignatureName = GAMEPLAYABILITIES_BlueprintCustomHandler;
	EventSignatureClass = UGameplayCueInterface::StaticClass();
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

bool UK2Node_GameplayCueEvent::IsCompatibleWithGraph(UEdGraph const* TargetGraph) const
{
	bool bIsCompatible = false;
	if (bIsCompatible)
	{
		if (UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(TargetGraph))
		{
			check(Blueprint->GeneratedClass != nullptr);
			bIsCompatible = Blueprint->GeneratedClass->ImplementsInterface(UGameplayCueInterface::StaticClass());
		}
	}
	
	return bIsCompatible && Super::IsCompatibleWithGraph(TargetGraph);
}

void UK2Node_GameplayCueEvent::GetMenuEntries(FGraphContextMenuBuilder& Context) const
{
	Super::GetMenuEntries(Context);

	if (!IsCompatibleWithGraph(Context.CurrentGraph))
	{
		return;
	}

	const FString FunctionCategory(TEXT("GameplayCue Event"));

	IGameplayTagsModule& GameplayTagsModule = IGameplayTagsModule::Get();
	FGameplayTag RootTag = GameplayTagsModule.GetGameplayTagsManager().RequestGameplayTag(FName(TEXT("GameplayCue")));

	FGameplayTagContainer CueTags = GameplayTagsModule.GetGameplayTagsManager().RequestGameplayTagChildren(RootTag);

	// Fixme: need to check if this function is already defined so that it can be reimplemented
	//	-Checking MyBlueprint->GeneratedClass isn't enough since they may have added an event and not recompiled
	//	-FEdGraphSchemaAction_K2AddCustomEvent does not check names/always ensures a valid name
	//	-FEdGraphSchemaAction_K2AddEvent does check and recenters - but it looks at EventSignatureName/EventSignatureClass for equality and that
	//		won't work here.
	//	
	//	Probably need a new EdGraphSchemaAction to do this properly. For now this is ok since they will get a compile error if they do drop in
	//	two of the same GameplayCue even Nodes and it should be pretty clear that they can't do that.

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

void UK2Node_GameplayCueEvent::GetMenuActions(TArray<UBlueprintNodeSpawner*>& ActionListOut) const
{
	auto CustomizeCueNodeLambda = [](UEdGraphNode* NewNode, bool bIsTemplateNode, FName TagName)
	{
		UK2Node_GameplayCueEvent* EventNode = CastChecked<UK2Node_GameplayCueEvent>(NewNode);
		EventNode->CustomFunctionName = TagName;
	};
	
	IGameplayTagsModule& GameplayTagsModule = IGameplayTagsModule::Get();
	FGameplayTag RootTag = GameplayTagsModule.GetGameplayTagsManager().RequestGameplayTag(FName(TEXT("GameplayCue")));
	
	FGameplayTagContainer CueTags = GameplayTagsModule.GetGameplayTagsManager().RequestGameplayTagChildren(RootTag);
	for (auto TagIt = CueTags.CreateConstIterator(); TagIt; ++TagIt)
	{
		UBlueprintNodeSpawner::FCustomizeNodeDelegate PostSpawnDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(CustomizeCueNodeLambda, TagIt->GetTagName());
		
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintEventNodeSpawner::Create(GetClass(), TagIt->GetTagName());
		check(NodeSpawner != nullptr);
		NodeSpawner->CustomizeNodeDelegate = PostSpawnDelegate;
		
		ActionListOut.Add(NodeSpawner);
	}
}

#undef LOCTEXT_NAMESPACE
