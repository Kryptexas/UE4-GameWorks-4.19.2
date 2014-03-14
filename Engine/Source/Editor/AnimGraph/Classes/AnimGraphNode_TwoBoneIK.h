// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimGraphNode_TwoBoneIK.generated.h"

UCLASS(MinimalAPI)
class UAnimGraphNode_TwoBoneIK : public UAnimGraphNode_SkeletalControlBase
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Settings)
	FAnimNode_TwoBoneIK Node;

public:
	// UEdGraphNode interface
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	// End of UEdGraphNode interface

	// UAnimGraphNode_SkeletalControlBase interface
	ANIMGRAPH_API virtual void Draw( FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* SkelMeshComp) const OVERRIDE;
	// End of UAnimGraphNode_SkeletalControlBase interface

protected:
	// UAnimGraphNode_SkeletalControlBase interface
	virtual FString GetControllerDescription() const;
	// End of UAnimGraphNode_SkeletalControlBase interface

	// local conversion function for drawing
	void ConvertToComponentSpaceTransform(USkeletalMeshComponent* SkelComp, USkeleton * Skeleton, const FTransform & InTransform, FTransform & OutCSTransform, int32 BoneIndex, uint8 Space) const;
	void DrawTargetLocation(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* SkelComp, USkeleton * Skeleton, uint8 SpaceBase, FName SpaceBoneName, const FVector & TargetLocation, const FColor & TargetColor, const FColor & BoneColor) const;
};
