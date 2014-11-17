// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ScrollBox.generated.h"

/** An arbitrary scrollable collection of widgets.  Great for presenting 10-100 widgets in a list.  Doesn't support virtualization. */
UCLASS(meta=( Category="Panel" ), ClassGroup=UserInterface)
class UMG_API UScrollBox : public UPanelWidget
{
	GENERATED_UCLASS_BODY()

	/** Removes all child widget from the scrollbox */
	UFUNCTION(BlueprintCallable, Category="Widget")
	void ClearChildren();

	/** Updates the scroll offset of the scrollbox */
	UFUNCTION(BlueprintCallable, Category="Widget")
	void SetScrollOffset(float NewScrollOffset);

	// UWidget interface
	virtual void SyncronizeProperties() override;
	// End of UWidget interface

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
	/** The desired scroll offset for the underlying scrollbox.  This is a cache so that it can be set before the widget is constructed. */
	float DesiredScrollOffset;

	TSharedPtr<class SScrollBox> MyScrollBox;

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface
};
