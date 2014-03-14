// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "EnvironmentQueryGraphNode_Root.generated.h"

UCLASS()
class UEnvironmentQueryGraphNode_Root : public UEnvironmentQueryGraphNode
{
	GENERATED_UCLASS_BODY()

	virtual void AllocateDefaultPins() OVERRIDE;
	virtual bool CanUserDeleteNode() const OVERRIDE { return false; }
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
};
