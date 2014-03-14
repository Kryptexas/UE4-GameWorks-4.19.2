// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MaterialGraphNode_Comment.generated.h"

UCLASS(MinimalAPI)
class UMaterialGraphNode_Comment : public UEdGraphNode_Comment
{
	GENERATED_UCLASS_BODY()

	/** Material Comment that this node represents */
	UPROPERTY()
	class UMaterialExpressionComment* MaterialExpressionComment;

	/** Marks the Material Editor as dirty so that user prompted to apply change */
	FSetMaterialDirty MaterialDirtyDelegate;

	/** Fix up the node's owner after being copied */
	UNREALED_API void PostCopyNode();

	// UObject interface
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	virtual void PostEditImport() OVERRIDE;
	virtual void PostDuplicate(bool bDuplicateForPIE) OVERRIDE;
	// End of UObject interface

	// Begin UEdGraphNode interface.
	virtual void PrepareForCopying() OVERRIDE;
	virtual void GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const OVERRIDE;
	virtual bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const OVERRIDE;
	virtual void PostPlacedNewNode() OVERRIDE;
	virtual void OnRenameNode(const FString& NewName) OVERRIDE;
	virtual void ResizeNode(const FVector2D& NewSize) OVERRIDE;
	// End UEdGraphNode interface.

private:
	/** Make sure the MaterialExpressionComment is owned by the Material */
	void ResetMaterialExpressionOwner();
};
