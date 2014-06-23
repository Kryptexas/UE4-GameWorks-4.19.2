// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#if WITH_EDITOR
#include "MessageLog.h"
#include "UObjectToken.h"
#endif

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UContentWidget

UContentWidget::UContentWidget(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bCanHaveMultipleChildren = false;
}

UPanelSlot* UContentWidget::GetContentSlot() const
{
	return Slots.Num() > 0 ? Slots[0] : NULL;
}

UClass* UContentWidget::GetSlotClass() const
{
	return UPanelSlot::StaticClass();
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE
