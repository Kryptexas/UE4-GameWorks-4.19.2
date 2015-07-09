// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "InvalidationBox.generated.h"

/**
 * Invalidate
 * ● Single Child
 * ● Caching / Performance
 */
UCLASS()
class UMG_API UInvalidationBox : public UContentWidget
{
	GENERATED_UCLASS_BODY()

public:

	/**
	 * Caches the locations for child draw elements relative to the invalidation box,
	 * this adds extra overhead to drawing them every frame.  However, in cases where
	 * the position of the invalidation boxes changes every frame this can be a big savings.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Caching")
	bool CacheRelativeTransforms;

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
