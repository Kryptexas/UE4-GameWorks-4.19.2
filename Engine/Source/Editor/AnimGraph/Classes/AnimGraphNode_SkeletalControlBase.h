// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimGraphNode_Base.h"
#include "AnimGraphNode_SkeletalControlBase.generated.h"

UCLASS(Abstract, MinimalAPI)
class UAnimGraphNode_SkeletalControlBase : public UAnimGraphNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	// UEdGraphNode interface
	ANIMGRAPH_API virtual FLinearColor GetNodeTitleColor() const override;
	ANIMGRAPH_API virtual FString GetTooltip() const override;
	// End of UEdGraphNode interface

	// UAnimGraphNode_Base interface
	ANIMGRAPH_API virtual FString GetNodeCategory() const override;
	ANIMGRAPH_API virtual void CreateOutputPins() override;
	ANIMGRAPH_API virtual void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
	// End of UAnimGraphNode_Base interface

	// Draw function for supporting visualization
	ANIMGRAPH_API virtual void Draw( FPrimitiveDrawInterface* PDI, USkeletalMeshComponent * PreviewSkelMeshComp ) const {};
protected:
	// Returns the short descriptive name of the controller
	virtual FText GetControllerDescription() const;
};
