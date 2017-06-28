/* Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
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
