// Copyright 2017 Google Inc.

#include "TangoMapConfigurationActor.h"
#include "TangoPluginPrivate.h"
#include "TangoLifecycle.h"

void ATangoMapConfigurationActor::BeginPlay()
{
	Super::BeginPlay();

	FTangoDevice::GetInstance()->UpdateTangoConfiguration(TangoRunConfiguration);
}

void ATangoMapConfigurationActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	FTangoDevice::GetInstance()->ResetTangoConfiguration();
}
