// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UniformGridPanel.generated.h"

/** A panel that evenly divides up available space between all of its children. */
UCLASS(ClassGroup=UserInterface)
class UMG_API UUniformGridPanel : public UPanelWidget
{
	GENERATED_UCLASS_BODY()

public:

	/** Padding given to each slot */
	UPROPERTY(EditDefaultsOnly, Category="Child Layout")
	FMargin SlotPadding;

	/** The minimum desired width of the slots */
	UPROPERTY(EditDefaultsOnly, Category="Child Layout")
	float MinDesiredSlotWidth;

	/** The minimum desired height of the slots */
	UPROPERTY(EditDefaultsOnly, Category="Child Layout")
	float MinDesiredSlotHeight;

public:

	// UWidget interface
	virtual void SynchronizeProperties() override;
	// End of UWidget interface

	virtual void ReleaseNativeWidget() override;

#if WITH_EDITOR
	virtual const FSlateBrush* GetEditorIcon() override;
	virtual const FText GetToolboxCategory() override;
#endif

protected:

	// UPanelWidget
	virtual UClass* GetSlotClass() const override;
	virtual void OnSlotAdded(UPanelSlot* Slot) override;
	virtual void OnSlotRemoved(UPanelSlot* Slot) override;
	// End UPanelWidget

protected:

	TSharedPtr<SUniformGridPanel> MyUniformGridPanel;

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface
};
