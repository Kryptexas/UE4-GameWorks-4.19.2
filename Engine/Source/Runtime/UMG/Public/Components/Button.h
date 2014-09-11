// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ButtonWidgetStyle.h"

#include "Button.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnButtonClickedEvent);

/**
 * The button is a clickable primitive widget to enable basic interaction.
 */
UCLASS(ClassGroup=UserInterface)
class UMG_API UButton : public UContentWidget
{
	GENERATED_UCLASS_BODY()

public:
	/** The template style asset, used to seed the mutable instance of the style. */
	UPROPERTY()
	USlateWidgetStyleAsset* Style_DEPRECATED;

	/** The button style used at runtime */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style", meta=( DisplayName="Style" ))
	FButtonStyle WidgetStyle;
	/*ShowOnlyInnerProperties,*/
	
	/** The color multiplier for the button content */
	UPROPERTY(EditDefaultsOnly, Category=Appearance )
	FLinearColor ColorAndOpacity;
	
	/** The color multiplier for the button background */
	UPROPERTY(EditDefaultsOnly, Category=Appearance )
	FLinearColor BackgroundColor;

	/** The type of mouse action required by the user to trigger the buttons 'Click' */
	UPROPERTY(EditDefaultsOnly, Category="Interaction", AdvancedDisplay)
	TEnumAsByte<EButtonClickMethod::Type> ClickMethod;

	/** The type of touch action required by the user to trigger the buttons 'Click' */
	UPROPERTY(EditDefaultsOnly, Category="Interaction", AdvancedDisplay)
	TEnumAsByte<EButtonTouchMethod::Type> TouchMethod;

	/** Sometimes a button should only be mouse-clickable and never keyboard focusable. */
	UPROPERTY(EditDefaultsOnly, Category="Interaction", AdvancedDisplay)
	bool IsFocusable;

public:

	/** Called when the button is clicked */
	UPROPERTY(BlueprintAssignable)
	FOnButtonClickedEvent OnClicked;
	
	/** Sets the color multiplier for the button content */
	UFUNCTION(BlueprintCallable, Category="Button|Appearance")
	void SetColorAndOpacity(FLinearColor InColorAndOpacity);

	/** Sets the color multiplier for the button background */
	UFUNCTION(BlueprintCallable, Category="Button|Appearance")
	void SetBackgroundColor(FLinearColor InBackgroundColor);

	/**
	 * Returns true if the user is actively pressing the button.  Do not use this for detecting 'Clicks', use the OnClicked event instead.
	 *
	 * @return true if the user is actively pressing the button otherwise false.
	 */
	UFUNCTION(BlueprintCallable, Category="Button")
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
	virtual const FText GetToolboxCategory() override;
#endif

protected:

	// UPanelWidget
	virtual UClass* GetSlotClass() const override;
	virtual void OnSlotAdded(UPanelSlot* Slot) override;
	virtual void OnSlotRemoved(UPanelSlot* Slot) override;
	// End UPanelWidget

protected:
	/** Handle the actual click event from slate and forward it on */
	FReply SlateHandleClicked();

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface

protected:
	/** Cached pointer to the underlying slate button owned by this UWidget */
	TSharedPtr<SButton> MyButton;

	/** Stores the style set by users dynamically. */
	TOptional<FButtonStyle> MyStyle;
};
