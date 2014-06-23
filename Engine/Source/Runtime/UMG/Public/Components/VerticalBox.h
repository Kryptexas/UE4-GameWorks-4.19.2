// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "VerticalBox.generated.h"

/**
 * A vertical box widget is a layout panel allowing child widgets to be automatically laid out
 * vertically.
 */
UCLASS(meta=( Category="Panel" ), ClassGroup=UserInterface)
class UMG_API UVerticalBox : public UPanelWidget
{
	GENERATED_UCLASS_BODY()

	/**  */
	UFUNCTION(BlueprintCallable, Category="Widget")
	UVerticalBoxSlot* AddChild(UWidget* Content);

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

	TSharedPtr<class SVerticalBox> MyVerticalBox;

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface
};
