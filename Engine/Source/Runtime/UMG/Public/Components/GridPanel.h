// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GridPanel.generated.h"

/** A panel that evenly divides up available space between all of its children. */
UCLASS(meta=( Category="Panel" ), ClassGroup=UserInterface)
class UMG_API UGridPanel : public UPanelWidget
{
	GENERATED_UCLASS_BODY()

public:

	/** Fill */
	UPROPERTY(EditDefaultsOnly, Category="Fill Rules")
	TArray<float> ColumnFill;

	/** Fill */
	UPROPERTY(EditDefaultsOnly, Category="Fill Rules")
	TArray<float> RowFill;

public:

	// UWidget interface
	virtual void SynchronizeProperties() override;
	// End of UWidget interface

	virtual void ReleaseNativeWidget() override;

#if WITH_EDITOR
	virtual const FSlateBrush* GetEditorIcon() override;
#endif

protected:

	// UPanelWidget
	virtual UClass* GetSlotClass() const override;
	virtual void OnSlotAdded(UPanelSlot* Slot) override;
	virtual void OnSlotRemoved(UPanelSlot* Slot) override;
	// End UPanelWidget

protected:

	TSharedPtr<SGridPanel> MyGridPanel;

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface
};
