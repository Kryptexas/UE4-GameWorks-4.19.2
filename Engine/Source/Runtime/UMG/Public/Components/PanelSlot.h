// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PanelSlot.generated.h"

UCLASS()
class UMG_API UPanelSlot : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	class USlateWrapperComponent* Content;

	virtual void Resize(const FVector2D& Direction, const FVector2D& Amount);

	virtual bool CanResize(const FVector2D& Direction) const;
};
