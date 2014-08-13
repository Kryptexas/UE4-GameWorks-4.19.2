// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SDropPanel.h"

#include "DropPanel.generated.h"

/**
 *
 */
UCLASS(meta=( Category="Primitive" ), ClassGroup=UserInterface)
class UMG_API UDropPanel : public UContentWidget
{
	GENERATED_UCLASS_BODY()

	DECLARE_DYNAMIC_DELEGATE_ThreeParams(FOnDragEnter, FGeometry, DropGeometry, const FPointerEvent&, PointerEvent, UDragDropOperation*, DragOperation);
	DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnDragLeave, const FPointerEvent&, PointerEvent, UDragDropOperation*, DragOperation);
	DECLARE_DYNAMIC_DELEGATE_RetVal_ThreeParams(bool, FOnDragOver, FGeometry, DropGeometry, const FPointerEvent&, PointerEvent, UDragDropOperation*, DragOperation);
	DECLARE_DYNAMIC_DELEGATE_RetVal_ThreeParams(bool, FOnDrop, FGeometry, DropGeometry, const FPointerEvent&, PointerEvent, UDragDropOperation*, DragOperation);

public:

	UPROPERTY(EditDefaultsOnly, Category=Events)
	FOnDragEnter OnDragEnterEvent;

	UPROPERTY(EditDefaultsOnly, Category=Events)
	FOnDragLeave OnDragLeaveEvent;

	UPROPERTY(EditDefaultsOnly, Category=Events)
	FOnDragOver OnDragOverEvent;

	UPROPERTY(EditDefaultsOnly, Category=Events)
	FOnDrop OnDropEvent;

public:

	virtual void ReleaseNativeWidget() override;

#if WITH_EDITOR
	virtual const FSlateBrush* GetEditorIcon() override;
#endif

protected:

	// UPanelWidget
	virtual void OnSlotAdded(UPanelSlot* Slot) override;
	virtual void OnSlotRemoved(UPanelSlot* Slot) override;
	// End UPanelWidget

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface

protected:
	void HandleDragEnter(FGeometry MyGeometry, const FDragDropEvent& DragDropEvent);

	void HandleDragLeave(const FDragDropEvent& DragDropEvent);

	FReply HandleDragOver(FGeometry MyGeometry, const FDragDropEvent& DragDropEvent);

	FReply HandleDrop(FGeometry MyGeometry, const FDragDropEvent& DragDropEvent);

protected:
	TSharedPtr<SDropPanel> MyDropPanel;
};
