// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvQueryTest_Pathfinding.generated.h"

UENUM()
namespace EEnvTestPathfinding
{
	enum Type
	{
		PathExist,
		PathCost,
		PathLength,
	};
}

UCLASS()
class UEnvQueryTest_Pathfinding : public UEnvQueryTest
{
	GENERATED_UCLASS_BODY()

	/** testing mode */
	UPROPERTY(EditDefaultsOnly, Category=Pathfinding)
	TEnumAsByte<EEnvTestPathfinding::Type> TestMode;

	/** context: other end of pathfinding test */
	UPROPERTY(EditDefaultsOnly, Category=Pathfinding)
	TSubclassOf<class UEnvQueryContext> Context;

	/** pathfinding direction */
	UPROPERTY(EditDefaultsOnly, Category=Pathfinding)
	FEnvBoolParam PathToItem;

	/** if set, items with failed path will be invalidated (PathCost, PathLength) */
	UPROPERTY(EditDefaultsOnly, Category=Pathfinding, AdvancedDisplay)
	FEnvBoolParam DiscardUnreachable;

	/** if set, hierarchical (faster) pathfinding will be used */
	UPROPERTY(EditDefaultsOnly, Category=Pathfinding, AdvancedDisplay)
	FEnvBoolParam HierarchicalPathfinding;

	void RunTest(struct FEnvQueryInstance& QueryInstance);

	virtual FString GetDescriptionTitle() const OVERRIDE;
	virtual FString GetDescriptionDetails() const OVERRIDE;

#if WITH_EDITOR
	/** update test properties after changing mode */
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent);
#endif

protected:

	DECLARE_DELEGATE_RetVal_FiveParams(bool, FTestPathSignature, const FVector&, const FVector&, EPathFindingMode::Type&, const ANavigationData*, UNavigationSystem*);
	DECLARE_DELEGATE_RetVal_FiveParams(float, FFindPathSignature, const FVector&, const FVector&, EPathFindingMode::Type&, const ANavigationData*, UNavigationSystem*);

	bool TestPathFrom(const FVector& ItemPos, const FVector& ContextPos, EPathFindingMode::Type& Mode, const ANavigationData* NavData, UNavigationSystem* NavSys);
	bool TestPathTo(const FVector& ItemPos, const FVector& ContextPos, EPathFindingMode::Type& Mode, const ANavigationData* NavData, UNavigationSystem* NavSys);
	float FindPathCostFrom(const FVector& ItemPos, const FVector& ContextPos, EPathFindingMode::Type& Mode, const ANavigationData* NavData, UNavigationSystem* NavSys);
	float FindPathCostTo(const FVector& ItemPos, const FVector& ContextPos, EPathFindingMode::Type& Mode, const ANavigationData* NavData, UNavigationSystem* NavSys);
	float FindPathLengthFrom(const FVector& ItemPos, const FVector& ContextPos, EPathFindingMode::Type& Mode, const ANavigationData* NavData, UNavigationSystem* NavSys);
	float FindPathLengthTo(const FVector& ItemPos, const FVector& ContextPos, EPathFindingMode::Type& Mode, const ANavigationData* NavData, UNavigationSystem* NavSys);

	ANavigationData* FindNavigationData(UNavigationSystem* NavSys, UObject* Owner);
};
