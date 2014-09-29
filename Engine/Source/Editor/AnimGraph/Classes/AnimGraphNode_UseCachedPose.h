// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimGraphNode_Base.h"
#include "Animation/AnimNode_UseCachedPose.h"
#include "EdGraph/EdGraphNodeUtils.h" // for FNodeTextCache
#include "AnimGraphNode_UseCachedPose.generated.h"

UCLASS(MinimalAPI)
class UAnimGraphNode_UseCachedPose : public UAnimGraphNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY()
	FAnimNode_UseCachedPose Node;

	UPROPERTY()
	TWeakObjectPtr<UAnimGraphNode_SaveCachedPose> SaveCachedPoseNode;

public:
	// Begin UObject interface
	virtual void PostLoad() override;
	// End UObject interface

	// UEdGraphNode interface
	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	// End of UEdGraphNode interface

	// UK2Node interface.
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual bool IsActionFilteredOut(class FBlueprintActionFilter const& Filter) override;
	// End of UK2Node interface

	// UAnimGraphNode_Base interface
	virtual FString GetNodeCategory() const override;
	virtual void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
	// End of UAnimGraphNode_Base interface

private:
	UPROPERTY()
	mutable FString NameOfCache;
};
