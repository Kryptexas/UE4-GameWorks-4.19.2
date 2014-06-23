// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

#include "GraphEditorActions.h"
#include "ScopedTransaction.h"
#include "Kismet2NameValidators.h"
#include "K2ActionMenuBuilder.h" // for FK2ActionMenuBuilder::AddNewNodeAction()
#include "AnimGraphNode_SaveCachedPose.h"

/////////////////////////////////////////////////////
// FCachedPoseNameValidator

class FCachedPoseNameValidator : public FStringSetNameValidator
{
public:
	FCachedPoseNameValidator(class UBlueprint* InBlueprint, const FString& InExistingName)
		: FStringSetNameValidator(InExistingName)
	{
		TArray<UAnimGraphNode_SaveCachedPose*> Nodes;
		FBlueprintEditorUtils::GetAllNodesOfClass<UAnimGraphNode_SaveCachedPose>(InBlueprint, Nodes);

		for (auto Node : Nodes)
		{
			Names.Add(Node->CacheName);
		}
	}
};

/////////////////////////////////////////////////////
// UAnimGraphNode_ComponentToLocalSpace

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_SaveCachedPose::UAnimGraphNode_SaveCachedPose(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bCanRenameNode = true;
	CacheName = TEXT("SavedPose");
}

FString UAnimGraphNode_SaveCachedPose::GetTooltip() const
{
	return TEXT("Denotes an animation tree that can be referenced elsewhere in the blueprint, which will be evaluated at most once per frame and then cached.");
}

FText UAnimGraphNode_SaveCachedPose::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (TitleType == ENodeTitleType::EditableTitle)
	{
		return FText::FromString(CacheName);
	}
	else
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("NodeTitle"), FText::FromString(CacheName));
		return FText::Format(LOCTEXT("AnimGraphNode_SaveCachedPose_Title", "Save cached pose '{NodeTitle}'"), Args);
	}
}

FString UAnimGraphNode_SaveCachedPose::GetNodeCategory() const
{
	return TEXT("Cached Poses");
}

void UAnimGraphNode_SaveCachedPose::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	//@TODO: Check the type of the from pin to make sure it's a pose
	if ((ContextMenuBuilder.FromPin == NULL) || (ContextMenuBuilder.FromPin->Direction == EGPD_Output))
	{
		// Disallow the graph being cached from residing in a state for safety and sanity; the tree is in effect it's own graph entirely and should be treated that way
		const bool bNotInStateMachine = ContextMenuBuilder.CurrentGraph->GetOuter()->IsA(UAnimBlueprint::StaticClass());

		if (bNotInStateMachine)
		{
			// Offer save cached pose
			UAnimGraphNode_SaveCachedPose* SaveCachedPose = NewObject<UAnimGraphNode_SaveCachedPose>();
			SaveCachedPose->CacheName += FString::FromInt(FMath::Rand());

			TSharedPtr<FEdGraphSchemaAction_K2NewNode> SaveCachedPoseAction = FK2ActionMenuBuilder::AddNewNodeAction(ContextMenuBuilder, GetNodeCategory(), LOCTEXT("NewSaveCachedPose", "New Save cached pose..."), SaveCachedPose->GetTooltip(), 0, SaveCachedPose->GetKeywords());
			SaveCachedPoseAction->NodeTemplate = SaveCachedPose;
		}
	}
}

void UAnimGraphNode_SaveCachedPose::OnRenameNode(const FString& NewName)
{
	CacheName = NewName;
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(GetBlueprint());
}

TSharedPtr<class INameValidatorInterface> UAnimGraphNode_SaveCachedPose::MakeNameValidator() const
{
	return MakeShareable(new FCachedPoseNameValidator(GetBlueprint(), CacheName));
}

#undef LOCTEXT_NAMESPACE