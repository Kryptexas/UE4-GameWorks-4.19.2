// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UPanelSlot

UPanelSlot::UPanelSlot(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UPanelSlot::Resize(const FVector2D& Direction, const FVector2D& Amount)
{

}

bool UPanelSlot::CanResize(const FVector2D& Direction) const
{
	return false;
}

void UPanelSlot::MoveTo(const FVector2D& Location)
{

}

bool UPanelSlot::CanMove() const
{
	return false;
}

bool UPanelSlot::IsDesignTime() const
{
	return Parent->IsDesignTime();
}
