// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UDragDropOperation

UDragDropOperation::UDragDropOperation(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UDragDropOperation::Drop_Implementation(const FPointerEvent& PointerEvent)
{
	OnDrop.Broadcast(this);
}

void UDragDropOperation::DropCanceled_Implementation(const FPointerEvent& PointerEvent)
{
	OnDropCanceled.Broadcast(this);
}

void UDragDropOperation::Dragged_Implementation(const FPointerEvent& PointerEvent)
{
	OnDragged.Broadcast(this);
}
