// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EdGraphNode_Comment.generated.h"

typedef TArray<class UObject*> FCommentNodeSet;

UENUM()
namespace ECommentBoxMode
{
	enum Type
	{
		// This comment box will move any fully contained nodes when it moves
		GroupMovement UMETA(DisplayName="Group Movement"),

		// This comment box has no effect on nodes contained inside it
		NoGroupMovement UMETA(DisplayName="Comment")
	};
}

UCLASS(MinimalAPI)
class UEdGraphNode_Comment : public UEdGraphNode
{
	GENERATED_UCLASS_BODY()

	/** Color to style comment with */
	UPROPERTY(EditAnywhere, Category=Comment)
	FLinearColor CommentColor;

	/** Whether to use Comment Color to color the background of the comment bubble shown when zoomed out. */
	UPROPERTY(EditAnywhere, Category=Comment, meta=(FriendlyName="Color Bubble"))
	uint32 bColorCommentBubble:1;

	/** Whether the comment should move any fully enclosed nodes around when it is moved */
	UPROPERTY(EditAnywhere, Category=Comment)
	TEnumAsByte<ECommentBoxMode::Type> MoveMode;

public:

	// Begin UObject Interface
	ENGINE_API static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	// End UObject Interface

#if WITH_EDITOR
	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE {}
	ENGINE_API virtual FString GetTooltip() const OVERRIDE;
	ENGINE_API virtual FLinearColor GetNodeCommentColor() const OVERRIDE;
	ENGINE_API virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual bool ShouldOverridePinNames() const OVERRIDE { return true; }
	ENGINE_API virtual FString GetPinNameOverride(const UEdGraphPin& Pin) const OVERRIDE;
	ENGINE_API virtual void ResizeNode(const FVector2D& NewSize) OVERRIDE;
	ENGINE_API virtual void PostPlacedNewNode() OVERRIDE;
	ENGINE_API virtual void OnRenameNode(const FString& NewName) OVERRIDE;
	ENGINE_API virtual TSharedPtr<class INameValidatorInterface> MakeNameValidator() const OVERRIDE;
	virtual bool ShouldDrawNodeAsComment() const OVERRIDE { return true; }
	ENGINE_API virtual FString GetDocumentationLink() const OVERRIDE;
	ENGINE_API virtual FString GetDocumentationExcerptName() const OVERRIDE;
	// End UEdGraphNode interface

	/** Add a node that will be dragged when this comment is dragged */
	ENGINE_API void	AddNodeUnderComment(UObject* Object);

	/** Clear all of the nodes which are currently being dragged with this comment */
	ENGINE_API void	ClearNodesUnderComment();

	/** Set the Bounds for the comment node */
	ENGINE_API void SetBounds(const class FSlateRect& Rect);

	/** Return the set of nodes underneath the comment */
	ENGINE_API const FCommentNodeSet& GetNodesUnderComment() const;

private:
	/** Nodes currently within the region of the comment */
	FCommentNodeSet	NodesUnderComment;
#endif
};

