// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Overlay.generated.h"

UCLASS(meta=( Category="Panel" ), ClassGroup=UserInterface)
class UMG_API UOverlay : public UPanelWidget
{
	GENERATED_UCLASS_BODY()

protected:

	// UPanelWidget
	virtual UClass* GetSlotClass() const override;
	virtual void OnSlotAdded(UPanelSlot* Slot) override;
	virtual void OnSlotRemoved(UPanelSlot* Slot) override;
	// End UPanelWidget

protected:

	TSharedPtr<class SOverlay> MyOverlay;

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface
};
