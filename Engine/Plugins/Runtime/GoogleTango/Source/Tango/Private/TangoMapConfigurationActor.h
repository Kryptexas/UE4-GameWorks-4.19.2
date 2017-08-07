// Copyright 2017 Google Inc.

#pragma once

#include "TangoPrimitives.h"
#include "GameFramework/Actor.h"

#include "TangoMapConfigurationActor.generated.h"

/**
 * A helper that makes some of the common setup in Tango simpler.
 */
UCLASS()
class TANGO_API ATangoMapConfigurationActor : public AActor
{
	GENERATED_BODY()

public:
	/** Tango configuration. */
	UPROPERTY(EditAnywhere, Category = Tango,  meta = (ShowOnlyInnerProperties))
	FTangoConfiguration TangoRunConfiguration;

private:
	// AActor interface.
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
