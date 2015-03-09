// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "AIGraph.h"
#include "EnvironmentQueryGraph.generated.h"

UCLASS()
class UEnvironmentQueryGraph : public UAIGraph
{
	GENERATED_UCLASS_BODY()

	virtual void Initialize() override;
	virtual void OnLoaded() override;
	virtual void UpdateVersion() override;
	virtual void MarkVersion() override;
	virtual void UpdateAsset(int32 UpdateFlags = 0) override;

	void UpdateDeprecatedGeneratorClasses();
	void SpawnMissingNodes();
	void CalculateAllWeights();
	void CreateEnvQueryFromGraph(class UEnvironmentQueryGraphNode* RootEdNode);

protected:

	void UpdateVersion_NestedNodes();
	void UpdateVersion_FixupOuters();
	void UpdateVersion_CollectClassData();

	void UpdateNodeClassData(UAIGraphNode* UpdateNode, UClass* InstanceClass);

	virtual void CollectAllNodeInstances(TSet<UObject*>& NodeInstances) override;
};
