// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimGraphNode_SkeletalControlBase.h"
#include "Animation/BoneControllers/AnimNode_LookAt.h"
#include "AnimGraphNode_LookAt.generated.h"

UCLASS(MinimalAPI)
class UAnimGraphNode_LookAt : public UAnimGraphNode_SkeletalControlBase
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Settings)
	FAnimNode_LookAt Node;

public:
	// UEdGraphNode interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FString GetTooltip() const override;
	virtual FString GetKeywords() const override;
	// End of UEdGraphNode interface

	// UAnimGraphNode_SkeletalControlBase interface
	ANIMGRAPH_API virtual void Draw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* SkelMeshComp) const override;
	// End of UAnimGraphNode_SkeletalControlBase interface

protected:
	// UAnimGraphNode_SkeletalControlBase interface
	virtual FText GetControllerDescription() const override;
	// End of UAnimGraphNode_SkeletalControlBase interface
};
