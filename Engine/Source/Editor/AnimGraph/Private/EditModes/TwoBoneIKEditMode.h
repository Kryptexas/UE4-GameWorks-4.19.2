// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimNodeEditMode.h"
#include "AnimGraphNode_TwoBoneIK.h"

class FTwoBoneIKEditMode : public FAnimNodeEditMode
{
public:
	FTwoBoneIKEditMode();

	/** IAnimNodeEditMode interface */
	virtual void EnterMode(class UAnimGraphNode_Base* InEditorNode, struct FAnimNode_Base* InRuntimeNode) override;
	virtual void ExitMode() override;
	virtual FVector GetWidgetLocation() const override;
	virtual FWidget::EWidgetMode GetWidgetMode() const override;
	virtual FName GetSelectedBone() const override;
	virtual void DoTranslation(FVector& InTranslation) override;

	/** FEdMode interface */
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click) override;
	virtual void Tick(FEditorViewportClient* ViewportClient, float DeltaTime) override;

private:
	/** Helper function for Render() */
	void DrawTargetLocation(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* SkelComp, USkeleton * Skeleton, EBoneControlSpace SpaceBase, FName SpaceBoneName, const FVector& TargetLocation, const FColor& TargetColor, const FColor& BoneColor) const;

	/** Creates and destroys the selection actor as needed */
	void ManageBoneSelectActor();

private:
	/** Cache the typed nodes */
	struct FAnimNode_TwoBoneIK* TwoBoneIKRuntimeNode;
	UAnimGraphNode_TwoBoneIK* TwoBoneIKGraphNode;

	/** Bone selection mode */
	enum BoneSelectModeType
	{
		BSM_EndEffector,
		BSM_JointTarget,
		BSM_Max
	};

	/** The current bone selection mode */
	BoneSelectModeType BoneSelectMode;

	/** */
	TWeakObjectPtr<class ABoneSelectActor> BoneSelectActor;
};