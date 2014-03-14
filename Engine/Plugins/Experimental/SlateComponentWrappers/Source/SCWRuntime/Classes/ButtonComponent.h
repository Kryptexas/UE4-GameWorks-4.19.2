// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ButtonComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE( FOnButtonComponentClicked );

/** Buttons are clickable widgets */
UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class SCWRUNTIME_API UButtonComponent : public USlateNonLeafWidgetComponent
{
	GENERATED_UCLASS_BODY()

protected:
	/** Style of the button */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	USlateWidgetStyleAsset* ButtonStyle;

	/** Horizontal positioning of the content within the button */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	TEnumAsByte<EHorizontalAlignment> HorizontalAlignment;

	/** Vertical positioning of the content within the button */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	TEnumAsByte<EVerticalAlignment> VerticalAlignment;

	/** The padding to add around the button content. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
	FMargin ContentPadding;

	/** Should the button have text content inside? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	bool bDisplayText;

	/** Text to display as the content of the button */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance, meta=(EditCondition="bDisplayText"))
	FText ButtonText;

	/** Text to display as the content of the button */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance, meta=(EditCondition="bDisplayText"))
	USlateWidgetStyleAsset* ButtonTextStyle;

	/** The scaling factor for the button border */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	FVector2D DesiredSizeScale;

	/** The scaling factor for the button content */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Appearance)
	FVector2D ContentScale;

	/** The color multiplier for the button images */
  	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
  	FSlateColor ButtonColorAndOpacity;

	/** The foreground color of the button */
 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Appearance)
 	FSlateColor ForegroundColor;

	/** Called when the button is clicked */
	UPROPERTY(BlueprintAssignable)
	FOnButtonComponentClicked OnClicked;

protected:
	TAttribute<FString> TextAttribute;
	TWeakPtr<class SButton> MyButton;

protected:
	// USlateWrapperComponent interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of USlateWrapperComponent interface

	// USlateNonLeafWidgetComponent interface
	virtual void OnKnownChildrenChanged() OVERRIDE;
	// End of USlateNonLeafWidgetComponent

	FMargin GetContentPadding() const;
 	FString GetButtonText() const;
	FSlateColor GetButtonColor() const;
	FSlateColor GetForegroundColor() const;

	virtual FReply SlateOnClickedCallback();
};
