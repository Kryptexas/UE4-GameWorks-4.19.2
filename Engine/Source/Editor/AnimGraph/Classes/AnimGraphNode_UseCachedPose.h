// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimGraphNode_UseCachedPose.generated.h"

UCLASS(MinimalAPI)
class UAnimGraphNode_UseCachedPose : public UAnimGraphNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(EditAnywhere, Category=Settings)
	FAnimNode_UseCachedPose Node;

	UPROPERTY(EditAnywhere, Category=CachedPose)
	FString NameOfCache;

public:
	// UEdGraphNode interface
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	// End of UEdGraphNode interface

	// UAnimGraphNode_Base interface
	virtual FString GetNodeCategory() const OVERRIDE;
	virtual void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const OVERRIDE;
	// End of UAnimGraphNode_Base interface
};
