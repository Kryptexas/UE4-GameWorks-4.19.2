// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "IntroTutorialsPrivatePCH.h"
#include "K2Node_TutorialExcerptComplete.h"
#include "CompilerResultsLog.h"
#include "Kismet2NameValidators.h"
#include "K2ActionMenuBuilder.h" // for FK2ActionMenuBuilder::AddNewNodeAction()

#define LOCTEXT_NAMESPACE "TutorialExcerptComplete"

FTutorialExcerptComplete UK2Node_TutorialExcerptComplete::OnTutorialExcerptComplete;
TSet<FString> UK2Node_TutorialExcerptComplete::AllBlueprintExcerpts;
bool UK2Node_TutorialExcerptComplete::bExcerptCacheDirty = true;

/**
 * This is a wrapper node to allow us to efficiently track instances of a particular function call in blueprint graphs.
 */
UK2Node_TutorialExcerptComplete::UK2Node_TutorialExcerptComplete(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UK2Node_TutorialExcerptComplete::PostLoad()
{
	Super::PostLoad();

	UEdGraphPin* Pin = FindPin(TEXT("Excerpt"));
	if(Pin != NULL)
	{
		CachedExcerpt = Pin->DefaultValue;
		Pin->bNotConnectable = true;
		bExcerptCacheDirty = true;
	}
}

void UK2Node_TutorialExcerptComplete::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	UK2Node_TutorialExcerptComplete* TemplateNode = ContextMenuBuilder.CreateTemplateNode<UK2Node_TutorialExcerptComplete>();
	TemplateNode->BindFunction();

	const FString Category = LOCTEXT("TutorialExcerptComplete_Category", "Call Function").ToString();
	const FString SubCategory = LOCTEXT("TutorialExcerptComplete_SubCategory", "Tutorials").ToString();
	const FString MenuDesc = LOCTEXT("TutorialExcerptComplete_Desc", "Tutorial Excerpt Complete").ToString();
	const FString Tooltip = LOCTEXT("TutorialExcerptComplete_Tooltip", "Add a node to complete a tutorial excerpt").ToString();

	TSharedPtr<FEdGraphSchemaAction_K2NewNode> NodeAction = FK2ActionMenuBuilder::AddNewNodeAction(ContextMenuBuilder, Category + TEXT("|") + SubCategory, MenuDesc, Tooltip);
	NodeAction->NodeTemplate = TemplateNode;
}

void UK2Node_TutorialExcerptComplete::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);

	if(Pin->PinName == TEXT("Excerpt"))
	{
		if(CachedExcerpt != Pin->DefaultValue)
		{
			bExcerptCacheDirty = true;
		}
		CachedExcerpt = Pin->DefaultValue;
	}
}

void UK2Node_TutorialExcerptComplete::PostParameterPinCreated(UEdGraphPin *Pin)
{
	Super::PostParameterPinCreated(Pin);

	// we don't want our excerpts to be modified on the fly, so we only allow default parameters here
// 	if(Pin->PinName == TEXT("Excerpt"))
// 	{
// 		Pin->bNotConnectable = true;
// 	}
}

void UK2Node_TutorialExcerptComplete::ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const
{
	Super::ValidateNodeDuringCompilation(MessageLog);

	UEdGraphPin* Pin = FindPin(TEXT("Excerpt"));
	if(Pin != NULL && Pin->DefaultValue.Len() == 0)
	{
		MessageLog.Warning(*LOCTEXT("NoExcerptName", "No excerpt name specified for @@").ToString(), this);
	}
}

void UK2Node_TutorialExcerptComplete::PostPlacedNewNode()
{
	BindFunction();

	Super::PostPlacedNewNode();
}

FString UK2Node_TutorialExcerptComplete::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if(CachedExcerpt.Len() > 0)
	{
		FFormatNamedArguments NamedArgs;
		NamedArgs.Add(TEXT("ExcerptName"), FText::FromString(CachedExcerpt));
		return FText::Format(LOCTEXT("TutorialExcerptComplete_TitleWithExcerpt", "Tutorial Excerpt '{ExcerptName}' Complete"), NamedArgs).ToString();
	}
	else
	{
		return LOCTEXT("TutorialExcerptComplete_Title", "Tutorial Excerpt Complete").ToString();
	}
}

void UK2Node_TutorialExcerptComplete::TutorialExcerptComplete(const FString& Excerpt)
{
	OnTutorialExcerptComplete.Broadcast(Excerpt);
}

bool UK2Node_TutorialExcerptComplete::IsExcerptInteractive(const FString& InExerptName)
{
	// check to see if we need to rebuild the set of blueprint-interactive excerpts
	if(bExcerptCacheDirty)
	{
		AllBlueprintExcerpts.Empty();
		for(TObjectIterator<UK2Node_TutorialExcerptComplete> It; It; ++It)
		{
			const FString& CachedExcerpt = It->CachedExcerpt;
			if(CachedExcerpt.Len() > 0)
			{
				AllBlueprintExcerpts.Add(CachedExcerpt);
			}
		}
		bExcerptCacheDirty = false;
	}

	return AllBlueprintExcerpts.Find(InExerptName) != NULL;
}

void UK2Node_TutorialExcerptComplete::BindFunction()
{
	// link up to the function that actually performs the operation
	UFunction* Function = FindField<UFunction>(UK2Node_TutorialExcerptComplete::StaticClass(), TEXT("TutorialExcerptComplete"));
	check(Function != NULL);
	SetFromFunction(Function);
}

#undef LOCTEXT_NAMESPACE
