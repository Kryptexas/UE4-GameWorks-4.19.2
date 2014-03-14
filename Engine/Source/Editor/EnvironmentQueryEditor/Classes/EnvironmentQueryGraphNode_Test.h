// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvironmentQueryGraphNode_Test.generated.h"

UCLASS()
class UEnvironmentQueryGraphNode_Test : public UEnvironmentQueryGraphNode
{
	GENERATED_UCLASS_BODY()

	/** weight percent for display */
	UPROPERTY()
	float TestWeightPct;

	/** weight is passed as named param */
	UPROPERTY()
	uint32 bHasNamedWeight : 1;

	/** toggles test */
	UPROPERTY()
	uint32 bTestEnabled : 1;

	UPROPERTY()
	class UEnvironmentQueryGraphNode_Option* ParentNode;

	virtual void PostPlacedNewNode() OVERRIDE;
	virtual void DestroyNode() OVERRIDE;
	virtual bool IsSubNode() const OVERRIDE { return true; }
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FString GetDescription() const OVERRIDE;

	void SetDisplayedWeight(float Pct, bool bNamed);
};
