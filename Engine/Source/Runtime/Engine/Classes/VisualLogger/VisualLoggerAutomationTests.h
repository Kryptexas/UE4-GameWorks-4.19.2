// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Misc/AutomationTest.h"
#include "VisualLoggerAutomationTests.generated.h"

UCLASS(NotBlueprintable, Transient, hidecategories = UObject, notplaceable)
class UVisualLoggerAutomationTests : public UObject
{
	GENERATED_BODY()
public:
	UVisualLoggerAutomationTests(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

