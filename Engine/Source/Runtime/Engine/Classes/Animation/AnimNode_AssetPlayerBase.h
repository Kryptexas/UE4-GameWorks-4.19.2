// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimNodeBase.h"
#include "AnimNode_AssetPlayerBase.generated.h"

/* Base class for any asset playing anim node */
USTRUCT()
struct ENGINE_API FAnimNode_AssetPlayerBase : public FAnimNode_Base
{
	GENERATED_BODY();

	/** If true, "Relevant anim" nodes that look for the highest weighted animation in a state will ignore
	 *  this node
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Relevancy, meta=(PinHiddenByDefault))
	bool bIgnoreForRelevancyTest;

	FAnimNode_AssetPlayerBase();

	/** Get the last encountered blend weight for this node */
	virtual float GetCachedBlendWeight();
	
	/** Get the currently referenced time within the asset player node */
	virtual float GetAccumulatedTime();

	/** Override the currently accumulated time */
	virtual void SetAccumulatedTime(const float& NewTime);

	/** Get the animation asset associated with the node, derived classes should implement this */
	virtual UAnimationAsset* GetAnimAsset();

	/** Update the node, marked final so we can always handle blendweight caching.
	 *  Derived classes should implement UpdateAssetPlayer
	 */
	virtual void Update(const FAnimationUpdateContext& Context) final override;

	/** Update method for the asset player, to be implemented by derived classes */
	virtual void UpdateAssetPlayer(const FAnimationUpdateContext& Context) {};

protected:

	/** Last encountered blendweight for this node */
	UPROPERTY(BlueprintReadWrite, Transient, Category=DoNotEdit)
	float BlendWeight;

	/** Accumulated time used to reference the asset in this node */
	UPROPERTY(BlueprintReadWrite, Transient, Category=DoNotEdit)
	float InternalTimeAccumulator;
};