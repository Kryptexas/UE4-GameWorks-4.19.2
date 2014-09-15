// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "HorizontalBox.generated.h"

UCLASS(ClassGroup=UserInterface)
class UMG_API UHorizontalBox : public UPanelWidget
{
	GENERATED_UCLASS_BODY()

	/**  */
	UFUNCTION(BlueprintCallable, Category="Widget")
	UHorizontalBoxSlot* AddSlot(UWidget* Content);

	/**  */
	UFUNCTION(BlueprintCallable, Category="Slot")
	static UHorizontalBoxSlot* HorizontalBoxSlot(UWidget* Child);

#if WITH_EDITOR
	// UWidget interface
	virtual const FSlateBrush* GetEditorIcon() override;
	virtual const FText GetPaletteCategory() override;
	// End UWidget interface
#endif

	virtual void ReleaseNativeWidget() override;

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
