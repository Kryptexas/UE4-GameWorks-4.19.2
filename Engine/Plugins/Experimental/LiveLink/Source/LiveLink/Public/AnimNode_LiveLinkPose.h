// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNodeBase.h"
#include "AnimGraphNode_Base.h"
#include "LiveLinkRetargetAsset.h"

class ILiveLinkClient;

#include "AnimNode_LiveLinkPose.generated.h"

USTRUCT(BlueprintInternalUseOnly)
struct LIVELINK_API FAnimNode_LiveLinkPose : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SourceData, meta = (PinShownByDefault))
	FName SubjectName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Retarget, meta = (PinShownByDefault))
	ULiveLinkRetargetAsset* RetargetAsset;

	UPROPERTY(transient)
	ULiveLinkRetargetAsset* PreviousRetargetAsset;

	TSharedPtr<FLiveLinkRetargetContext> RetargetContext;

	FAnimNode_LiveLinkPose() : LiveLinkClient(nullptr) {}

	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;

	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext & Context) override {}

	virtual void Update_AnyThread(const FAnimationUpdateContext & Context) override;

	virtual void Evaluate_AnyThread(FPoseContext& Output) override;
	// End of FAnimNode_Base interface

	void OnLiveLinkClientRegistered(const FName& Type, class IModularFeature* ModularFeature);
	void OnLiveLinkClientUnregistered(const FName& Type, class IModularFeature* ModularFeature);

private:

	ILiveLinkClient* LiveLinkClient;
};

UCLASS()
class UAnimGraphNode_LiveLinkPose : public UAnimGraphNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = Settings)
	FAnimNode_LiveLinkPose Node;

	// UEdGraphNode interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FText GetMenuCategory() const;
	// End of UEdGraphNode
};