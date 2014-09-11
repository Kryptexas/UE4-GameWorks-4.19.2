// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ScrollBox.generated.h"

/** An arbitrary scrollable collection of widgets.  Great for presenting 10-100 widgets in a list.  Doesn't support virtualization. */
UCLASS(ClassGroup=UserInterface)
class UMG_API UScrollBox : public UPanelWidget
{
	GENERATED_UCLASS_BODY()

	/** The style */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style", meta=( DisplayName="Style" ))
	FScrollBoxStyle WidgetStyle;

	/** The bar style */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style", meta=( DisplayName="Bar Style" ))
	FScrollBarStyle WidgetBarStyle;

	UPROPERTY()
	USlateWidgetStyleAsset* Style_DEPRECATED;

	UPROPERTY()
	USlateWidgetStyleAsset* BarStyle_DEPRECATED;

	/** The orientation of the scrolling and stacking in the box. */
	UPROPERTY(EditDefaultsOnly, Category = "Behavior")
	TEnumAsByte<EOrientation> Orientation;

	/** Updates the scroll offset of the scrollbox */
	UFUNCTION(BlueprintCallable, Category="Widget")
	void SetScrollOffset(float NewScrollOffset);

	// UWidget interface
	virtual void SynchronizeProperties() override;
	// End of UWidget interface

	// UVisual interface
	virtual void ReleaseNativeWidget() override;
	// End of UVisual interface

	// Begin UObject interface
	virtual void PostLoad() override;
	// End of UObject interface

#if WITH_EDITOR
	// UWidget interface
	virtual const FSlateBrush* GetEditorIcon() override;
	virtual const FText GetToolboxCategory() override;
	// End UWidget interface
#endif

protected:

	// UPanelWidget
	virtual UClass* GetSlotClass() const override;
	virtual void OnSlotAdded(UPanelSlot* Slot) override;
	virtual void OnSlotRemoved(UPanelSlot* Slot) override;
	// End UPanelWidget

protected:
	/** The desired scroll offset for the underlying scrollbox.  This is a cache so that it can be set before the widget is constructed. */
	float DesiredScrollOffset;

	TSharedPtr<class SScrollBox> MyScrollBox;

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface
};
