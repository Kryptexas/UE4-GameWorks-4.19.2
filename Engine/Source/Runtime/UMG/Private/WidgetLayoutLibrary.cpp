// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"
#include "Slate/SlateBrushAsset.h"
#include "WidgetLayoutLibrary.h"

#define LOCTEXT_NAMESPACE "UMG"

/////////////////////////////////////////////////////
// UWidgetLayoutLibrary

UWidgetLayoutLibrary::UWidgetLayoutLibrary(const FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
}

UCanvasPanelSlot* UWidgetLayoutLibrary::SlotAsCanvasSlot(UWidget* ChildWidget)
{
	return Cast<UCanvasPanelSlot>(ChildWidget->Slot);
}

UGridSlot* UWidgetLayoutLibrary::SlotAsGridSlot(UWidget* ChildWidget)
{
	return Cast<UGridSlot>(ChildWidget->Slot);
}

UUniformGridSlot* UWidgetLayoutLibrary::SlotAsUniformGridSlot(UWidget* ChildWidget)
{
	return Cast<UUniformGridSlot>(ChildWidget->Slot);
}

#undef LOCTEXT_NAMESPACE