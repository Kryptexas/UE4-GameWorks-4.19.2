// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimGraphNode_RefPoseBase.h"
#include "AnimGraphNode_IdentityPose.generated.h"

UCLASS(MinimalAPI)
class UAnimGraphNode_IdentityPose : public UAnimGraphNode_RefPoseBase
{
	GENERATED_BODY()
public:
	ANIMGRAPH_API UAnimGraphNode_IdentityPose(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// UEdGraphNode interface
	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	// End of UEdGraphNode interface
};
