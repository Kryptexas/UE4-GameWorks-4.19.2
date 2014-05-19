// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "BlueprintUtilities.h"
#if WITH_EDITOR
#include "Editor/UnrealEd/Public/Kismet2/BlueprintEditorUtils.h"
#include "Slate.h"
#include "ScopedTransaction.h"
#include "Editor/UnrealEd/Public/Kismet2/Kismet2NameValidators.h"
#endif

#define LOCTEXT_NAMESPACE "EdGraph"

/////////////////////////////////////////////////////
// UEdGraphNode_Comment

UEdGraphNode_Comment::UEdGraphNode_Comment(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	NodeWidth = 400.0f;
	NodeHeight = 100.0f;
	CommentColor = FLinearColor::White;
	bColorCommentBubble = false;
	MoveMode = ECommentBoxMode::GroupMovement;

#if WITH_EDITORONLY_DATA
	bCanResizeNode = true;
	bCanRenameNode = true;
#endif // WITH_EDITORONLY_DATA
}

void UEdGraphNode_Comment::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector) 
{
#if WITH_EDITOR
	UEdGraphNode_Comment* This = CastChecked<UEdGraphNode_Comment>(InThis);
	for (auto It = This->NodesUnderComment.CreateIterator(); It; ++It)
	{
		Collector.AddReferencedObject(*It, This);
	}
#endif // WITH_EDITOR

	Super::AddReferencedObjects(InThis, Collector);
}

#if WITH_EDITOR
void UEdGraphNode_Comment::PostPlacedNewNode()
{
	//@TODO: Consider making the default value we use here a preference
	// This is done here instead of in the constructor so we can later change the default for newly placed
	// instances without changing all of the existing ones (due to delta serialization)
	MoveMode = ECommentBoxMode::GroupMovement;

	NodeComment = NSLOCTEXT("K2Node", "CommentBlock_NewEmptyComment", "Comment").ToString();
}

FString UEdGraphNode_Comment::GetTooltip() const
{
	return FString::Printf(*NSLOCTEXT("K2Node", "CommentBlock_Tooltip", "Comment:\n%s").ToString(), *NodeComment);
}

FString UEdGraphNode_Comment::GetDocumentationLink() const
{
	return TEXT("Shared/GraphNodes/Common");
}

FString UEdGraphNode_Comment::GetDocumentationExcerptName() const
{
	return TEXT("UEdGraphNode_Comment");
}

FText UEdGraphNode_Comment::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if(TitleType == ENodeTitleType::ListView)
	{
		return NSLOCTEXT("K2Node", "CommentBlock_Title", "Comment");
	}

	return FText::FromString(NodeComment);
}

FString UEdGraphNode_Comment::GetPinNameOverride(const UEdGraphPin& Pin) const
{
	return GetNodeTitle(ENodeTitleType::ListView).ToString();
}

FLinearColor UEdGraphNode_Comment::GetNodeCommentColor() const
{
	// Only affects the 'zoomed out' comment bubble color, not the box itself
	return (bColorCommentBubble)
		? CommentColor 
		: FLinearColor::White;
}

void UEdGraphNode_Comment::ResizeNode(const FVector2D& NewSize)
{
	if (bCanResizeNode) 
	{
		NodeHeight = NewSize.Y;
		NodeWidth = NewSize.X;
	}
}

void UEdGraphNode_Comment::AddNodeUnderComment(UObject* Object)
{
	NodesUnderComment.Add(Object);
}

void UEdGraphNode_Comment::ClearNodesUnderComment()
{
	NodesUnderComment.Empty();
}

void UEdGraphNode_Comment::SetBounds(const class FSlateRect& Rect)
{
	NodePosX = Rect.Left;
	NodePosY = Rect.Top;

	FVector2D Size = Rect.GetSize();
	NodeWidth = Size.X;
	NodeHeight = Size.Y;
}

const FCommentNodeSet& UEdGraphNode_Comment::GetNodesUnderComment() const
{
	return NodesUnderComment;
}

void UEdGraphNode_Comment::OnRenameNode(const FString& NewName)
{
	NodeComment = NewName;
}

TSharedPtr<class INameValidatorInterface> UEdGraphNode_Comment::MakeNameValidator() const
{
	// Comments can be duplicated, etc...
	return MakeShareable(new FDummyNameValidator(EValidatorResult::Ok));
}
#endif // WITH_EDITOR

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
