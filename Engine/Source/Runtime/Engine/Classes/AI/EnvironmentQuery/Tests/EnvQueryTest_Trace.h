// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvQueryTest_Trace.generated.h"

UENUM()
namespace EEnvTestTrace
{
	enum Type
	{
		Line,
		Box,
		Sphere,
		Capsule,
	};
}

UCLASS(MinimalAPI)
class UEnvQueryTest_Trace : public UEnvQueryTest
{
	GENERATED_UCLASS_BODY()

	/** trace channel */
	UPROPERTY(EditDefaultsOnly, Category=Trace)
	TEnumAsByte<enum ETraceTypeQuery> TraceChannel;

	/** shape used for tracing */
	UPROPERTY(EditDefaultsOnly, Category=Trace)
	TEnumAsByte<EEnvTestTrace::Type> TraceMode;

	/** shape param */
	UPROPERTY(EditDefaultsOnly, Category=Trace)
	FEnvFloatParam TraceExtentX;

	/** shape param */
	UPROPERTY(EditDefaultsOnly, Category=Trace)
	FEnvFloatParam TraceExtentY;

	/** shape param */
	UPROPERTY(EditDefaultsOnly, Category=Trace)
	FEnvFloatParam TraceExtentZ;

	/** test against complex collisions */
	UPROPERTY(EditDefaultsOnly, Category=Trace, AdvancedDisplay)
	FEnvBoolParam TraceComplex;

	/** trace direction */
	UPROPERTY(EditDefaultsOnly, Category=Trace)
	FEnvBoolParam TraceToItem;

	/** Z offset from item */
	UPROPERTY(EditDefaultsOnly, Category=Trace, AdvancedDisplay)
	FEnvFloatParam ItemOffsetZ;

	/** Z offset from querier */
	UPROPERTY(EditDefaultsOnly, Category=Trace, AdvancedDisplay)
	FEnvFloatParam ContextOffsetZ;

	/** context: other end of trace test */
	UPROPERTY(EditDefaultsOnly, Category=Trace)
	TSubclassOf<class UEnvQueryContext> Context;

	void RunTest(struct FEnvQueryInstance& QueryInstance);

	virtual FString GetDescriptionTitle() const OVERRIDE;
	virtual FString GetDescriptionDetails() const OVERRIDE;

protected:

	DECLARE_DELEGATE_RetVal_SevenParams(bool, FRunTraceSignature, const FVector&, const FVector&, AActor*, UWorld*, enum ECollisionChannel, const FCollisionQueryParams&, const FVector&);

	bool RunLineTraceTo(const FVector& ItemPos, const FVector& ContextPos, AActor* ItemActor, UWorld* World, enum ECollisionChannel Channel, const FCollisionQueryParams& Params, const FVector& Extent);
	bool RunLineTraceFrom(const FVector& ItemPos, const FVector& ContextPos, AActor* ItemActor, UWorld* World, enum ECollisionChannel Channel, const FCollisionQueryParams& Params, const FVector& Extent);
	bool RunBoxTraceTo(const FVector& ItemPos, const FVector& ContextPos, AActor* ItemActor, UWorld* World, enum ECollisionChannel Channel, const FCollisionQueryParams& Params, const FVector& Extent);
	bool RunBoxTraceFrom(const FVector& ItemPos, const FVector& ContextPos, AActor* ItemActor, UWorld* World, enum ECollisionChannel Channel, const FCollisionQueryParams& Params, const FVector& Extent);
	bool RunSphereTraceTo(const FVector& ItemPos, const FVector& ContextPos, AActor* ItemActor, UWorld* World, enum ECollisionChannel Channel, const FCollisionQueryParams& Params, const FVector& Extent);
	bool RunSphereTraceFrom(const FVector& ItemPos, const FVector& ContextPos, AActor* ItemActor, UWorld* World, enum ECollisionChannel Channel, const FCollisionQueryParams& Params, const FVector& Extent);
	bool RunCapsuleTraceTo(const FVector& ItemPos, const FVector& ContextPos, AActor* ItemActor, UWorld* World, enum ECollisionChannel Channel, const FCollisionQueryParams& Params, const FVector& Extent);
	bool RunCapsuleTraceFrom(const FVector& ItemPos, const FVector& ContextPos, AActor* ItemActor, UWorld* World, enum ECollisionChannel Channel, const FCollisionQueryParams& Params, const FVector& Extent);
};
