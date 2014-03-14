// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SoundCueGraphNode.generated.h"

UCLASS(MinimalAPI)
class USoundCueGraphNode : public USoundCueGraphNode_Base
{
	GENERATED_UCLASS_BODY()

	/** The SoundNode this represents */
	UPROPERTY(VisibleAnywhere, instanced, Category=Sound)
	USoundNode* SoundNode;

	/** Set the SoundNode this represents (also assigns this to the SoundNode in Editor)*/
	UNREALED_API void SetSoundNode(USoundNode* InSoundNode);
	/** Fix up the node's owner after being copied */
	UNREALED_API void PostCopyNode();
	/** Add an input pin to this node */
	UNREALED_API void AddInputPin();
	/** Remove a specific input pin from this node */
	UNREALED_API void RemoveInputPin(UEdGraphPin* InGraphPin);
	/** Estimate the width of this Node from the length of its title */
	UNREALED_API int32 EstimateNodeWidth() const;

	// USoundCueGraphNode_Base interface
	virtual void CreateInputPins() OVERRIDE;
	// End of USoundCueGraphNode_Base interface

	// UEdGraphNode interface
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual void PrepareForCopying() OVERRIDE;
	virtual void GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetDocumentationExcerptName() const OVERRIDE;
	// End of UEdGraphNode interface

	// UObject interface
	virtual void PostLoad() OVERRIDE;
	virtual void PostEditImport() OVERRIDE;
	virtual void PostDuplicate(bool bDuplicateForPIE) OVERRIDE;
	// End of UObject interface

private:
	/** Make sure the soundnode is owned by the SoundCue */
	void ResetSoundNodeOwner();
};
