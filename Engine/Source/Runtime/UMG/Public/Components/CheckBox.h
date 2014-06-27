// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CheckBox.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FOnCheckBoxComponentStateChanged, bool, bIsChecked );

/** Check box widget */
UCLASS(meta=( Category="Common" ), ClassGroup=UserInterface)
class UMG_API UCheckBox : public UContentWidget
{
	GENERATED_UCLASS_BODY()

protected:
	/** Style of the check box */
	UPROPERTY(EditDefaultsOnly, Category=Style, meta=( DisplayThumbnail = "true" ))
	USlateWidgetStyleAsset* Style;

	/** Image to use when the checkbox is unchecked */
	UPROPERTY(EditDefaultsOnly, Category="Style", meta=( DisplayThumbnail = "true" ), AdvancedDisplay)
	USlateBrushAsset* UncheckedImage;
	
	/** Image to use when the checkbox is unchecked and hovered */
	UPROPERTY(EditDefaultsOnly, Category="Style", meta=( DisplayThumbnail = "true" ), AdvancedDisplay)
	USlateBrushAsset* UncheckedHoveredImage;
	
	/** Image to use when the checkbox is unchecked and pressed */
	UPROPERTY(EditDefaultsOnly, Category="Style", meta=( DisplayThumbnail = "true" ), AdvancedDisplay)
	USlateBrushAsset* UncheckedPressedImage;
	
	/** Image to use when the checkbox is checked */
	UPROPERTY(EditDefaultsOnly, Category="Style", meta=( DisplayThumbnail = "true" ), AdvancedDisplay)
	USlateBrushAsset* CheckedImage;
	
	/** Image to use when the checkbox is checked and hovered */
	UPROPERTY(EditDefaultsOnly, Category="Style", meta=( DisplayThumbnail = "true" ), AdvancedDisplay)
	USlateBrushAsset* CheckedHoveredImage;
	
	/** Image to use when the checkbox is checked and pressed */
	UPROPERTY(EditDefaultsOnly, Category="Style", meta=( DisplayThumbnail = "true" ), AdvancedDisplay)
	USlateBrushAsset* CheckedPressedImage;
	
	/** Image to use when the checkbox is in an ambiguous state and hovered */
	UPROPERTY(EditDefaultsOnly, Category="Style", meta=( DisplayThumbnail = "true" ), AdvancedDisplay)
	USlateBrushAsset* UndeterminedImage;
	
	/** Image to use when the checkbox is checked and hovered */
	UPROPERTY(EditDefaultsOnly, Category="Style", meta=( DisplayThumbnail = "true" ), AdvancedDisplay)
	USlateBrushAsset* UndeterminedHoveredImage;
	
	/** Image to use when the checkbox is in an ambiguous state and pressed */
	UPROPERTY(EditDefaultsOnly, Category="Style", meta=( DisplayThumbnail = "true" ), AdvancedDisplay)
	USlateBrushAsset* UndeterminedPressedImage;

	/** Whether the check box is currently in a checked state */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	TEnumAsByte<ESlateCheckBoxState::Type> CheckedState;

	/** A bindable delegate for the IsChecked. */
	UPROPERTY()
	FGetCheckBoxState CheckedStateDelegate;

	/** How the content of the toggle button should align within the given space */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	TEnumAsByte<EHorizontalAlignment> HorizontalAlignment;

	/** Spacing between the check box image and its content */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FMargin Padding;

	/** Foreground color for the checkbox's content and parts */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FSlateColor ForegroundColor;

	/** The color of the background border */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FSlateColor BorderBackgroundColor;

	//SLATE_ATTRIBUTE( bool, ReadOnly )
	//SLATE_ARGUMENT( bool, IsFocusable )

	//SLATE_EVENT( FOnGetContent, OnGetMenuContent )

	UPROPERTY(EditDefaultsOnly, Category="Sound")
	FSlateSound CheckedSound;

	UPROPERTY(EditDefaultsOnly, Category="Sound")
	FSlateSound UncheckedSound;

	UPROPERTY(EditDefaultsOnly, Category="Sound")
	FSlateSound HoveredSound;

	/** Called when the checked state has changed */
	UPROPERTY(BlueprintAssignable)
	FOnCheckBoxComponentStateChanged OnCheckStateChanged;

	/** Returns true if this button is currently pressed */
	UFUNCTION(BlueprintCallable, Category="Widget")
	bool IsPressed() const;
	
	/** Returns true if the checkbox is currently checked */
	UFUNCTION(BlueprintCallable, Category="Widget")
	bool IsChecked() const;

	/** @return the full current checked state. */
	UFUNCTION(BlueprintCallable, Category="Widget")
	ESlateCheckBoxState::Type GetCheckedState() const;

	/** Sets the checked state. */
	UFUNCTION(BlueprintCallable, Category="Widget")
	void SetIsChecked(bool InIsChecked);

	/** Sets the checked state. */
	UFUNCTION(BlueprintCallable, Category="Widget")
	void SetCheckedState(ESlateCheckBoxState::Type InCheckedState);
	
	// UWidget interface
	virtual void SyncronizeProperties() override;
	// End of UWidget interface

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

	void SlateOnCheckStateChangedCallback(ESlateCheckBoxState::Type NewState);
	
protected:
	TSharedPtr<SCheckBox> MyCheckbox;
};
