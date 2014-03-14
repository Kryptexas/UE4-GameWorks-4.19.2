// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "EnvironmentQueryGraph.generated.h"

UCLASS()
class UEnvironmentQueryGraph : public UEdGraph
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	int32 GraphVersion;

	void UpdateVersion();
	void MarkVersion();

	void UpdateAsset();
	void CreateEnvQueryFromGraph(class UEnvironmentQueryGraphNode* RootEdNode);

protected:

	void UpdateVersion_NestedNodes();
};

