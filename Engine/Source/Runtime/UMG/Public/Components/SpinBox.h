// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SpinBox.generated.h"

/** Spin box widget */
UCLASS(ClassGroup = UserInterface)
class UMG_API USpinBox : public UWidget
{
	GENERATED_UCLASS_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSpinBoxValueChangedEvent, float, InValue);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSpinBoxValueCommittedEvent, float, InValue, ETextCommit::Type, CommitMethod);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSpinBoxBeginSliderMovement);

public:
	/** Text style */
	UPROPERTY(EditDefaultsOnly, Category = Style, meta = (DisplayThumbnail = "true"))
	USlateWidgetStyleAsset* Style;

	/** Value stored in this spin box */
	UPROPERTY(EditDefaultsOnly, Category = Content)
	float Value;

	/** The amount by which to change the spin box value as the slider moves. */
	UPROPERTY(EditDefaultsOnly, Category = Content)
	float Delta;

	/** The exponent by which to increase the delta as the mouse moves. 1 is constant (never increases the delta). */
	UPROPERTY(EditDefaultsOnly, Category = Content)
	float SliderExponent;
	
	/** Font color and opacity (overrides style) */
	UPROPERTY(EditDefaultsOnly, Category = Content)
	FSlateFontInfo Font;

	/** The minimum width of the spin box */
	UPROPERTY(EditDefaultsOnly, Category = Content, AdvancedDisplay, DisplayName = "Minimum Desired Width")
	float MinDesiredWidth;

	/** Whether to remove the keyboard focus from the spin box when the value is committed */
	UPROPERTY(EditDefaultsOnly, Category = Content, AdvancedDisplay)
	bool ClearKeyboardFocusOnCommit;

	/** Whether to select the text in the spin box when the value is committed */
	UPROPERTY(EditDefaultsOnly, Category = Content, AdvancedDisplay)
	bool SelectAllTextOnCommit;

public:
	/** Called when the value is changed interactively by the user */
	UPROPERTY(BlueprintAssignable, Category = "Widget Event")
	FOnSpinBoxValueChangedEvent OnValueChanged;

	/** Called when the value is committed. Occurs when the user presses Enter or the text box loses focus. */
	UPROPERTY(BlueprintAssignable, Category = "Widget Event")
	FOnSpinBoxValueCommittedEvent OnValueCommitted;

	/** Called right before the slider begins to move */
	UPROPERTY(BlueprintAssignable, Category = "Widget Event")
	FOnSpinBoxBeginSliderMovement OnBeginSliderMovement;

	/** Called right after the slider handle is released by the user */
	UPROPERTY(BlueprintAssignable, Category = "Widget Event")
	FOnSpinBoxValueChangedEvent OnEndSliderMovement;

	/** Get the current value of the spin box. */
	UFUNCTION(BlueprintCallable, Category = "Behavior")
	float GetValue() const;

	/** Set the value of the spin box. */
	UFUNCTION(BlueprintCallable, Category = "Behavior")
	void SetValue(float NewValue);

	// MIN VALUE
	/** Get the current minimum value that can be manually set in the spin box. */
	UFUNCTION(BlueprintCallable, Category = "Behavior")
	float GetMinValue() const;

	/** Set the minimum value that can be manually set in the spin box. */
	UFUNCTION(BlueprintCallable, Category = "Behavior")
	void SetMinValue(float NewValue);

	/** Clear the minimum value that can be manually set in the spin box. */
	UFUNCTION(BlueprintCallable, Category = "Behavior")
	void ClearMinValue();

	// MAX VALUE
	/** Get the current maximum value that can be manually set in the spin box. */
	UFUNCTION(BlueprintCallable, Category = "Behavior")
	float GetMaxValue() const;

	/** Set the maximum value that can be manually set in the spin box. */
	UFUNCTION(BlueprintCallable, Category = "Behavior")
	void SetMaxValue(float NewValue);

	/** Clear the maximum value that can be manually set in the spin box. */
	UFUNCTION(BlueprintCallable, Category = "Behavior")
	void ClearMaxValue();

	// MIN SLIDER VALUE
	/** Get the current minimum value that can be specified using the slider. */
	UFUNCTION(BlueprintCallable, Category = "Behavior")
	float GetMinSliderValue() const;

	/** Set the minimum value that can be specified using the slider. */
	UFUNCTION(BlueprintCallable, Category = "Behavior")
	void SetMinSliderValue(float NewValue);

	/** Clear the minimum value that can be specified using the slider. */
	UFUNCTION(BlueprintCallable, Category = "Behavior")
	void ClearMinSliderValue();

	// MAX SLIDER VALUE
	/** Get the current maximum value that can be specified using the slider. */
	UFUNCTION(BlueprintCallable, Category = "Behavior")
	float GetMaxSliderValue() const;

	/** Set the maximum value that can be specified using the slider. */
	UFUNCTION(BlueprintCallable, Category = "Behavior")
	void SetMaxSliderValue(float NewValue);

	/** Clear the maximum value that can be specified using the slider. */
	UFUNCTION(BlueprintCallable, Category = "Behavior")
	void ClearMaxSliderValue();

	// UVisual interface
	virtual void ReleaseNativeWidget() override;
	// End of UVisual interface

	// UWidget interface
	virtual void SynchronizeProperties() override;

#if WITH_EDITOR
	virtual const FSlateBrush* GetEditorIcon() override;
	virtual const FText GetToolboxCategory() override;
#endif
	// End of UWidget interface

	/** A bindable delegate to allow logic to drive the value of the widget */
	UPROPERTY()
	FGetFloat ValueDelegate;

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget

	void HandleOnValueChanged(float InValue);
	void HandleOnValueCommitted(float InValue, ETextCommit::Type CommitMethod);
	void HandleOnBeginSliderMovement();
	void HandleOnEndSliderMovement(float InValue);

protected:
	/** Whether the optional MinValue attribute of the widget is set */
	UPROPERTY()
	uint32 bOverride_MinValue : 1;

	/** Whether the optional MaxValue attribute of the widget is set */
	UPROPERTY()
	uint32 bOverride_MaxValue : 1;

	/** Whether the optional MinSliderValue attribute of the widget is set */
	UPROPERTY()
	uint32 bOverride_MinSliderValue : 1;

	/** Whether the optional MaxSliderValue attribute of the widget is set */
	UPROPERTY()
	uint32 bOverride_MaxSliderValue : 1;

	/** The minimum allowable value that can be manually entered into the spin box */
	UPROPERTY(EditDefaultsOnly, Category = Content, DisplayName = "Minimum Value", meta = (editcondition = "bOverride_MinValue"))
	float MinValue;

	/** The maximum allowable value that can be manually entered into the spin box */
	UPROPERTY(EditDefaultsOnly, Category = Content, DisplayName = "Maximum Value", meta = (editcondition = "bOverride_MaxValue"))
	float MaxValue;

	/** The minimum allowable value that can be specified using the slider */
	UPROPERTY(EditDefaultsOnly, Category = Content, DisplayName = "Minimum Slider Value", meta = (editcondition = "bOverride_MinSliderValue"))
	float MinSliderValue;

	/** The maximum allowable value that can be specified using the slider */
	UPROPERTY(EditDefaultsOnly, Category = Content, DisplayName = "Maximum Slider Value", meta = (editcondition = "bOverride_MaxSliderValue"))
	float MaxSliderValue;

protected:
	TSharedPtr<SSpinBox<float>> MySpinBox;
};