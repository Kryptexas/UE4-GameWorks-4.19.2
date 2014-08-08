// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SDragPanel.h"

#include "DragPanel.generated.h"

/**
 *
 */
UCLASS(meta=( Category="Primitive" ), ClassGroup=UserInterface)
class UMG_API UDragPanel : public UContentWidget
{
	GENERATED_UCLASS_BODY()

	DECLARE_DYNAMIC_DELEGATE_ThreeParams(FOnDragEvent, FGeometry, Geometry, const FPointerEvent&, PointerEvent, UDragDropOperation*&, Operation);

public:
	UPROPERTY(EditDefaultsOnly, Category=Events)
	FOnDragEvent OnDragEvent;

public:
	
	// UWidget interface
	virtual void SyncronizeProperties() override;
	// End of UWidget interface

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

	TSharedPtr<FDragDropOperation> HandleDragDetected(FGeometry Geometry, FPointerEvent PointerEvent);
	
protected:
	TSharedPtr<SDragPanel> MyDragPanel;
};
