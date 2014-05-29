// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PanelSlot.generated.h"

UCLASS()
class UMG_API UPanelSlot : public UObject
{
	GENERATED_UCLASS_BODY()
	
	UPROPERTY()
	class UPanelWidget* Parent;

	UPROPERTY()
	class UWidget* Content;

	virtual void Resize(const FVector2D& Direction, const FVector2D& Amount);

	virtual bool CanResize(const FVector2D& Direction) const;

	virtual void MoveTo(const FVector2D& Location);

	virtual bool CanMove() const;
};
