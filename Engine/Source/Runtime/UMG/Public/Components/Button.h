// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Button.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnButtonClickedEvent);

/**
 * The button is a clickable primitive widget to enable basic interaction.
 */
UCLASS(meta=( Category="Common" ), ClassGroup=UserInterface)
class UMG_API UButton : public UContentWidget
{
	GENERATED_UCLASS_BODY()

public:
	/** Style of the button */
	UPROPERTY(EditDefaultsOnly, Category=Style, meta=( DisplayThumbnail = "true" ))
	USlateWidgetStyleAsset* Style;

	/** The scaling factor for the button border */
	UPROPERTY(EditDefaultsOnly, Category=Appearance, AdvancedDisplay)
	FVector2D DesiredSizeScale;

	/** The scaling factor for the button content */
	UPROPERTY(EditDefaultsOnly, Category=Appearance, AdvancedDisplay)
	FVector2D ContentScale;
	
	/** The color multiplier for the button content */
	UPROPERTY(EditDefaultsOnly, Category=Appearance )
	FLinearColor ColorAndOpacity;
	
	/** The color multiplier for the button background */
	UPROPERTY(EditDefaultsOnly, Category=Appearance )
	FLinearColor BackgroundColor;

	/** The type of mouse action required by the user to trigger the buttons 'Click' */
	UPROPERTY(EditDefaultsOnly, Category="Interaction")
	TEnumAsByte<EButtonClickMethod::Type> ClickMethod;

	/** The type of touch action required by the user to trigger the buttons 'Click' */
	UPROPERTY(EditDefaultsOnly, Category="Interaction")
	TEnumAsByte<EButtonTouchMethod::Type> TouchMethod;

	/** The sound made when the user 'clicks' the button */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	FSlateSound PressedSound;

	/** The sound made when the user hovers over the button */
	UPROPERTY(EditDefaultsOnly, Category=Sound)
	FSlateSound HoveredSound;

public:

	/** Called when the button is clicked */
	UPROPERTY(BlueprintAssignable)
	FOnButtonClickedEvent OnClicked;
	
	/** Sets the color multiplier for the button content */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetColorAndOpacity(FLinearColor InColorAndOpacity);

	/** Sets the color multiplier for the button background */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void SetBackgroundColor(FLinearColor InBackgroundColor);

	/** @return true if the user is actively pressing the button otherwise false.  For detecting Clicks, use the OnClicked event. */
	UFUNCTION(BlueprintCallable, Category="Widget")
	bool IsPressed() const;

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
	virtual const FSlateBrush* GetEditorIcon() override;
#endif

protected:

	// UPanelWidget
	virtual UClass* GetSlotClass() const override;
	virtual void OnSlotAdded(UPanelSlot* Slot) override;
	virtual void OnSlotRemoved(UPanelSlot* Slot) override;
	// End UPanelWidget

protected:
	FReply SlateHandleClicked();

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface

protected:
	TSharedPtr<SButton> MyButton;
};
