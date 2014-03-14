// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MaterialGraphNode.generated.h"

UCLASS(MinimalAPI)
class UMaterialGraphNode : public UMaterialGraphNode_Base
{
	GENERATED_UCLASS_BODY()

	/** Material Expression this node is representing */
	UPROPERTY()
	class UMaterialExpression* MaterialExpression;

	/** Set to true when Expression Preview compiles, so we can update SGraphNode */
	bool bPreviewNeedsUpdate;

	/** Set to true if this expression causes an error in the material */
	bool bIsErrorExpression;

	/** Set to true if this expression is the current search result */
	bool bIsCurrentSearchResult;

	/** Set to true if this expression is currently being previewed */
	bool bIsPreviewExpression;

	/** Checks if Material Editor is in realtime mode, so we update SGraphNode every frame */
	FRealtimeStateGetter RealtimeDelegate;

	/** Marks the Material Editor as dirty so that user prompted to apply change */
	FSetMaterialDirty MaterialDirtyDelegate;

public:
	/** Fix up the node's owner after being copied */
	UNREALED_API void PostCopyNode();

	/** Get the expression preview for this node */
	UNREALED_API FMaterialRenderProxy* GetExpressionPreview();

	/** Recreate this node's pins and relink the Graph from the Material */
	UNREALED_API void RecreateAndLinkNode();

	/** Get the Material Expression output index from an output pin */
	int32 GetOutputIndex(const UEdGraphPin* OutputPin);
	/** Get the Material value type of an output pin */
	uint32 GetOutputType(const UEdGraphPin* OutputPin);

	// UObject interface
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	virtual void PostEditImport() OVERRIDE;
	virtual void PostDuplicate(bool bDuplicateForPIE) OVERRIDE;
	// End of UObject interface

	// Begin UEdGraphNode interface.
	virtual bool CanPasteHere(const UEdGraph* TargetGraph, const UEdGraphSchema* Schema) const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual void PrepareForCopying() OVERRIDE;
	virtual void GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const OVERRIDE;
	virtual void PostPlacedNewNode() OVERRIDE;
	virtual void OnRenameNode(const FString& NewName) OVERRIDE;
	virtual void GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const OVERRIDE;
	// End UEdGraphNode interface.

	// UMaterialGraphNode_Base interface
	virtual void CreateInputPins() OVERRIDE;
	virtual void CreateOutputPins() OVERRIDE;
	virtual int32 GetInputIndex(const UEdGraphPin* InputPin) const OVERRIDE;
	virtual uint32 GetInputType(const UEdGraphPin* InputPin) const OVERRIDE;
	// End of UMaterialGraphNode_Base interface

private:
	/** Make sure the MaterialExpression is owned by the Material */
	void ResetMaterialExpressionOwner();

	/** Get the parameter name from the Material Expression */
	FString GetParameterName() const;

	/** Sets the Material Expression's parameter name */
	void SetParameterName(const FString& NewName);
};
