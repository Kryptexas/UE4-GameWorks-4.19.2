// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "InvalidationBox.generated.h"

/**
 * Invalidate
 * ● Single Child
 * ● Popup
 */
UCLASS()
class UMG_API UInvalidationBox : public UContentWidget
{
	GENERATED_UCLASS_BODY()

public:

	/**
	 * 
	 */
	UFUNCTION(BlueprintCallable, Category="Invalidation Box")
	void InvalidateCache();

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

#if WITH_EDITOR
	virtual const FSlateBrush* GetEditorIcon() override;
	virtual const FText GetPaletteCategory() override;
#endif

protected:

	// UPanelWidget
	virtual void OnSlotAdded(UPanelSlot* Slot) override;
	virtual void OnSlotRemoved(UPanelSlot* Slot) override;
	// End UPanelWidget

	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface

protected:
	TSharedPtr<class SInvalidationPanel> MyInvalidationPanel;
};
