// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CheckBoxComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FOnCheckBoxComponentStateChanged, bool, bIsChecked );

/** Check box widget */
UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class UMG_API UCheckBoxComponent : public USlateWrapperComponent
{
	GENERATED_UCLASS_BODY()

protected:
	/** Style of the check box */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Style)
	USlateWidgetStyleAsset* Style;

	/** Called when the checked state has changed */
	UPROPERTY(BlueprintAssignable)
	FOnCheckBoxComponentStateChanged OnCheckStateChanged;

	/** Whether the check box is currently in a checked state */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
	bool bIsChecked;

	/** How the content of the toggle button should align within the given space */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	TEnumAsByte<EHorizontalAlignment> HorizontalAlignment;

	/** Spacing between the check box image and its content */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	FMargin Padding;

	/** Foreground color for the checkbox's content and parts */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	FSlateColor ForegroundColor;

	/** The color of the background border */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	FSlateColor BorderBackgroundColor;

	//SLATE_ATTRIBUTE( bool, ReadOnly )
	//SLATE_ARGUMENT( bool, IsFocusable )

	//SLATE_EVENT( FOnGetContent, OnGetMenuContent )

	UPROPERTY(EditAnywhere, Category=Sound)
	FSlateSound CheckedSound;

	UPROPERTY(EditAnywhere, Category=Sound)
	FSlateSound UncheckedSound;

	UPROPERTY(EditAnywhere, Category=Sound)
	FSlateSound HoveredSound;

protected:
	// USlateWrapperComponent interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of USlateWrapperComponent interface

	ESlateCheckBoxState::Type GetCheckState() const;
	void SlateOnCheckStateChangedCallback(ESlateCheckBoxState::Type NewState);
};
