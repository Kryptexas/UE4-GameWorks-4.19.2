// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CanvasPanel.generated.h"

UCLASS(meta=( Category="Panel" ), ClassGroup=UserInterface)
class UMG_API UCanvasPanel : public UPanelWidget
{
	GENERATED_UCLASS_BODY()

	/** The desired size of the canvas */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FVector2D DesiredCanvasSize;

	/** Gets the underlying native canvas widget if it has been constructed */
	TSharedPtr<class SConstraintCanvas> GetCanvasWidget() const;

	/** Computes the geometry for a particular slot based on the current geometry of the canvas. */
	bool GetGeometryForSlot(int32 SlotIndex, FGeometry& ArrangedGeometry) const;

	/** Computes the geometry for a particular slot based on the current geometry of the canvas. */
	bool GetGeometryForSlot(UCanvasPanelSlot* Slot, FGeometry& ArrangedGeometry) const;

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

	TSharedPtr<class SFixedSizeCanvas> MyCanvas;

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface
};
