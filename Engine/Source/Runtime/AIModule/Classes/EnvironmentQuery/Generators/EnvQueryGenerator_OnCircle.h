// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvironmentQuery/Generators/EnvQueryGenerator_ProjectedPoints.h"
#include "EnvQueryGenerator_OnCircle.generated.h"

struct FEnvQueryInstance;

class FVectorAndDataContainer
{
public:
	virtual ~FVectorAndDataContainer() {}

	virtual void Reset() { Locations.Empty(); }

	FORCEINLINE int32 GetNumEntries() const { return Locations.Num(); }
	FORCEINLINE FVector GetVector(int32 Index) const { return Locations[Index]; }

	FORCEINLINE TArray<FVector>& GetLocationArray() { return Locations; }

protected:
	// TODO: Make inline non-virtual and turn this into the base class?  If so, we wouldn't have to allocate this class
	// We'd only have to fill in a parallel array if/when needed.
	
	// virtual FVectorAdditionalData& GetAdditionalData(int32 Index);

private:
	TArray<FVector> Locations;
};

UCLASS()	
class AIMODULE_API UEnvQueryGenerator_OnCircle : public UEnvQueryGenerator_ProjectedPoints
{
	GENERATED_UCLASS_BODY()

	/** max distance of path between point and context */
	UPROPERTY(EditDefaultsOnly, Category=Generator)
	FEnvFloatParam Radius;

	/** items will be generated on a circle this much apart */
	UPROPERTY(EditDefaultsOnly, Category=Generator)
	FEnvFloatParam ItemSpacing;

	/** If you generate items on a piece of circle you define direction of Arc cut here */
	UPROPERTY(EditDefaultsOnly, Category=Generator, meta=(EditCondition="bDefineArc"))
	FEnvDirection ArcDirection;

	/** If you generate items on a piece of circle you define angle of Arc cut here */
	UPROPERTY(EditDefaultsOnly, Category=Generator)
	FEnvFloatParam Angle;
	
	UPROPERTY()
	mutable float AngleRadians;

	/** context */
	UPROPERTY(EditAnywhere, Category=Generator)
	TSubclassOf<class UEnvQueryContext> CircleCenter;

	/** horizontal trace for nearest obstacle */
	UPROPERTY(EditAnywhere, Category=Generator)
	FEnvTraceData TraceData;

	UPROPERTY()
	uint32 bDefineArc:1;

	virtual void PostLoad() override;

	virtual void GenerateItems(FEnvQueryInstance& QueryInstance) const override;

	virtual FText GetDescriptionTitle() const override;
	virtual FText GetDescriptionDetails() const override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

protected:
	FVector CalcDirection(FEnvQueryInstance& QueryInstance) const;

	// For derived classes that need to store additional data associated with the FVector, override these two functions:
	virtual void PrepareGeneratorContext(FEnvQueryInstance& QueryInstance,
										 TSharedPtr<FVectorAndDataContainer>& CenterLocationCandidates) const;

	virtual void AddItemDataForCircle(const TArray<FVector>& ItemCandidates,
									  TSharedPtr<FVectorAndDataContainer>& CenterLocationCandidates,
									  int32 CenterLocationCandidateIndex,
									  FEnvQueryInstance& OutQueryInstance) const;

private:
	void GenerateItemsForCircle(TSharedPtr<FVectorAndDataContainer>& CenterLocationCandidates,
								int32 CenterLocationCandidateIndex,
								const FVector& CenterLocation,
								const FVector& StartDirection,
								int32 StepsCount, float AngleStep,
								FEnvQueryInstance& OutQueryInstance) const;
};
