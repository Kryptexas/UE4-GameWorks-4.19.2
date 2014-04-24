// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PanelSlot.generated.h"

UCLASS()
class UMG_API UPanelSlot : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	class USlateWrapperComponent* Content;
};
