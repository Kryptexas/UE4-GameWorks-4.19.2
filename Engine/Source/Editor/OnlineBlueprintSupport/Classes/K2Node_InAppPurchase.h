// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "K2Node_InAppPurchase.generated.h"

UCLASS()
class ONLINEBLUEPRINTSUPPORT_API UK2Node_InAppPurchase : public UK2Node_BaseAsyncTask
{
	GENERATED_BODY()
public:
	UK2Node_InAppPurchase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
