// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CanvasPanel.generated.h"

UCLASS(meta=( Category="Panel" ), ClassGroup=UserInterface)
class UMG_API UCanvasPanel : public UPanelWidget
{
	GENERATED_UCLASS_BODY()

	/** The items placed on the canvas */
	UPROPERTY()
	TArray<UCanvasPanelSlot*> Slots;

	/** The desired size of the canvas */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FVector2D DesiredCanvasSize;

	UCanvasPanelSlot* AddSlot(UWidget* Content);

	// UPanelWidget
	virtual int32 GetChildrenCount() const OVERRIDE;
	virtual UWidget* GetChildAt(int32 Index) const OVERRIDE;
	virtual int32 GetChildIndex(UWidget* Content) const OVERRIDE;
	virtual bool AddChild(UWidget* Child, FVector2D Position) OVERRIDE;
	virtual bool RemoveChild(UWidget* Child) OVERRIDE;
	virtual void ReplaceChildAt(int32 Index, UWidget* Child) OVERRIDE;
	virtual void InsertChildAt(int32 Index, UWidget* Child) OVERRIDE;
	// End UPanelWidget

	/** Gets the underlying native canvas widget if it has been constructed */
	TSharedPtr<class SConstraintCanvas> GetCanvasWidget() const;

	/** Computes the geometry for a particular slot based on the current geometry of the canvas. */
	bool GetGeometryForSlot(UCanvasPanelSlot* Slot, FGeometry& ArrangedGeometry) const;

#if WITH_EDITOR
	// UObject interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	// End of UObject interface

	// UWidget interface
	virtual void ConnectEditorData() OVERRIDE;
	// End UWidget interface
#endif

protected:

	TSharedPtr<class SFixedSizeCanvas> MyCanvas;

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of UWidget interface
};
