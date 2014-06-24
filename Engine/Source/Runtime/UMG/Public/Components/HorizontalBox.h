// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "HorizontalBox.generated.h"

UCLASS(meta=( Category="Panel" ), ClassGroup=UserInterface)
class UMG_API UHorizontalBox : public UPanelWidget
{
	GENERATED_UCLASS_BODY()

	/**  */
	UFUNCTION(BlueprintCallable, Category="Widget")
	UHorizontalBoxSlot* Add(UWidget* Content);

#if WITH_EDITOR
	// UWidget interface
	virtual const FSlateBrush* GetEditorIcon() override;
	// End UWidget interface
#endif

protected:

	// UPanelWidget
	virtual UClass* GetSlotClass() const override;
	virtual void OnSlotAdded(UPanelSlot* Slot) override;
	virtual void OnSlotRemoved(UPanelSlot* Slot) override;
	// End UPanelWidget

protected:

	TSharedPtr<class SHorizontalBox> MyHorizontalBox;

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface
};
