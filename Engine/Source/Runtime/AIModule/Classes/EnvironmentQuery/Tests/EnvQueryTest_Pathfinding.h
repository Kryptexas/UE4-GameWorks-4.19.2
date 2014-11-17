// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvironmentQuery/EnvQueryTest.h"
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

	virtual FString GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;

#if WITH_EDITOR
	/** update test properties after changing mode */
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent);
#endif

protected:

	DECLARE_DELEGATE_RetVal_SixParams(bool, FTestPathSignature, const FVector&, const FVector&, EPathFindingMode::Type&, const ANavigationData*, UNavigationSystem*, const UObject*);
	DECLARE_DELEGATE_RetVal_SixParams(float, FFindPathSignature, const FVector&, const FVector&, EPathFindingMode::Type&, const ANavigationData*, UNavigationSystem*, const UObject*);

	bool TestPathFrom(const FVector& ItemPos, const FVector& ContextPos, EPathFindingMode::Type& Mode, const ANavigationData* NavData, UNavigationSystem* NavSys, const UObject* PathOwner);
	bool TestPathTo(const FVector& ItemPos, const FVector& ContextPos, EPathFindingMode::Type& Mode, const ANavigationData* NavData, UNavigationSystem* NavSys, const UObject* PathOwner);
	float FindPathCostFrom(const FVector& ItemPos, const FVector& ContextPos, EPathFindingMode::Type& Mode, const ANavigationData* NavData, UNavigationSystem* NavSys, const UObject* PathOwner);
	float FindPathCostTo(const FVector& ItemPos, const FVector& ContextPos, EPathFindingMode::Type& Mode, const ANavigationData* NavData, UNavigationSystem* NavSys, const UObject* PathOwner);
	float FindPathLengthFrom(const FVector& ItemPos, const FVector& ContextPos, EPathFindingMode::Type& Mode, const ANavigationData* NavData, UNavigationSystem* NavSys, const UObject* PathOwner);
	float FindPathLengthTo(const FVector& ItemPos, const FVector& ContextPos, EPathFindingMode::Type& Mode, const ANavigationData* NavData, UNavigationSystem* NavSys, const UObject* PathOwner);

	ANavigationData* FindNavigationData(UNavigationSystem* NavSys, UObject* Owner);
};
