// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CanvasPanel.generated.h"

UCLASS(meta=( Category="Panel" ), ClassGroup=UserInterface)
class UMG_API UCanvasPanel : public UPanelWidget
{
	GENERATED_UCLASS_BODY()

public:

	/** Gets the underlying native canvas widget if it has been constructed */
	TSharedPtr<class SConstraintCanvas> GetCanvasWidget() const;

	/** Computes the geometry for a particular slot based on the current geometry of the canvas. */
	bool GetGeometryForSlot(int32 SlotIndex, FGeometry& ArrangedGeometry) const;

	/** Computes the geometry for a particular slot based on the current geometry of the canvas. */
	bool GetGeometryForSlot(UCanvasPanelSlot* Slot, FGeometry& ArrangedGeometry) const;

	void ReleaseNativeWidget() override;

#if WITH_EDITOR
	// UWidget interface
	virtual const FSlateBrush* GetEditorIcon() override;
	// End UWidget interface

	// UWidget interface
	virtual bool LockToPanelOnDrag() const override
	{
		return true;
	}
	// End UWidget interface
#endif

protected:

	// UPanelWidget
	virtual UClass* GetSlotClass() const override;
	virtual void OnSlotAdded(UPanelSlot* Slot) override;
	virtual void OnSlotRemoved(UPanelSlot* Slot) override;
	// End UPanelWidget

protected:

	TSharedPtr<class SConstraintCanvas> MyCanvas;

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface
};
