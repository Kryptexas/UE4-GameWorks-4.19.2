// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimGraphNode_BlendSpaceBase.generated.h"

UCLASS(Abstract, MinimalAPI)
class UAnimGraphNode_BlendSpaceBase : public UAnimGraphNode_Base
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode interface
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	// End of UEdGraphNode interface

	// UAnimGraphNode_Base interface
	virtual void CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const OVERRIDE;
	virtual void PreloadRequiredAssets() OVERRIDE;
	// End of UAnimGraphNode_Base interface

protected:
	static void GetBlendSpaceEntries(bool bWantAimOffsets, FGraphContextMenuBuilder& ContextMenuBuilder);

	UBlendSpaceBase* GetBlendSpace() const { return Cast<UBlendSpaceBase>(GetAnimationAsset()); }
};
